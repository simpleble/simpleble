#pragma once

#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <typeindex>

#include <simpledbus/base/Connection.h>
#include <simpledbus/base/Holder.h>
#include <simpledbus/base/Logging.h>

namespace SimpleDBus {

class Proxy;

using ProxyCreatorFunction = std::shared_ptr<Proxy> (*)(std::shared_ptr<Connection>, const std::string&, const std::string&);

class ProxyRegistry {
  public:
    static ProxyRegistry& getInstance() {
        static ProxyRegistry instance;
        return instance;
    }

    template <typename T>
    void registerClass(const std::string& prefix, ProxyCreatorFunction creator) {
        static_assert(std::is_base_of<Proxy, T>::value, "T must inherit from Proxy");
        creators[prefix] = creator;
    }

    bool isRegistered(const std::string& prefix) const {
        return creators.find(prefix) != creators.end();
    }

    std::shared_ptr<Proxy> create(const std::string& prefix, std::shared_ptr<Connection> conn, const std::string& bus_name, const std::string& path) const {
        auto it = creators.find(prefix);
        if (it != creators.end()) {
            auto proxy = it->second(conn, bus_name, path);
            return proxy;
        }

        return std::make_shared<Proxy>(conn, bus_name, path);
    }

  private:
    std::unordered_map<std::string, ProxyCreatorFunction> creators;
    ProxyRegistry() = default;
};

template <typename T>
struct AutoRegisterProxy {
    AutoRegisterProxy(const std::string& prefix, ProxyCreatorFunction creator) {
        static_assert(std::is_base_of<Proxy, T>::value, "T must inherit from Proxy");
        ProxyRegistry::getInstance().registerClass<T>(prefix, creator);
        LOG_DEBUG("Registered class with prefix {}", prefix);
    }
};

}  // namespace SimpleDBus
