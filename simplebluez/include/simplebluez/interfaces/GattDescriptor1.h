#pragma once

#include <simpledbus/advanced/Interface.h>
#include <simpledbus/advanced/InterfaceRegistry.h>

#include "kvn/kvn_safe_callback.hpp"

#include <simplebluez/Types.h>

#include <string>

#include <simpledbus/base/HolderUtils.h>

namespace SimpleBluez {

class GattDescriptor1 : public SimpleDBus::Interface {
  public:
    GattDescriptor1(std::shared_ptr<SimpleDBus::Connection> conn, std::shared_ptr<SimpleDBus::Proxy> proxy);
    virtual ~GattDescriptor1();

    // ----- METHODS -----
    void WriteValue(const ByteArray& value);
    ByteArray ReadValue();

    // ----- PROPERTIES -----
    Property<std::string>& UUID = property<std::string>("UUID");
    CustomProperty<ByteArray>& Value = property<ByteArray>("Value", SimpleDBus::HolderUtils::from_byte_array,
                                                           SimpleDBus::HolderUtils::to_byte_array);

  private:
    static const SimpleDBus::AutoRegisterInterface<GattDescriptor1> registry;
};

}  // namespace SimpleBluez
