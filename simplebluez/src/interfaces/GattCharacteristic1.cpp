#include "simplebluez/interfaces/GattCharacteristic1.h"

#include <exception>
#include <map>
#include <utility>

using namespace SimpleBluez;

const SimpleDBus::AutoRegisterInterface<GattCharacteristic1> GattCharacteristic1::registry{
    "org.bluez.GattCharacteristic1",
    // clang-format off
    [](std::shared_ptr<SimpleDBus::Connection> conn, std::shared_ptr<SimpleDBus::Proxy> proxy) -> std::shared_ptr<SimpleDBus::Interface> {
        return std::static_pointer_cast<SimpleDBus::Interface>(std::make_shared<GattCharacteristic1>(conn, proxy));
    }
    // clang-format on
};

GattCharacteristic1::GattCharacteristic1(std::shared_ptr<SimpleDBus::Connection> conn,
                                         std::shared_ptr<SimpleDBus::Proxy> proxy)
    : SimpleDBus::Interface(conn, proxy, "org.bluez.GattCharacteristic1") {}

// IMPORTANT: The destructor is defined here (instead of inline) to anchor the vtable to this object file.
// This prevents the linker from stripping this translation unit and ensures the static 'registry' variable is
// initialized at startup.
GattCharacteristic1::~GattCharacteristic1() = default;

void GattCharacteristic1::StartNotify() {
    auto msg = create_method_call("StartNotify");
    _conn->send_with_reply(msg);
}

void GattCharacteristic1::StopNotify() {
    auto msg = create_method_call("StopNotify");
    _conn->send_with_reply(msg);
}

void GattCharacteristic1::WriteValue(const ByteArray& value, WriteType type) {
    SimpleDBus::Holder value_data = SimpleDBus::Holder::create<std::vector<SimpleDBus::Holder>>();
    for (size_t i = 0; i < value.size(); i++) {
        value_data.array_append(SimpleDBus::Holder::create<uint8_t>(value[i]));
    }

    SimpleDBus::Holder options = SimpleDBus::Holder::create<std::map<std::string, SimpleDBus::Holder>>();
    if (type == WriteType::REQUEST) {
        options.dict_append(SimpleDBus::Holder::Type::STRING, "type",
                            SimpleDBus::Holder::create<std::string>("request"));
    } else if (type == WriteType::COMMAND) {
        options.dict_append(SimpleDBus::Holder::Type::STRING, "type",
                            SimpleDBus::Holder::create<std::string>("command"));
    }

    auto msg = create_method_call("WriteValue");
    msg.append_argument(value_data, "ay");
    msg.append_argument(options, "a{sv}");
    _conn->send_with_reply(msg);
}

ByteArray GattCharacteristic1::ReadValue() {
    auto msg = create_method_call("ReadValue");

    // NOTE: ReadValue requires an additional argument, which currently is not supported
    SimpleDBus::Holder options = SimpleDBus::Holder::create<std::map<std::string, SimpleDBus::Holder>>();
    msg.append_argument(options, "a{sv}");

    SimpleDBus::Message reply_msg = _conn->send_with_reply(msg);
    SimpleDBus::Holder value = reply_msg.extract();

    Value.set(value);
    return Value();
}

void GattCharacteristic1::enable_acquire_notify() {
    NotifyAcquired.set(false).emit();
}

void GattCharacteristic1::disable_acquire_notify() {
    property_invalidate("NotifyAcquired");
}

void GattCharacteristic1::message_handle(SimpleDBus::Message& msg) {
    if (msg.is_method_call(_interface_name, "ReadValue")) {
        SimpleDBus::Holder options = msg.extract();
        ValueOptions value_options = _parse_value_options(options);

        OnReadValue(value_options);

        SimpleDBus::Message reply = SimpleDBus::Message::create_method_return(msg);
        reply.append_argument(_properties["Value"]->get(), "ay");
        _conn->send(reply);
    } else if (msg.is_method_call(_interface_name, "WriteValue")) {
        SimpleDBus::Holder value = msg.extract();
        msg.extract_next();
        SimpleDBus::Holder options = msg.extract();
        ValueOptions value_options = _parse_value_options(options);

        Value.set(value);
        SimpleDBus::Message reply = SimpleDBus::Message::create_method_return(msg);
        _conn->send(reply);

        OnWriteValue(Value.get(), value_options);
    } else if (msg.is_method_call(_interface_name, "AcquireNotify")) {
        if (!NotifyAcquired.valid() || !OnAcquireNotify.is_loaded()) {
            SimpleDBus::Message reply =
                SimpleDBus::Message::create_error(msg, "org.bluez.Error.NotSupported", "AcquireNotify not supported");
            _conn->send(reply);
            return;
        }

        SimpleDBus::Holder options = msg.extract();
        ValueOptions value_options = _parse_value_options(options);

        SimpleDBus::UnixSocket bluez_socket;
        SimpleDBus::UnixSocket session_socket;
        try {
            auto sockets = SimpleDBus::UnixSocket::create_pair();
            bluez_socket = std::move(sockets.first);
            session_socket = std::move(sockets.second);
        } catch (const std::exception& e) {
            SimpleDBus::Message reply = SimpleDBus::Message::create_error(msg, "org.bluez.Error.Failed", e.what());
            _conn->send(reply);
            return;
        }

        uint16_t mtu = value_options.mtu.value_or(0);
        int fd = bluez_socket.fd();
        SimpleDBus::Message reply = SimpleDBus::Message::create_method_return(msg);
        DBusMessage* reply_raw = reply;
        if (!dbus_message_append_args(reply_raw, DBUS_TYPE_UNIX_FD, &fd, DBUS_TYPE_UINT16, &mtu, DBUS_TYPE_INVALID)) {
            SimpleDBus::Message error = SimpleDBus::Message::create_error(
                msg, "org.bluez.Error.Failed", "Failed to append AcquireNotify response");
            _conn->send(error);
            return;
        }

        _conn->send(reply);

        OnAcquireNotify(std::move(session_socket), value_options);
    } else if (msg.is_method_call(_interface_name, "StartNotify")) {
        SimpleDBus::Message reply = SimpleDBus::Message::create_method_return(msg);
        _conn->send(reply);

        Notifying.set(true).emit();

        OnStartNotify();
    } else if (msg.is_method_call(_interface_name, "StopNotify")) {
        SimpleDBus::Message reply = SimpleDBus::Message::create_method_return(msg);
        _conn->send(reply);

        Notifying.set(false).emit();

        OnStopNotify();
    }
}

GattCharacteristic1::ValueOptions GattCharacteristic1::_parse_value_options(const SimpleDBus::Holder& options) {
    ValueOptions parsed;

    const auto options_map = options.get<std::map<std::string, SimpleDBus::Holder>>();

    if (auto option = options_map.find("device"); option != options_map.end()) {
        if (option->second.type() == SimpleDBus::Holder::Type::OBJ_PATH) {
            parsed.device = option->second.get<SimpleDBus::ObjectPath>();
        }
    }

    if (auto option = options_map.find("mtu"); option != options_map.end()) {
        if (option->second.type() == SimpleDBus::Holder::Type::UINT16) {
            parsed.mtu = option->second.get<uint16_t>();
        }
    }

    if (auto option = options_map.find("offset"); option != options_map.end()) {
        if (option->second.type() == SimpleDBus::Holder::Type::UINT16) {
            parsed.offset = option->second.get<uint16_t>();
        }
    }

    if (auto option = options_map.find("type"); option != options_map.end()) {
        if (option->second.type() == SimpleDBus::Holder::Type::STRING) {
            parsed.type = option->second.get<std::string>();
        }
    }

    if (auto option = options_map.find("link"); option != options_map.end()) {
        if (option->second.type() == SimpleDBus::Holder::Type::STRING) {
            parsed.link = option->second.get<std::string>();
        }
    }

    if (auto option = options_map.find("prepare-authorize"); option != options_map.end()) {
        if (option->second.type() == SimpleDBus::Holder::Type::BOOLEAN) {
            parsed.prepare_authorize = option->second.get<bool>();
        }
    }

    return parsed;
}
