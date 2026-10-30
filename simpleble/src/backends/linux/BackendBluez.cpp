#include "BackendBluez.h"

#include "AdapterLinux.h"
#include "CommonUtils.h"

#include <simpleble/Config.h>
#include <simplebluez/Config.h>

#include <algorithm>

namespace SimpleBLE {

std::shared_ptr<BackendBase> BACKEND_LINUX() { return BackendBluez::get(); }

BackendBluez::BackendBluez(buildToken) {
    static std::mutex get_mutex;       // Static mutex to ensure thread safety when accessing the logger
    std::scoped_lock lock(get_mutex);  // Unlock the mutex on function return

    SimpleBluez::Config::use_system_bus = Config::SimpleBluez::use_system_bus;

    bluez.init();
    agent = bluez.root_custom()->agent_add("default");
    agent->set_capabilities(Config::SimpleBluez::interactive_pairing ? SimpleBluez::Agent::KeyboardDisplay
                                                                     : SimpleBluez::Agent::NoInputNoOutput);

    // NOTE: Agent1 rejects passkeys above 999999.
    agent->set_on_request_passkey([this](const std::string& device_path) -> uint32_t {
        BluetoothAddress peer;
        auto adapter = _adapter_for_device(device_path, peer);
        if (!adapter) return 1000000;
        return adapter->agent_request_passkey(peer).value_or(1000000);
    });
    agent->set_on_display_passkey([this](const std::string& device_path, uint32_t passkey, uint16_t entered) {
        // NOTE: BlueZ calls again for every key the peer enters, so only the first call shows the passkey.
        if (entered != 0) return;

        BluetoothAddress peer;
        auto adapter = _adapter_for_device(device_path, peer);
        if (adapter) {
            adapter->agent_display_passkey(peer, passkey);
        }
    });
    agent->set_on_request_confirmation([this](const std::string& device_path, uint32_t passkey) {
        BluetoothAddress peer;
        auto adapter = _adapter_for_device(device_path, peer);
        return adapter && adapter->agent_request_confirmation(peer, passkey);
    });
    agent->set_on_request_authorization([this](const std::string& device_path) {
        BluetoothAddress peer;
        auto adapter = _adapter_for_device(device_path, peer);
        return !adapter || adapter->agent_request_authorization(peer);
    });

    async_thread_active = true;
    async_thread = std::thread(&BackendBluez::async_thread_function, this);
    SAFE_RUN({
        bluez.register_agent(agent);
        // NOTE: BlueZ sends incoming pairing to its default agent, which is the first one registered unless asked.
        if (Config::SimpleBluez::interactive_pairing) {
            bluez.request_default_agent(agent);
        }
    });
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

bool BackendBluez::is_active() { return true; }

std::shared_ptr<AdapterLinux> BackendBluez::_adapter_for_device(const std::string& device_path,
                                                               BluetoothAddress& peer) {
    // NOTE: BlueZ names device objects <adapter path>/dev_XX_XX_XX_XX_XX_XX.
    const auto separator = device_path.rfind("/dev_");
    if (separator == std::string::npos) return nullptr;

    peer = device_path.substr(separator + 5);
    std::replace(peer.begin(), peer.end(), '_', ':');

    std::scoped_lock lock(adapters_mutex_);
    auto it = adapters_.find(device_path.substr(0, separator));
    if (it == adapters_.end()) return nullptr;
    return it->second;
}

std::string BackendBluez::identifier() const noexcept { return "SimpleBluez"; }

void BackendBluez::async_thread_function() {
    while (async_thread_active) {
        SAFE_RUN({ bluez.run_async(); });
        std::this_thread::sleep_for(std::chrono::microseconds(100));
    }
}

}  // namespace SimpleBLE
