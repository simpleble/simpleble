#include <simplebluez/interfaces/AgentManager1.h>

using namespace SimpleBluez;

const SimpleDBus::AutoRegisterInterface<AgentManager1> AgentManager1::registry{
    "org.bluez.AgentManager1",
    // clang-format off
    [](std::shared_ptr<SimpleDBus::Connection> conn, const std::string& bus_name, const std::string& path, const SimpleDBus::Holder& options) -> std::shared_ptr<SimpleDBus::Interface> {
        return std::static_pointer_cast<SimpleDBus::Interface>(std::make_shared<AgentManager1>(conn, path));
    }
    // clang-format on
};

AgentManager1::AgentManager1(std::shared_ptr<SimpleDBus::Connection> conn, std::string path)
    : SimpleDBus::Interface(conn, "org.bluez", path, "org.bluez.AgentManager1") {}

void AgentManager1::RegisterAgent(std::string agent, std::string capability) {
    auto msg = create_method_call("RegisterAgent");
    msg.append_argument(SimpleDBus::Holder::create_object_path(agent), "o");
    msg.append_argument(SimpleDBus::Holder::create_string(capability), "s");
    _conn->send_with_reply_and_block(msg);
}

void AgentManager1::RequestDefaultAgent(std::string agent) {
    auto msg = create_method_call("RequestDefaultAgent");
    msg.append_argument(SimpleDBus::Holder::create_object_path(agent), "o");
    _conn->send_with_reply_and_block(msg);
}

void AgentManager1::UnregisterAgent(std::string agent) {
    auto msg = create_method_call("UnregisterAgent");
    msg.append_argument(SimpleDBus::Holder::create_object_path(agent), "o");
    _conn->send_with_reply_and_block(msg);
}
