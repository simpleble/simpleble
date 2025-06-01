#include "simplebluez/interfaces/GattCharacteristic1.h"

using namespace SimpleBluez;

GattCharacteristic1::GattCharacteristic1(std::shared_ptr<SimpleDBus::Connection> conn, std::string path)
    : SimpleDBus::Interface(conn, "org.bluez", path, "org.bluez.GattCharacteristic1") {}

GattCharacteristic1::~GattCharacteristic1() { OnValueChanged.unload(); }

void GattCharacteristic1::StartNotify() {
    auto msg = create_method_call("StartNotify");
    _conn->send_with_reply_and_block(msg);
}

void GattCharacteristic1::StopNotify() {
    auto msg = create_method_call("StopNotify");
    _conn->send_with_reply_and_block(msg);
}

void GattCharacteristic1::WriteValue(const ByteArray& value, WriteType type) {
    SimpleDBus::Holder value_data = SimpleDBus::Holder::create_array();
    for (size_t i = 0; i < value.size(); i++) {
        value_data.array_append(SimpleDBus::Holder::create_byte(value[i]));
    }

    SimpleDBus::Holder options = SimpleDBus::Holder::create_dict();
    if (type == WriteType::REQUEST) {
        options.dict_append(SimpleDBus::Holder::Type::STRING, "type", SimpleDBus::Holder::create_string("request"));
    } else if (type == WriteType::COMMAND) {
        options.dict_append(SimpleDBus::Holder::Type::STRING, "type", SimpleDBus::Holder::create_string("command"));
    }

    auto msg = create_method_call("WriteValue");
    msg.append_argument(value_data, "ay");
    msg.append_argument(options, "a{sv}");
    _conn->send_with_reply_and_block(msg);
}

ByteArray GattCharacteristic1::ReadValue() {
    auto msg = create_method_call("ReadValue");

    // NOTE: ReadValue requires an additional argument, which currently is not supported
    SimpleDBus::Holder options = SimpleDBus::Holder::create_dict();
    msg.append_argument(options, "a{sv}");

    SimpleDBus::Message reply_msg = _conn->send_with_reply_and_block(msg);
    SimpleDBus::Holder value = reply_msg.extract();
    Value.set(value.get_string());

    return Value.get();
}


void GattCharacteristic1::property_changed(std::string option_name) {
    if (option_name == "UUID") {
        UUID.update_cached_property();
    } else if (option_name == "Value") {
        Value.update_cached_property();
        OnValueChanged();
    }
}
