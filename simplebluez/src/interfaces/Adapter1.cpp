#include "simplebluez/interfaces/Adapter1.h"

using namespace SimpleBluez;

const SimpleDBus::AutoRegisterInterface<Adapter1> Adapter1::registry{
    "org.bluez.Adapter1",
    // clang-format off
    [](std::shared_ptr<SimpleDBus::Connection> conn, std::shared_ptr<SimpleDBus::Proxy> proxy) -> std::shared_ptr<SimpleDBus::Interface> {
        return std::static_pointer_cast<SimpleDBus::Interface>(std::make_shared<Adapter1>(conn, proxy));
    }
    // clang-format on
};

Adapter1::Adapter1(std::shared_ptr<SimpleDBus::Connection> conn, std::shared_ptr<SimpleDBus::Proxy> proxy)
    : SimpleDBus::Interface(conn, proxy, "org.bluez.Adapter1") {}

// IMPORTANT: The destructor is defined here (instead of inline) to anchor the vtable to this object file.
// This prevents the linker from stripping this translation unit and ensures the static 'registry' variable is
// initialized at startup.
Adapter1::~Adapter1() = default;

void Adapter1::StartDiscovery() {
    auto msg = create_method_call("StartDiscovery");
    _conn->send_with_reply(msg);
}

void Adapter1::StopDiscovery() {
    auto msg = create_method_call("StopDiscovery");
    _conn->send_with_reply(msg);
    // NOTE: It might take a few seconds until the peripheral reports that is has actually stopped discovering.
}

SimpleDBus::Holder Adapter1::GetDiscoveryFilters() {
    auto msg = create_method_call("GetDiscoveryFilters");
    SimpleDBus::Message reply_msg = _conn->send_with_reply(msg);
    SimpleDBus::Holder discovery_filters = reply_msg.extract();
    return discovery_filters;
}

void Adapter1::SetDiscoveryFilter(DiscoveryFilter filter) {
    SimpleDBus::Holder properties = SimpleDBus::Holder::create_array();

    if (filter.UUIDs.size() > 0) {
        SimpleDBus::Holder uuids = SimpleDBus::Holder::create_array();
        for (size_t i = 0; i < filter.UUIDs.size(); i++) {
            uuids.array_append(SimpleDBus::Holder::create_string(filter.UUIDs.at(i)));
        }
        SimpleDBus::Holder dict = SimpleDBus::Holder::create_dict();
        dict.dict_append(SimpleDBus::Holder::Type::STRING, "UUIDs", uuids);
        properties.array_append(dict);
    }

    if (filter.RSSI.has_value()) {
        SimpleDBus::Holder dict = SimpleDBus::Holder::create_dict();
        dict.dict_append(SimpleDBus::Holder::Type::STRING, "RSSI",
                         SimpleDBus::Holder::create_int16(filter.RSSI.value()));
        properties.array_append(dict);
    }

    if (filter.Pathloss.has_value()) {
        SimpleDBus::Holder dict = SimpleDBus::Holder::create_dict();
        dict.dict_append(SimpleDBus::Holder::Type::STRING, "Pathloss",
                         SimpleDBus::Holder::create_uint16(filter.Pathloss.value()));
        properties.array_append(dict);
    }

    {
        SimpleDBus::Holder dict = SimpleDBus::Holder::create_dict();
        switch (filter.Transport) {
            case DiscoveryFilter::TransportType::AUTO: {
                dict.dict_append(SimpleDBus::Holder::Type::STRING, "Transport",
                                 SimpleDBus::Holder::create_string("auto"));
                break;
            }
            case DiscoveryFilter::TransportType::BREDR: {
                dict.dict_append(SimpleDBus::Holder::Type::STRING, "Transport",
                                 SimpleDBus::Holder::create_string("bredr"));
                break;
            }
            case DiscoveryFilter::TransportType::LE: {
                dict.dict_append(SimpleDBus::Holder::Type::STRING, "Transport",
                                 SimpleDBus::Holder::create_string("le"));
                break;
            }
        }
        properties.array_append(dict);
    }

    if (!filter.DuplicateData) {
        SimpleDBus::Holder dict = SimpleDBus::Holder::create_dict();
        dict.dict_append(SimpleDBus::Holder::Type::STRING, "DuplicateData",
                               SimpleDBus::Holder::create_boolean(false));
        properties.array_append(dict);
    }

    if (filter.Discoverable) {
        SimpleDBus::Holder dict = SimpleDBus::Holder::create_dict();
        dict.dict_append(SimpleDBus::Holder::Type::STRING, "Discoverable",
                               SimpleDBus::Holder::create_boolean(false));
        properties.array_append(dict);
    }

    if (filter.Pattern.size() > 0) {
        SimpleDBus::Holder dict = SimpleDBus::Holder::create_dict();
        dict.dict_append(SimpleDBus::Holder::Type::STRING, "Pattern",
                               SimpleDBus::Holder::create_string(filter.Pattern));
        properties.array_append(dict);
    }

    auto msg = create_method_call("SetDiscoveryFilter");
    msg.append_argument(properties, "a{sv}");
    _conn->send_with_reply(msg);
}

void Adapter1::RemoveDevice(std::string device_path) {
    auto msg = create_method_call("RemoveDevice");
    msg.append_argument(SimpleDBus::Holder::create_object_path(device_path), "o");
    _conn->send_with_reply(msg);
}
