#pragma once

#include <simpledbus/advanced/Proxy.h>
#include <simpledbus/interfaces/ObjectManager.h>

#include <simplebluez/Adapter.h>
#include <simplebluez/Agent.h>

#include <vector>

namespace SimpleBluez {

class BluezRoot : public SimpleDBus::Proxy {
  public:

    BluezRoot(std::shared_ptr<SimpleDBus::Connection> conn, const std::string& bus_name, const std::string& path);
    virtual ~BluezRoot() = default;

    void load_managed_objects();

    std::vector<std::shared_ptr<Adapter>> get_adapters();
    std::shared_ptr<Agent> get_agent();
    void register_agent();

  private:
    std::shared_ptr<SimpleDBus::Proxy> path_create(const std::string& path) override;
    std::shared_ptr<SimpleDBus::ObjectManager> object_manager();

    std::shared_ptr<Agent> _agent;
};

}  // namespace SimpleBluez
