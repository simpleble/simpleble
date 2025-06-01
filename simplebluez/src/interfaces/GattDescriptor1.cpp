#include "simplebluez/interfaces/GattDescriptor1.h"

using namespace SimpleBluez;

GattDescriptor1::GattDescriptor1(std::shared_ptr<SimpleDBus::Connection> conn, std::string path)
    : SimpleDBus::Interface(conn, "org.bluez", path, "org.bluez.GattDescriptor1") {}

GattDescriptor1::~GattDescriptor1() { OnValueChanged.unload(); }

void GattDescriptor1::WriteValue(const ByteArray& value) {
    SimpleDBus::Holder value_data = SimpleDBus::Holder::create_array();
    for (size_t i = 0; i < value.size(); i++) {
        value_data.array_append(SimpleDBus::Holder::create_byte(value[i]));
    }

    SimpleDBus::Holder options = SimpleDBus::Holder::create_dict();

    auto msg = create_method_call("WriteValue");
    msg.append_argument(value_data, "ay");
    msg.append_argument(options, "a{sv}");
    _conn->send_with_reply_and_block(msg);
}

ByteArray GattDescriptor1::ReadValue() {
    auto msg = create_method_call("ReadValue");

    // NOTE: ReadValue requires an additional argument, which currently is not supported
    SimpleDBus::Holder options = SimpleDBus::Holder::create_dict();
    msg.append_argument(options, "a{sv}");

    SimpleDBus::Message reply_msg = _conn->send_with_reply_and_block(msg);
    Value.set(reply_msg.extract().get_string());

    return Value.get();
}

void GattDescriptor1::property_changed(std::string option_name) {
    if (option_name == "UUID") {
        UUID.update_cached_property();
    } else if (option_name == "Value") {
        Value.update_cached_property();
    }
}
