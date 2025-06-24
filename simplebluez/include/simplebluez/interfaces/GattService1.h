#pragma once

#include <simpledbus/advanced/Interface.h>
#include <simpledbus/advanced/InterfaceRegistry.h>
#include <string>

namespace SimpleBluez {

class GattService1 : public SimpleDBus::Interface {
  public:
    GattService1(std::shared_ptr<SimpleDBus::Connection> conn, std::string path);
    virtual ~GattService1() = default;

    // ----- METHODS -----

    // ----- PROPERTIES -----
    std::string UUID();

  protected:
    void property_changed(std::string option_name) override;

    std::string _uuid;

  private:
    static const SimpleDBus::AutoRegisterInterface<GattService1> registry;
};

}  // namespace SimpleBluez
