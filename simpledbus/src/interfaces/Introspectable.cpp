#include <simpledbus/advanced/Proxy.h>
#include <simpledbus/base/Path.h>
#include <simpledbus/interfaces/Introspectable.h>

#include <sstream>
#include <vector>

using namespace SimpleDBus;
using namespace SimpleDBus::Interfaces;

const AutoRegisterInterface<Introspectable> Introspectable::registry{
    "org.freedesktop.DBus.Introspectable",
    // clang-format off
    [](std::shared_ptr<Connection> conn, std::shared_ptr<Proxy> proxy) -> std::shared_ptr<Interface> {
        return std::static_pointer_cast<Interface>(std::make_shared<Introspectable>(conn, proxy));
    }
    // clang-format on
};

Introspectable::Introspectable(std::shared_ptr<Connection> conn, std::shared_ptr<Proxy> proxy)
    : Interface(conn, proxy, "org.freedesktop.DBus.Introspectable") {}

// IMPORTANT: The destructor is defined here (instead of inline) to anchor the vtable to this object file.
// This prevents the linker from stripping this translation unit and ensures the static 'registry' variable is
// initialized at startup.
Introspectable::~Introspectable() = default;

std::string Introspectable::Introspect() {
    std::ostringstream xml;

    xml << "<!DOCTYPE node PUBLIC \"-//freedesktop//DTD D-BUS Object Introspection 1.0//EN\"\n";
    xml << " \"http://www.freedesktop.org/standards/dbus/1.0/introspect.dtd\">\n";
    xml << "<node name=\"" << _path << "\">\n";

    for (const auto& [interface_name, interface] : proxy()->interfaces()) {
        if (interface->is_loaded()) {
            if (interface_name == _interface_name) {
                xml << "  <interface name=\"" << interface_name << "\">\n";
                xml << "    <method name=\"Introspect\">\n";
                xml << "      <arg name=\"xml_data\" type=\"s\" direction=\"out\"/>\n";
                xml << "    </method>\n";
                xml << "  </interface>\n";
            } else {
                xml << "  <interface name=\"" << interface_name << "\"/>\n";
            }
        }
    }

    for (const auto& [child_path, child] : proxy()->children()) {
        const std::vector<std::string> child_elements = PathUtils::split_elements(child_path);
        if (!child_elements.empty()) {
            xml << "  <node name=\"" << child_elements.back() << "\"/>\n";
        }
    }

    xml << "</node>\n";

    return xml.str();
}

void Introspectable::message_handle(Message& msg) {
    if (msg.is_method_call(_interface_name, "Introspect")) {
        Message reply = Message::create_method_return(msg);
        reply.append_argument(Holder::create<std::string>(Introspect()), "s");
        _conn->send(reply);
    }
}
