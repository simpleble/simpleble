#pragma once

#include <simpledbus/advanced/Proxy.h>

#include <simplebluez/Adapter.h>
#include <simplebluez/Agent.h>

namespace SimpleBluez {

class BluezOrg : public SimpleDBus::Proxy {
  public:
    BluezOrg(std::shared_ptr<SimpleDBus::Connection> conn, const std::string& bus_name, const std::string& path);
    virtual ~BluezOrg() = default;

    std::vector<std::shared_ptr<Adapter>> get_adapters();
    void register_agent(std::shared_ptr<Agent> agent);

  private:
    std::shared_ptr<SimpleDBus::Proxy> path_create(const std::string& path) override;
};

}  // namespace SimpleBluez
