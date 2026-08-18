#import "Utils.h"

#import <fmt/core.h>

#include <simpleble/Exceptions.h>

SimpleBLE::BluetoothUUID uuidToSimpleBLE(CBUUID* uuid) {
    std::string uuid_raw = [[[uuid UUIDString] lowercaseString] UTF8String];

    if (uuid_raw.length() == 4) {
        return fmt::format("0000{}-0000-1000-8000-00805f9b34fb", uuid_raw);
    } else {
        return uuid_raw;
    }
}

NSString* uuidToString(CBUUID* uuid) {
    NSString* uuidString = [[uuid UUIDString] lowercaseString];

    if ([uuidString length] == 4) {
        return [NSString stringWithFormat:@"0000%@-0000-1000-8000-00805f9b34fb", uuidString];
    } else {
        return uuidString;
    }
}

CBUUID* uuidFromSimpleBLE(const SimpleBLE::BluetoothUUID& uuid) {
    NSString* uuidString = [[NSString alloc] initWithBytes:uuid.data() length:uuid.size() encoding:NSUTF8StringEncoding];
    if (uuidString == nil || uuidString.length == 0) {
        throw SimpleBLE::Exception::OperationFailed("The Bluetooth UUID is not valid UTF-8.");
    }

    @try {
        CBUUID* cbUuid = [CBUUID UUIDWithString:uuidString];
        if (cbUuid == nil) {
            throw SimpleBLE::Exception::OperationFailed("Invalid Bluetooth UUID: " + uuid);
        }
        return cbUuid;
    } @catch (NSException* exception) {
        throw SimpleBLE::Exception::OperationFailed("Invalid Bluetooth UUID " + uuid + ": " + std::string(exception.reason.UTF8String));
    }
}
