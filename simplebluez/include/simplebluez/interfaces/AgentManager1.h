#pragma once

#include <simpledbus/advanced/Interface.h>
#include <simpledbus/advanced/Registry.h>

#include <string>

namespace SimpleBluez {

class AgentManager1 : public SimpleDBus::Interface {
  public:
    AgentManager1(std::shared_ptr<SimpleDBus::Connection> conn, std::string path);
    virtual ~AgentManager1() = default;

    // ----- METHODS -----
    void RegisterAgent(std::string agent, std::string capability);
    void RequestDefaultAgent(std::string agent);
    void UnregisterAgent(std::string agent);

  private:
    static const SimpleDBus::AutoRegister<AgentManager1> registry;
};

}  // namespace SimpleBluez
