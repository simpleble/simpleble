#pragma once

#include <simpledbus/advanced/Interface.h>
#include <simpledbus/advanced/InterfaceRegistry.h>

namespace SimpleDBus::Interfaces {

class Properties : public Interface {
  public:
    Properties(std::shared_ptr<Connection> conn, std::shared_ptr<Proxy> proxy);
    virtual ~Properties();

    Holder Get(const std::string& interface_name, const std::string& property_name);
    Holder GetAll(const std::string& interface_name);
    void Set(const std::string& interface_name, const std::string& property_name, const Holder& value);

    void PropertiesChanged(const std::string& interface_name, const std::map<std::string, Holder>& changed_properties);
    void PropertiesChanged(const std::string& interface_name, const std::vector<std::string>& invalidated_properties);
    void PropertiesChanged(const std::string& interface_name, const std::map<std::string, Holder>& changed_properties,
                           const std::vector<std::string>& invalidated_properties);

    void message_handle(Message& msg) override;

    // NOTE: In this particular case, we had to make the registry public to make it
    // visible to Proxy, so that we can make sure there's a live reference to it
    // to prevent the linker from optimizing it away.
    static const SimpleDBus::AutoRegisterInterface<Properties> registry;
};

}  // namespace SimpleDBus::Interfaces