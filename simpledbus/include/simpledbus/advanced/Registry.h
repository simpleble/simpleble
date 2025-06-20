#pragma once

#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <typeindex>

#include <simpledbus/base/Connection.h>
#include <simpledbus/base/Holder.h>

#include <fmt/core.h>

namespace SimpleDBus {

class Interface;

using CreatorFunction = std::shared_ptr<Interface> (*)(std::shared_ptr<Connection>, const std::string&,
                                                       const std::string&, const Holder&);

class Registry {
  public:
    static Registry& getInstance() {
        static Registry instance;
        return instance;
    }

    template <typename T>
    void registerClass(const std::string& key, CreatorFunction creator) {
        static_assert(std::is_base_of<Interface, T>::value, "T must inherit from Interface");
        creators[key] = creator;
    }

    // NOTES; We need a method inside Interfaces that will automatically retrieve the Interface name for the class.
    std::shared_ptr<Interface> create(const std::string& key, std::shared_ptr<Connection> conn,
                                      const std::string& bus_name, const std::string& path,
                                      const Holder& options) const {
        auto it = creators.find(key);
        if (it != creators.end()) {
            auto iface = it->second(conn, bus_name, path, options);
            iface->load(options);
            return iface;
        }
        return nullptr;
    }

  private:
    std::unordered_map<std::string, CreatorFunction> creators;
    Registry() = default;
};

template <typename T>
struct AutoRegister {
    AutoRegister(const std::string& key, CreatorFunction creator) {
        static_assert(std::is_base_of<Interface, T>::value, "T must inherit from Interface");
        Registry::getInstance().registerClass<T>(key, creator);
        fmt::print("Registered class with key {}\n", key);
    }
};

}  // namespace SimpleDBus
