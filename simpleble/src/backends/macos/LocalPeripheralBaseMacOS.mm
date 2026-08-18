#import "LocalPeripheralBaseMacOS.h"

#include <exception>
#include <memory>
#include <vector>

#import "LocalCharacteristicMac.h"
#import "LocalPeripheralMac.h"
#import "LoggingInternal.h"

namespace {

const void* kPeripheralManagerQueueKey = &kPeripheralManagerQueueKey;
constexpr NSTimeInterval kOperationTimeout = 5.0;

NSString* clientIdentifier(CBCentral* central) { return central.identifier.UUIDString.lowercaseString; }

}  // namespace

@interface LocalPeripheralBaseMacOS ()

@property(nonatomic, assign) SimpleBLE::Local::PeripheralMac* peripheral;
@property(nonatomic, strong) dispatch_queue_t managerQueue;
@property(nonatomic, strong) CBPeripheralManager* manager;
@property(nonatomic, strong) NSCondition* condition;
@property(nonatomic, assign) NSUInteger pendingServiceCount;
@property(nonatomic, assign) BOOL advertisingCompleted;
@property(nonatomic, assign) BOOL active;
@property(nonatomic, copy, nullable) NSString* operationError;
@property(nonatomic, strong) NSMapTable<CBMutableCharacteristic*, NSData*>* pendingUpdates;

- (BOOL)isOnManagerQueue;
- (void)performOnManagerQueueAndWait:(dispatch_block_t)block;

@end

@implementation LocalPeripheralBaseMacOS

- (instancetype)init:(SimpleBLE::Local::PeripheralMac*)peripheral {
    self = [super init];
    if (self) {
        _peripheral = peripheral;
        _condition = [[NSCondition alloc] init];
        _pendingUpdates = [NSMapTable strongToStrongObjectsMapTable];

        dispatch_queue_attr_t attributes = dispatch_queue_attr_make_with_qos_class(DISPATCH_QUEUE_SERIAL, QOS_CLASS_USER_INITIATED, -1);
        _managerQueue = dispatch_queue_create("LocalPeripheralBaseMacOS.managerQueue", attributes);
        dispatch_queue_set_specific(_managerQueue, kPeripheralManagerQueueKey, (__bridge void*)self, nullptr);
        _manager = [[CBPeripheralManager alloc] initWithDelegate:self queue:_managerQueue options:nil];
    }
    return self;
}

- (void*)underlying {
    return (__bridge void*)self.manager;
}

- (nullable NSString*)startWithServices:(NSArray<CBMutableService*>*)services
                      advertisementData:(NSDictionary<NSString*, id>*)advertisementData {
    if ([self isOnManagerQueue]) {
        return @"A local peripheral cannot be started from a CoreBluetooth peripheral callback.";
    }

    NSDate* deadline = [NSDate dateWithTimeIntervalSinceNow:kOperationTimeout];
    [self.condition lock];
    while (self.manager.state == CBManagerStateUnknown) {
        if (![self.condition waitUntilDate:deadline]) {
            break;
        }
    }

    if (self.manager.state != CBManagerStatePoweredOn) {
        NSString* error = [NSString
            stringWithFormat:@"CoreBluetooth peripheral manager is not powered on (state %ld).", (long)self.manager.state];
        [self.condition unlock];
        return error;
    }

    self.pendingServiceCount = services.count;
    self.advertisingCompleted = NO;
    self.operationError = nil;
    [self.condition unlock];

    dispatch_async(self.managerQueue, ^{
      for (CBMutableService* service in services) {
          [self.manager addService:service];
      }
    });

    [self.condition lock];
    while (self.pendingServiceCount > 0) {
        if (![self.condition waitUntilDate:deadline]) {
            self.operationError = @"Timed out while publishing local GATT services.";
            break;
        }
    }
    NSString* serviceError = self.operationError;
    [self.condition unlock];

    if (serviceError != nil) {
        [self stop];
        return serviceError;
    }

    dispatch_async(self.managerQueue, ^{
      [self.manager startAdvertising:advertisementData];
    });

    [self.condition lock];
    while (!self.advertisingCompleted) {
        if (![self.condition waitUntilDate:deadline]) {
            self.operationError = @"Timed out while starting local peripheral advertising.";
            break;
        }
    }
    NSString* advertisingError = self.operationError;
    [self.condition unlock];

    if (advertisingError != nil) {
        [self stop];
    }
    return advertisingError;
}

- (void)stop {
    [self performOnManagerQueueAndWait:^{
      self.active = NO;
      [self.manager stopAdvertising];
      [self.manager removeAllServices];
      [self.pendingUpdates removeAllObjects];
    }];
}

- (bool)isAdvertising {
    if ([self isOnManagerQueue]) {
        return self.manager.isAdvertising;
    }

    __block BOOL advertising = NO;
    dispatch_sync(self.managerQueue, ^{
      advertising = self.manager.isAdvertising;
    });
    return advertising;
}

- (void)publishValue:(NSData*)value forCharacteristic:(CBMutableCharacteristic*)characteristic {
    dispatch_async(self.managerQueue, ^{
      if (!self.active) {
          return;
      }
      if (![self.manager updateValue:value forCharacteristic:characteristic onSubscribedCentrals:nil]) {
          [self.pendingUpdates setObject:value forKey:characteristic];
      } else {
          [self.pendingUpdates removeObjectForKey:characteristic];
      }
    });
}

- (void)detach {
    [self performOnManagerQueueAndWait:^{
      self.active = NO;
      [self.manager stopAdvertising];
      [self.manager removeAllServices];
      [self.pendingUpdates removeAllObjects];
      self.peripheral = nullptr;
      self.manager.delegate = nil;
    }];
}

- (BOOL)isOnManagerQueue {
    return dispatch_get_specific(kPeripheralManagerQueueKey) == (__bridge void*)self;
}

- (void)performOnManagerQueueAndWait:(dispatch_block_t)block {
    if ([self isOnManagerQueue]) {
        block();
    } else {
        dispatch_sync(self.managerQueue, block);
    }
}

#pragma mark - CBPeripheralManagerDelegate

- (void)peripheralManagerDidUpdateState:(CBPeripheralManager*)peripheral {
    self.active = peripheral.isAdvertising;
    if (!self.active) {
        [self.pendingUpdates removeAllObjects];
    }
    [self.condition lock];
    [self.condition broadcast];
    [self.condition unlock];
}

- (void)peripheralManager:(CBPeripheralManager*)peripheral didAddService:(CBService*)service error:(nullable NSError*)error {
    [self.condition lock];
    if (error != nil && self.operationError == nil) {
        self.operationError = error.localizedDescription;
    }
    if (self.pendingServiceCount > 0) {
        --self.pendingServiceCount;
    }
    [self.condition broadcast];
    [self.condition unlock];
}

- (void)peripheralManagerDidStartAdvertising:(CBPeripheralManager*)peripheral error:(nullable NSError*)error {
    [self.condition lock];
    if (error != nil) {
        self.operationError = error.localizedDescription;
    }
    self.active = error == nil;
    self.advertisingCompleted = YES;
    [self.condition broadcast];
    [self.condition unlock];
}

- (void)peripheralManager:(CBPeripheralManager*)peripheral
                         central:(CBCentral*)central
    didSubscribeToCharacteristic:(CBCharacteristic*)characteristic {
    auto* localPeripheral = self.peripheral;
    if (localPeripheral != nullptr) {
        localPeripheral->handle_subscribed((__bridge void*)characteristic, [clientIdentifier(central) UTF8String]);
    }
}

- (void)peripheralManager:(CBPeripheralManager*)peripheral
                             central:(CBCentral*)central
    didUnsubscribeFromCharacteristic:(CBCharacteristic*)characteristic {
    auto* localPeripheral = self.peripheral;
    if (localPeripheral != nullptr) {
        localPeripheral->handle_unsubscribed((__bridge void*)characteristic, [clientIdentifier(central) UTF8String]);
    }
}

- (void)peripheralManager:(CBPeripheralManager*)peripheral didReceiveReadRequest:(CBATTRequest*)request {
    auto* localPeripheral = self.peripheral;
    if (localPeripheral == nullptr) {
        [peripheral respondToRequest:request withResult:CBATTErrorUnlikelyError];
        return;
    }

    auto characteristic = localPeripheral->characteristic_for((__bridge void*)request.characteristic);
    if (!characteristic || !characteristic->can_read()) {
        [peripheral respondToRequest:request withResult:CBATTErrorReadNotPermitted];
        return;
    }

    try {
        SimpleBLE::ByteArray value = characteristic->handle_read();
        if (request.offset > value.size()) {
            [peripheral respondToRequest:request withResult:CBATTErrorInvalidOffset];
            return;
        }

        const auto* bytes = value.empty() ? nullptr : value.data() + request.offset;
        request.value = [NSData dataWithBytes:bytes length:value.size() - request.offset];
        [peripheral respondToRequest:request withResult:CBATTErrorSuccess];
    } catch (const std::exception& ex) {
        SIMPLEBLE_LOG_ERROR(fmt::format("Exception while handling local characteristic read: {}", ex.what()));
        [peripheral respondToRequest:request withResult:CBATTErrorUnlikelyError];
    } catch (...) {
        SIMPLEBLE_LOG_ERROR("Unknown exception while handling local characteristic read");
        [peripheral respondToRequest:request withResult:CBATTErrorUnlikelyError];
    }
}

- (void)peripheralManager:(CBPeripheralManager*)peripheral didReceiveWriteRequests:(NSArray<CBATTRequest*>*)requests {
    if (requests.count == 0) {
        return;
    }

    auto* localPeripheral = self.peripheral;
    if (localPeripheral == nullptr) {
        [peripheral respondToRequest:requests.firstObject withResult:CBATTErrorUnlikelyError];
        return;
    }

    std::vector<std::pair<std::shared_ptr<SimpleBLE::Local::CharacteristicMac>, SimpleBLE::ByteArray>> writes;
    writes.reserve(requests.count);

    for (CBATTRequest* request in requests) {
        auto characteristic = localPeripheral->characteristic_for((__bridge void*)request.characteristic);
        if (!characteristic || !characteristic->can_write()) {
            [peripheral respondToRequest:request withResult:CBATTErrorWriteNotPermitted];
            return;
        }
        if (request.offset != 0) {
            [peripheral respondToRequest:request withResult:CBATTErrorInvalidOffset];
            return;
        }
        if (request.value == nil) {
            [peripheral respondToRequest:request withResult:CBATTErrorInvalidAttributeValueLength];
            return;
        }

        writes.emplace_back(characteristic, SimpleBLE::ByteArray(static_cast<const uint8_t*>(request.value.bytes), request.value.length));
    }

    try {
        for (auto& [characteristic, value] : writes) {
            characteristic->handle_write(std::move(value));
        }
        [peripheral respondToRequest:requests.firstObject withResult:CBATTErrorSuccess];
    } catch (const std::exception& ex) {
        SIMPLEBLE_LOG_ERROR(fmt::format("Exception while handling local characteristic write: {}", ex.what()));
        [peripheral respondToRequest:requests.firstObject withResult:CBATTErrorUnlikelyError];
    } catch (...) {
        SIMPLEBLE_LOG_ERROR("Unknown exception while handling local characteristic write");
        [peripheral respondToRequest:requests.firstObject withResult:CBATTErrorUnlikelyError];
    }
}

- (void)peripheralManagerIsReadyToUpdateSubscribers:(CBPeripheralManager*)peripheral {
    NSMutableArray<CBMutableCharacteristic*>* characteristics = [NSMutableArray array];
    for (CBMutableCharacteristic* characteristic in self.pendingUpdates.keyEnumerator) {
        [characteristics addObject:characteristic];
    }
    for (CBMutableCharacteristic* characteristic in characteristics) {
        NSData* value = [self.pendingUpdates objectForKey:characteristic];
        if (![peripheral updateValue:value forCharacteristic:characteristic onSubscribedCentrals:nil]) {
            return;
        }
        [self.pendingUpdates removeObjectForKey:characteristic];
    }
}

@end
