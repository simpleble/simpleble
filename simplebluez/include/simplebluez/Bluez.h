#pragma once

#include <simpledbus/advanced/Proxy.h>
#include <simpledbus/interfaces/ObjectManager.h>

#include <simplebluez/BluezRoot.h>
#include <simplebluez/Adapter.h>
#include <simplebluez/Agent.h>

#include <vector>

namespace SimpleBluez {

class Bluez {
  public:
    Bluez();
    virtual ~Bluez();

    // Delete copy and move operations
    Bluez(const Bluez&) = delete;
    Bluez& operator=(const Bluez&) = delete;
    Bluez(Bluez&&) = delete;
    Bluez& operator=(Bluez&&) = delete;

    void init();
    void run_async();

    std::vector<std::shared_ptr<Adapter>> get_adapters();
    std::shared_ptr<Agent> get_agent();
    void register_agent();

  private:
    std::shared_ptr<SimpleDBus::Connection> _conn;
    std::shared_ptr<SimpleBluez::BluezRoot> _bluez_root;
};

}  // namespace SimpleBluez
