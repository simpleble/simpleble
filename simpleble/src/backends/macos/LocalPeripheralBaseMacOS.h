#pragma once

#import <CoreBluetooth/CoreBluetooth.h>
#import <Foundation/Foundation.h>

namespace SimpleBLE::Local {
class PeripheralMac;
}

NS_ASSUME_NONNULL_BEGIN

@interface LocalPeripheralBaseMacOS : NSObject<CBPeripheralManagerDelegate>

- (instancetype)init:(SimpleBLE::Local::PeripheralMac*)peripheral;
- (void*)underlying;
- (nullable NSString*)startWithServices:(NSArray<CBMutableService*>*)services
                      advertisementData:(NSDictionary<NSString*, id>*)advertisementData;
- (void)stop;
- (bool)isAdvertising;
- (void)publishValue:(NSData*)value forCharacteristic:(CBMutableCharacteristic*)characteristic;
- (void)detach;

@end

NS_ASSUME_NONNULL_END
