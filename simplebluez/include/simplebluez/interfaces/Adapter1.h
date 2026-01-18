#pragma once

#include <simpledbus/advanced/Interface.h>
#include <simpledbus/advanced/InterfaceRegistry.h>

#include <optional>
#include <string>
#include <vector>

namespace SimpleBluez {

class Adapter1 : public SimpleDBus::Interface {
  public:
    // ----- TYPES -----
    struct DiscoveryFilter {
        typedef enum { AUTO, BREDR, LE } TransportType;
        std::vector<std::string> UUIDs = {};
        std::optional<int16_t> RSSI;
        std::optional<uint16_t> Pathloss;
        TransportType Transport = TransportType::AUTO;
        bool DuplicateData = true;
        bool Discoverable = false;
        std::string Pattern = "";
    };

    // ----- CONSTRUCTORS -----
    Adapter1(std::shared_ptr<SimpleDBus::Connection> conn, std::shared_ptr<SimpleDBus::Proxy> proxy);
    virtual ~Adapter1();

    // ----- METHODS -----
    void RemoveDevice(std::string device_path);
    void StartDiscovery();
    void StopDiscovery();
    void SetDiscoveryFilter(DiscoveryFilter filter);
    SimpleDBus::Holder GetDiscoveryFilters();

    // ----- PROPERTIES -----
    Property<bool>& Discovering = create_property<bool>("Discovering");
    Property<bool>& Powered = create_property<bool>("Powered");
    Property<std::string>& Address = create_property<std::string>("Address");

  private:
    static const SimpleDBus::AutoRegisterInterface<Adapter1> registry;
};

}  // namespace SimpleBluez
