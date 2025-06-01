#pragma once

#include <simpledbus/advanced/Interface.h>
#include <simpledbus/external/kvn_safe_callback.hpp>

#include <simplebluez/Types.h>

#include <string>

namespace SimpleBluez {

class GattCharacteristic1 : public SimpleDBus::Interface {
  public:
    typedef enum { REQUEST = 0, COMMAND } WriteType;

    GattCharacteristic1(std::shared_ptr<SimpleDBus::Connection> conn, std::string path);
    virtual ~GattCharacteristic1();

    // ----- METHODS -----
    void StartNotify();
    void StopNotify();
    void WriteValue(const ByteArray& value, WriteType type);
    ByteArray ReadValue();

    // ----- PROPERTIES -----
    CachedProperty<std::string> UUID = create_cached_property<std::string>("UUID");
    CachedProperty<ByteArray> Value = create_cached_property<ByteArray>("Value");
    Property<bool> Notifying = create_property<bool>("Notifying");
    Property<std::vector<std::string>> Flags = create_property<std::vector<std::string>>("Flags");
    Property<uint16_t> MTU = create_property<uint16_t>("MTU");

    // ----- CALLBACKS -----
    kvn::safe_callback<void()> OnValueChanged;

  protected:
    void property_changed(std::string option_name) override;

    ByteArray _value;
};

}  // namespace SimpleBluez
