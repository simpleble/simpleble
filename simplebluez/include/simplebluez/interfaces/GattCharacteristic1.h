#pragma once

#include <simpledbus/advanced/Interface.h>
#include <simpledbus/advanced/InterfaceRegistry.h>
#include <simpledbus/base/HolderUtils.h>

#include <simplebluez/Types.h>

#include <string>

namespace SimpleBluez {

class GattCharacteristic1 : public SimpleDBus::Interface {
  public:
    typedef enum { REQUEST = 0, COMMAND } WriteType;

    GattCharacteristic1(std::shared_ptr<SimpleDBus::Connection> conn, std::shared_ptr<SimpleDBus::Proxy> proxy);
    virtual ~GattCharacteristic1();

    // ----- METHODS -----
    void StartNotify();
    void StopNotify();
    void WriteValue(const ByteArray& value, WriteType type);
    ByteArray ReadValue();

    // ----- PROPERTIES -----
    Property<std::string>& UUID = property<std::string>("UUID");
    Property<std::string>& Service = property<std::string>("Service");
    CustomProperty<ByteArray>& Value = property<ByteArray>("Value", SimpleDBus::HolderUtils::from_byte_array,
                                                           SimpleDBus::HolderUtils::to_byte_array);
    Property<bool>& Notifying = property<bool>("Notifying");
    CustomProperty<std::vector<std::string>>& Flags = property<std::vector<std::string>>(
        "Flags", SimpleDBus::HolderUtils::from_string_array, SimpleDBus::HolderUtils::to_string_array);
    Property<uint16_t>& MTU = property<uint16_t>("MTU");

    // ----- CALLBACKS -----
    kvn::safe_callback<void()> OnValueChanged;
    kvn::safe_callback<void(ByteArray value)> OnWriteValue;
    kvn::safe_callback<void()> OnReadValue;
    kvn::safe_callback<void()> OnStartNotify;
    kvn::safe_callback<void()> OnStopNotify;

    void message_handle(SimpleDBus::Message& msg) override;

  private:
    static const SimpleDBus::AutoRegisterInterface<GattCharacteristic1> registry;
};

}  // namespace SimpleBluez
