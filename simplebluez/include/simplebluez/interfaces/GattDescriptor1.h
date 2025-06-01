#pragma once

#include <simpledbus/advanced/Interface.h>
#include <simpledbus/external/kvn_safe_callback.hpp>

#include <simplebluez/Types.h>

#include <string>

namespace SimpleBluez {

class GattDescriptor1 : public SimpleDBus::Interface {
  public:
    GattDescriptor1(std::shared_ptr<SimpleDBus::Connection> conn, std::string path);
    virtual ~GattDescriptor1();

    // ----- METHODS -----
    void WriteValue(const ByteArray& value);
    ByteArray ReadValue();

    // ----- PROPERTIES -----
    CachedProperty<std::string> UUID = create_cached_property<std::string>("UUID");
    CachedProperty<ByteArray> Value = create_cached_property<ByteArray>("Value");

    // ----- CALLBACKS -----
    kvn::safe_callback<void()> OnValueChanged;

  protected:
    void property_changed(std::string option_name) override;
};

}  // namespace SimpleBluez
