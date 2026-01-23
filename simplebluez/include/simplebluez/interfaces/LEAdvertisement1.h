#pragma once
#include <simpledbus/advanced/Interface.h>
#include <simpledbus/advanced/InterfaceRegistry.h>
#include "kvn/kvn_safe_callback.hpp"

#include <cstdint>
#include <map>
#include <string>
#include <vector>

#include "simplebluez/Types.h"
#include "simpledbus/base/HolderUtils.h"

namespace SimpleBluez {

class LEAdvertisement1 : public SimpleDBus::Interface {
  public:
    // ----- CONSTRUCTORS -----

    LEAdvertisement1(std::shared_ptr<SimpleDBus::Connection> conn, std::shared_ptr<SimpleDBus::Proxy> proxy);
    virtual ~LEAdvertisement1();

    // ----- SIGNALS -----
    kvn::safe_callback<void()> OnRelease;

    // ----- PROPERTIES -----

    Property<std::string>& Type = property<std::string>("Type");
    CustomProperty<std::vector<std::string>>& ServiceUUIDs = property<std::vector<std::string>>(
        "ServiceUUIDs", SimpleDBus::HolderUtils::from_string_array, SimpleDBus::HolderUtils::to_string_array);
    CustomProperty<std::map<uint16_t, ByteArray>>& ManufacturerData = property<std::map<uint16_t, ByteArray>>(
        "ManufacturerData", SimpleDBus::HolderUtils::from_dict_uint16_byte_array, SimpleDBus::HolderUtils::to_dict_uint16_byte_array);
    CustomProperty<std::map<std::string, ByteArray>>& ServiceData = property<std::map<std::string, ByteArray>>(
        "ServiceData", SimpleDBus::HolderUtils::from_dict_string_byte_array, SimpleDBus::HolderUtils::to_dict_string_byte_array);
    CustomProperty<std::vector<std::string>>& SolicitUUIDs = property<std::vector<std::string>>(
        "SolicitUUIDs", SimpleDBus::HolderUtils::from_string_array, SimpleDBus::HolderUtils::to_string_array);
    CustomProperty<std::map<uint8_t, ByteArray>>& Data = property<std::map<uint8_t, ByteArray>>(
        "Data", SimpleDBus::HolderUtils::from_dict_uint8_byte_array, SimpleDBus::HolderUtils::to_dict_uint8_byte_array);
    Property<bool>& Discoverable = property<bool>("Discoverable");
    Property<uint16_t>& DiscoverableTimeout = property<uint16_t>("DiscoverableTimeout");
    CustomProperty<std::vector<std::string>>& Includes = property<std::vector<std::string>>(
        "Includes", SimpleDBus::HolderUtils::from_string_array, SimpleDBus::HolderUtils::to_string_array);
    Property<std::string>& LocalName = property<std::string>("LocalName");
    Property<uint16_t>& Appearance = property<uint16_t>("Appearance");
    Property<uint16_t>& Duration = property<uint16_t>("Duration");
    Property<uint16_t>& Timeout = property<uint16_t>("Timeout");
    Property<std::string>& SecondaryChannel = property<std::string>("SecondaryChannel");
    Property<uint32_t>& MinInterval = property<uint32_t>("MinInterval");
    Property<uint32_t>& MaxInterval = property<uint32_t>("MaxInterval");
    Property<int16_t>& TxPower = property<int16_t>("TxPower");
    Property<bool>& IncludeTxPower = property<bool>("IncludeTxPower");

    void message_handle(SimpleDBus::Message& msg) override;

  private:
    static const SimpleDBus::AutoRegisterInterface<LEAdvertisement1> registry;
};

}  // namespace SimpleBluez