#include <gtest/gtest.h>

#include <simplebluez/standard/Device.h>
#include <simplebluez/standard/Service.h>

#include <map>
#include <string>
#include <vector>

using namespace SimpleBluez;
using namespace SimpleDBus;

namespace {

Holder with_uuid(const std::string& interface_name, const std::string& uuid) {
    return Holder::create(std::map<std::string, Holder>{
        {interface_name, Holder::create(std::map<std::string, Holder>{{"UUID", Holder::create(uuid)}})}});
}

Holder removed_interface(const std::string& interface_name) {
    return Holder::create(std::vector<std::string>{interface_name});
}

}  // namespace

TEST(StandardLookup, DeviceSkipsInvalidServiceWithMatchingUuid) {
    const std::string device_path = "/org/bluez/hci0/dev_00";
    const std::string service_uuid = "1234";
    auto device = Proxy::create<Device>(nullptr, "", device_path);

    const std::string old_path = device_path + "/service0001";
    device->path_add(old_path, with_uuid("org.bluez.GattService1", service_uuid));
    auto old_service = device->get_service(service_uuid);

    device->path_remove(old_path, removed_interface("org.bluez.GattService1"));
    ASSERT_FALSE(old_service->valid());

    const std::string new_path = device_path + "/service0002";
    device->path_add(new_path, with_uuid("org.bluez.GattService1", service_uuid));

    EXPECT_EQ(new_path, device->get_service(service_uuid)->path());
}

TEST(StandardLookup, ServiceSkipsInvalidCharacteristicWithMatchingUuid) {
    const std::string service_path = "/org/bluez/hci0/dev_00/service0001";
    const std::string characteristic_uuid = "5678";
    auto service = Proxy::create<Service>(nullptr, "", service_path);

    const std::string old_path = service_path + "/char0001";
    service->path_add(old_path, with_uuid("org.bluez.GattCharacteristic1", characteristic_uuid));
    auto old_characteristic = service->get_characteristic(characteristic_uuid);

    service->path_remove(old_path, removed_interface("org.bluez.GattCharacteristic1"));
    ASSERT_FALSE(old_characteristic->valid());

    const std::string new_path = service_path + "/char0002";
    service->path_add(new_path, with_uuid("org.bluez.GattCharacteristic1", characteristic_uuid));

    EXPECT_EQ(new_path, service->get_characteristic(characteristic_uuid)->path());
}
