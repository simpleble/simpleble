#include <gtest/gtest.h>

#include <simpledbus/advanced/Proxy.h>
#include <simpledbus/advanced/InterfaceRegistry.h>

using namespace SimpleDBus;

const AutoRegisterInterface<Interface> registry_i1{
    "i.1",
    // clang-format off
    [](std::shared_ptr<Connection> conn, const std::string& bus_name, const std::string& path, const Holder& options) -> std::shared_ptr<SimpleDBus::Interface> {
        return std::make_shared<Interface>(conn, bus_name, path, "i.1");
    }
    // clang-format on
};

const AutoRegisterInterface<Interface> registry_i2{
    "i.2",
    // clang-format off
    [](std::shared_ptr<Connection> conn, const std::string& bus_name, const std::string& path, const Holder& options) -> std::shared_ptr<SimpleDBus::Interface> {
        return std::make_shared<Interface>(conn, bus_name, path, "i.2");
    }
    // clang-format on
};

const AutoRegisterInterface<Interface> registry_i3{
    "i.3",
    // clang-format off
    [](std::shared_ptr<Connection> conn, const std::string& bus_name, const std::string& path, const Holder& options) -> std::shared_ptr<SimpleDBus::Interface> {
        return std::make_shared<Interface>(conn, bus_name, path, "i.3");
    }
    // clang-format on
};

TEST(ProxyInterfaces, LoadInterfaces) {
    Holder managed_interfaces = Holder::create_dict();
    managed_interfaces.dict_append(Holder::STRING, "i.1", Holder());

    Proxy h = Proxy(nullptr, "", "/");
    EXPECT_FALSE(h.interfaces_loaded());

    // TODO: Check that all properties of the interface are correctly loaded.

    h.interfaces_load(managed_interfaces);

    EXPECT_TRUE(h.interfaces_loaded());
    EXPECT_EQ(1, h.interfaces_count());
    EXPECT_TRUE(h.interface_exists("i.1"));
}

TEST(ProxyInterfaces, UnloadInterfaces) {
    Holder managed_interfaces = Holder::create_dict();
    managed_interfaces.dict_append(Holder::STRING, "i.1", Holder());
    managed_interfaces.dict_append(Holder::STRING, "i.2", Holder());
    managed_interfaces.dict_append(Holder::STRING, "i.3", Holder());

    Proxy h = Proxy(nullptr, "", "/");
    h.interfaces_load(managed_interfaces);
    EXPECT_EQ(3, h.interfaces_count());

    Holder removed_interfaces = Holder::create_array();
    removed_interfaces.array_append(Holder::create_string("i.3"));
    h.interfaces_unload(removed_interfaces);

    EXPECT_EQ(2, h.interfaces_count());
    EXPECT_TRUE(h.interfaces_loaded());
    EXPECT_FALSE(h.interface_get("i.3")->is_loaded());

    removed_interfaces = Holder::create_array();
    removed_interfaces.array_append(Holder::create_string("i.2"));
    h.interfaces_unload(removed_interfaces);

    EXPECT_EQ(1, h.interfaces_count());
    EXPECT_TRUE(h.interfaces_loaded());
    EXPECT_FALSE(h.interface_get("i.2")->is_loaded());

    removed_interfaces = Holder::create_array();
    removed_interfaces.array_append(Holder::create_string("i.1"));
    h.interfaces_unload(removed_interfaces);

    EXPECT_EQ(0, h.interfaces_count());
    EXPECT_FALSE(h.interfaces_loaded());
    EXPECT_FALSE(h.interface_get("i.1")->is_loaded());
}

TEST(ProxyInterfaces, ReloadInterfaces) {
    Holder managed_interfaces = Holder::create_dict();
    managed_interfaces.dict_append(Holder::STRING, "i.1", Holder());
    managed_interfaces.dict_append(Holder::STRING, "i.2", Holder());
    managed_interfaces.dict_append(Holder::STRING, "i.3", Holder());

    Proxy h = Proxy(nullptr, "", "/");
    h.interfaces_load(managed_interfaces);
    EXPECT_EQ(3, h.interfaces_count());

    managed_interfaces = Holder::create_dict();
    managed_interfaces.dict_append(Holder::STRING, "i.1", Holder());
    managed_interfaces.dict_append(Holder::STRING, "i.3", Holder());
    h.interfaces_reload(managed_interfaces);

    EXPECT_EQ(2, h.interfaces_count());
}
