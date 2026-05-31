#pragma once

#include <simpledbus/advanced/Interface.h>
#include <simpledbus/advanced/InterfaceRegistry.h>
#include <simpledbus/base/UnixSocket.h>

#include <simplebluez/Types.h>

#include <memory>
#include <optional>
#include <string>

namespace SimpleBluez {

class GattCharacteristic1 : public SimpleDBus::Interface {
  public:
    typedef enum { REQUEST = 0, COMMAND } WriteType;

    struct ValueOptions {
        std::optional<SimpleDBus::ObjectPath> device;
        std::optional<uint16_t> mtu;
        std::optional<uint16_t> offset;
        std::optional<std::string> type;
        std::optional<std::string> link;
        std::optional<bool> prepare_authorize;
    };

    GattCharacteristic1(std::shared_ptr<SimpleDBus::Connection> conn, std::shared_ptr<SimpleDBus::Proxy> proxy);
    virtual ~GattCharacteristic1();

    // ----- METHODS -----
    void StartNotify();
    void StopNotify();
    void WriteValue(const ByteArray& value, WriteType type);
    ByteArray ReadValue();

    // ----- PROPERTIES -----
    Property<std::string>& UUID = property<std::string>("UUID");
    Property<SimpleDBus::ObjectPath>& Service = property<SimpleDBus::ObjectPath>("Service");
    Property<ByteArray>& Value = property<ByteArray>("Value");
    Property<bool>& Notifying = property<bool>("Notifying");
    Property<bool>& NotifyAcquired = property<bool>("NotifyAcquired");
    Property<std::vector<std::string>>& Flags = property<std::vector<std::string>>("Flags", {"read", "write", "notify"});
    // For local GATT server objects, this property is not a per-device negotiated MTU.
    // Use ValueOptions::mtu from server-side ReadValue/WriteValue callbacks instead.
    Property<uint16_t>& MTU = property<uint16_t>("MTU");

    // ----- CALLBACKS -----
    kvn::safe_callback<void()> OnValueChanged;
    kvn::safe_callback<void(ByteArray value, ValueOptions options)> OnWriteValue;
    kvn::safe_callback<void(ValueOptions options)> OnReadValue;
    kvn::safe_callback<void()> OnStartNotify;
    kvn::safe_callback<void()> OnStopNotify;
    // The socket is the application-owned endpoint of the acquired notify stream.
    kvn::safe_callback<void(SimpleDBus::UnixSocket socket, ValueOptions options)> OnAcquireNotify;

    // ------ SPECIAL METHODS ------
    void enable_acquire_notify();
    void disable_acquire_notify();

    void message_handle(SimpleDBus::Message& msg) override;

  private:
    ValueOptions _parse_value_options(const SimpleDBus::Holder& options);

    static const SimpleDBus::AutoRegisterInterface<GattCharacteristic1> registry;
};

}  // namespace SimpleBluez
