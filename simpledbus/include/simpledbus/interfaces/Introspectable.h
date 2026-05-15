#pragma once

#include <simpledbus/advanced/Interface.h>
#include <simpledbus/advanced/InterfaceRegistry.h>

namespace SimpleDBus::Interfaces {

class Introspectable : public Interface {
  public:
    Introspectable(std::shared_ptr<Connection> conn, std::shared_ptr<Proxy> proxy);
    virtual ~Introspectable();

    std::string Introspect();

    void message_handle(Message& msg) override;

  private:
    static const SimpleDBus::AutoRegisterInterface<Introspectable> registry;
};

}  // namespace SimpleDBus::Interfaces
