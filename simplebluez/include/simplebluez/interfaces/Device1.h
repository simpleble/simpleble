#pragma once

#include <simpledbus/advanced/Interface.h>
#include <simpledbus/advanced/InterfaceRegistry.h>

#include "kvn/kvn_safe_callback.hpp"

#include <string>

#include <simpledbus/base/HolderUtils.h>
#include "simplebluez/Types.h"

namespace SimpleBluez {

class Device1 : public SimpleDBus::Interface {
  public:
    Device1(std::shared_ptr<SimpleDBus::Connection> conn, std::shared_ptr<SimpleDBus::Proxy> proxy);
    virtual ~Device1();

    // ----- METHODS -----
    void Connect();
    void Disconnect();
    void Pair();
    void CancelPairing();

    // ----- PROPERTIES -----
    Property<int16_t>& RSSI = create_property<int16_t>("RSSI");
    Property<int16_t>& TxPower = create_property<int16_t>("TxPower");
    Property<uint16_t>& Appearance = create_property<uint16_t>("Appearance");
    Property<std::string>& Address = create_property<std::string>("Address");
    Property<std::string>& AddressType = create_property<std::string>("AddressType");
    Property<std::string>& Alias = create_property<std::string>("Alias");
    Property<std::string>& Name = create_property<std::string>("Name");
    CustomProperty<std::vector<std::string>>& UUIDs = create_custom_property<std::vector<std::string>>(
        "UUIDs", [](std::vector<std::string> value) { return SimpleDBus::HolderUtils::from_string_array(value); },
        [](SimpleDBus::Holder value) { return SimpleDBus::HolderUtils::to_string_array(value); });
    CustomProperty<std::map<uint16_t, ByteArray>>& ManufacturerData = create_custom_property<std::map<uint16_t, ByteArray>>(
        "ManufacturerData",
        [](std::map<uint16_t, ByteArray> value) {
            return SimpleDBus::HolderUtils::from_dict_uint16_byte_array({value.begin(), value.end()});
        },
        [](SimpleDBus::Holder value) {
            auto result = SimpleDBus::HolderUtils::to_dict_uint16_byte_array(value);
            return std::map<uint16_t, ByteArray>{result.begin(), result.end()};
        });
    CustomProperty<std::map<std::string, ByteArray>>& ServiceData = create_custom_property<std::map<std::string, ByteArray>>(
        "ServiceData",
        [](std::map<std::string, ByteArray> value) {
            return SimpleDBus::HolderUtils::from_dict_string_byte_array({value.begin(), value.end()});
        },
        [](SimpleDBus::Holder value) {
            auto result = SimpleDBus::HolderUtils::to_dict_string_byte_array(value);
            return std::map<std::string, ByteArray>{result.begin(), result.end()};
        });
    Property<bool>& Paired = create_property<bool>("Paired");
    Property<bool>& Connected = create_property<bool>("Connected");
    Property<bool>& ServicesResolved = create_property<bool>("ServicesResolved");

    // ----- CALLBACKS -----
    kvn::safe_callback<void()> OnServicesResolved;
    kvn::safe_callback<void()> OnDisconnected;

  protected:
    void property_changed(std::string option_name) override;

  private:
    static const SimpleDBus::AutoRegisterInterface<Device1> registry;
};

}  // namespace SimpleBluez
