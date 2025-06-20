#pragma once

#include <simpledbus/advanced/Proxy.h>

#include <simplebluez/Adapter.h>
#include <simplebluez/Agent.h>

#include <simplebluez/interfaces/AgentManager1.h>

namespace SimpleBluez {

class BluezOrgBluez : public SimpleDBus::Proxy {
  public:
    BluezOrgBluez(std::shared_ptr<SimpleDBus::Connection> conn, const std::string& bus_name, const std::string& path);
    virtual ~BluezOrgBluez() = default;

    void register_agent(std::shared_ptr<Agent> agent);

    std::vector<std::shared_ptr<Adapter>> get_adapters();

  private:
    std::shared_ptr<SimpleDBus::Proxy> path_create(const std::string& path) override;
    std::shared_ptr<AgentManager1> agentmanager1();
};

}  // namespace SimpleBluez
