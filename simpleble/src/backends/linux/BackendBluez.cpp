#include "AdapterLinux.h"
#include "BackendBase.h"
#include "BackendUtils.h"
#include "CommonUtils.h"

#include <simplebluez/Bluez.h>
#include <simpleble/Config.h>
#include <simplebluez/Config.h>

#include <atomic>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

namespace SimpleBLE {

class BackendBluez : public BackendSingleton<BackendBluez> {
  public:
    BackendBluez(buildToken);
    virtual ~BackendBluez();

    SimpleBluez::Bluez bluez;

    virtual SharedPtrVector<AdapterBase> adapters() override;
    virtual bool bluetooth_enabled() override;
    std::string identifier() const noexcept override;
    virtual bool is_active() override;

  private:
    std::thread async_thread;
    std::atomic_bool async_thread_active = false;
    std::shared_ptr<SimpleBluez::Agent> agent;
    void async_thread_function();

    std::map<std::string, std::shared_ptr<AdapterLinux>> adapters_;
    std::mutex adapters_mutex_;
};

std::shared_ptr<BackendBase> BACKEND_LINUX() { return BackendBluez::get(); }

BackendBluez::BackendBluez(buildToken) {
    static std::mutex get_mutex;       // Static mutex to ensure thread safety when accessing the logger
    std::scoped_lock lock(get_mutex);  // Unlock the mutex on function return

    SimpleBluez::Config::use_system_bus = Config::SimpleBluez::use_system_bus;

    bluez.init();
    agent = bluez.root_custom()->agent_add("default");

    async_thread_active = true;
    async_thread = std::thread(&BackendBluez::async_thread_function, this);
    SAFE_RUN({ bluez.register_agent(agent); });
}

BackendBluez::~BackendBluez() {
    async_thread_active = false;
    if (async_thread.joinable()) {
        async_thread.join();
    }
}

SharedPtrVector<AdapterBase> BackendBluez::adapters() {
    SharedPtrVector<AdapterBase> adapter_list;

    auto internal_adapters = bluez.get_adapters();
    std::scoped_lock lock(adapters_mutex_);
    for (auto& adapter : internal_adapters) {
        auto path = adapter->path();

        // Reuse the cached wrapper for this adapter if one exists, as creating a
        // second wrapper around the same adapter would clear the existing wrapper's
        // callbacks once it gets destroyed.
        if (adapters_.count(path) == 0) {
            adapters_.insert(std::make_pair(path, std::make_shared<AdapterLinux>(adapter)));
        }

        adapter_list.push_back(adapters_.at(path));
    }
    return adapter_list;
}

bool BackendBluez::bluetooth_enabled() {
    bool enabled = false;

    auto internal_adapters = bluez.get_adapters();
    for (auto& adapter : internal_adapters) {
        if (adapter->powered()) {
            enabled = true;
            break;
        }
    }

    return enabled;
}

bool BackendBluez::is_active() {
    return !Config::SimpleBluez::use_legacy_bluez_backend;
}

std::string BackendBluez::identifier() const noexcept { return "SimpleBluez"; }

void BackendBluez::async_thread_function() {
    while (async_thread_active) {
        SAFE_RUN({ bluez.run_async(); });
        std::this_thread::sleep_for(std::chrono::microseconds(100));
    }
}

}  // namespace SimpleBLE
