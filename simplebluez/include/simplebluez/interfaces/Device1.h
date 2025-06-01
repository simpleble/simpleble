#pragma once

#include <simpledbus/advanced/Interface.h>
#include <simpledbus/external/kvn_safe_callback.hpp>

#include <string>

namespace SimpleBluez {

class Device1 : public SimpleDBus::Interface {
  public:
    Device1(std::shared_ptr<SimpleDBus::Connection> conn, std::string path);
    virtual ~Device1();

    // ----- METHODS -----
    void Connect();
    void Disconnect();
    void Pair();
    void CancelPairing();

    // ----- PROPERTIES -----
    CachedProperty<int16_t> TxPower = create_cached_property<int16_t>("TxPower");
    CachedProperty<std::map<uint16_t, std::vector<uint8_t>>> ManufacturerData = 
        create_cached_property<std::map<uint16_t, std::vector<uint8_t>>>("ManufacturerData");
    CachedProperty<std::map<std::string, std::vector<uint8_t>>> ServiceData = 
        create_cached_property<std::map<std::string, std::vector<uint8_t>>>("ServiceData");
    Property<int16_t> RSSI = create_property<int16_t>("RSSI");
    Property<uint16_t> Appearance = create_property<uint16_t>("Appearance");
    Property<std::string> Address = create_property<std::string>("Address");
    Property<std::string> AddressType = create_property<std::string>("AddressType");
    Property<std::string> Alias = create_property<std::string>("Alias");
    Property<std::string> Name = create_property<std::string>("Name");
    Property<std::vector<std::string>> UUIDs = create_property<std::vector<std::string>>("UUIDs");
    Property<bool> Paired = create_property<bool>("Paired");
    Property<bool> Connected = create_property<bool>("Connected");
    Property<bool> ServicesResolved = create_property<bool>("ServicesResolved");

    // ----- CALLBACKS -----
    kvn::safe_callback<void()> OnServicesResolved;
    kvn::safe_callback<void()> OnDisconnected;

  protected:
    void property_changed(std::string option_name) override;

    int16_t _rssi = INT16_MIN;
    int16_t _tx_power = INT16_MIN;
    std::string _name;
    std::string _alias;
    std::string _address;
    std::string _address_type;
    bool _connected;
    bool _services_resolved;
    std::map<uint16_t, std::vector<uint8_t>> _manufacturer_data;
    std::map<std::string, std::vector<uint8_t>> _service_data;
};

}  // namespace SimpleBluez
