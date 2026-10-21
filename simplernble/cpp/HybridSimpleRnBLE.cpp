#include "HybridSimpleRnBLE.hpp"

#include <simpleble/Config.h>
#include <simpleble/Logging.h>

#include <atomic>
#include <stdexcept>
#include <iterator>
#include <utility>

#include "HybridAdapterHandle.hpp"
#include "HybridBackendHandle.hpp"
#include "Logging.hpp"

namespace margelo::nitro::simplernble {

namespace {

using Priority = SimpleBLE::Config::Android::ConnectionPriorityRequest;
using SimpleBLE::Logging::Level;
using SimpleBLE::Logging::Logger;

const std::pair<ConnectionPriorityRequest, Priority> PRIORITIES[] = {
    {ConnectionPriorityRequest::DISABLED, Priority::DISABLED},
    {ConnectionPriorityRequest::BALANCED, Priority::BALANCED},
    {ConnectionPriorityRequest::HIGH, Priority::HIGH},
    {ConnectionPriorityRequest::LOWPOWER, Priority::LOW_POWER},
    {ConnectionPriorityRequest::DCK, Priority::DCK},
};

// NOTE: JavaScript numbers the levels as SimpleBLE does, from 0 (none) to 6 (verbose).
double to_number(Level level) { return static_cast<double>(level); }

Level to_level(double number) {
    if (!(number >= static_cast<int>(Level::None) && number <= static_cast<int>(Level::Verbose))) {
        throw std::invalid_argument("Unknown log level");
    }
    return static_cast<Level>(static_cast<int>(number));
}

template <typename To, typename From, size_t N>
To convert(const std::pair<From, To> (&table)[N], From value) {
    for (const auto& [from, to] : table) {
        if (from == value) return to;
    }
    return table[0].second;
}

template <typename To, typename From, size_t N>
To convert_back(const std::pair<To, From> (&table)[N], From value) {
    for (const auto& [to, from] : table) {
        if (from == value) return to;
    }
    return table[0].first;
}

// The module object whose JavaScript callback receives SimpleBLE's log, if any.
std::atomic<const HybridSimpleRnBLE*> log_owner{nullptr};

}  // namespace

// NOTE: SimpleBLE's log outlives the JavaScript runtime, e.g. across a reload,
// so it goes back to the system's when the runtime that took it goes away.
HybridSimpleRnBLE::~HybridSimpleRnBLE() {
    const HybridSimpleRnBLE* self = this;
    if (log_owner.compare_exchange_strong(self, nullptr)) log_to_system();
}

std::shared_ptr<Promise<std::vector<std::shared_ptr<HybridBackendHandleSpec>>>> HybridSimpleRnBLE::getBackends() {
    return run<std::vector<std::shared_ptr<HybridBackendHandleSpec>>>(lifecycle(), [] {
        std::vector<std::shared_ptr<HybridBackendHandleSpec>> handles;
        for (auto& backend : SimpleBLE::Backend::get_backends()) handles.push_back(intern<HybridBackendHandle>(backend));
        return handles;
    });
}

std::shared_ptr<Promise<std::shared_ptr<HybridBackendHandleSpec>>> HybridSimpleRnBLE::getNativeBackend() {
    return run<std::shared_ptr<HybridBackendHandleSpec>>(lifecycle(), [] {
        return std::static_pointer_cast<HybridBackendHandleSpec>(intern<HybridBackendHandle>(SimpleBLE::Backend::get_native()));
    });
}

std::shared_ptr<Promise<std::vector<std::shared_ptr<HybridAdapterHandleSpec>>>> HybridSimpleRnBLE::getAdapters() {
    return run<std::vector<std::shared_ptr<HybridAdapterHandleSpec>>>(lifecycle(), [] {
        std::vector<std::shared_ptr<HybridAdapterHandleSpec>> handles;
        for (auto& adapter : SimpleBLE::Adapter::get_adapters()) handles.push_back(intern<HybridAdapterHandle>(adapter));
        return handles;
    });
}

std::shared_ptr<Promise<bool>> HybridSimpleRnBLE::bluetoothEnabled() {
    return run<bool>(lifecycle(), [] { return SimpleBLE::Adapter::bluetooth_enabled(); });
}

bool HybridSimpleRnBLE::getNegotiateMtu() { return SimpleBLE::Config::Android::negotiate_mtu; }

void HybridSimpleRnBLE::setNegotiateMtu(bool enabled) { SimpleBLE::Config::Android::negotiate_mtu = enabled; }

ConnectionPriorityRequest HybridSimpleRnBLE::getConnectionPriorityRequest() {
    return convert_back(PRIORITIES, SimpleBLE::Config::Android::connection_priority_request);
}

void HybridSimpleRnBLE::setConnectionPriorityRequest(ConnectionPriorityRequest request) {
    SimpleBLE::Config::Android::connection_priority_request = convert(PRIORITIES, request);
}

void HybridSimpleRnBLE::resetConfig() { SimpleBLE::Config::Base::reset_all(); }

double HybridSimpleRnBLE::getLogLevel() { return to_number(Logger::get()->get_level()); }

void HybridSimpleRnBLE::setLogLevel(double level) { Logger::get()->set_level(to_level(level)); }

// NOTE: SimpleBLE logs from any thread, holding its logger's lock while it
// calls the callback, so the callback only queues the entry for the JavaScript
// thread.
void HybridSimpleRnBLE::setLogCallback(const std::optional<std::function<void(const LogEntry&)>>& callback) {
    if (!callback) {
        Logger::get()->set_callback(nullptr);
        log_owner = nullptr;
        return;
    }
    auto deliver = *callback;
    Logger::get()->set_callback([deliver](Level level, const std::string& module, const std::string& file, uint32_t line,
                                          const std::string& function, const std::string& message) {
        deliver(LogEntry(to_number(level), module, file, static_cast<double>(line), function, message));
    });
    log_owner = this;
}

void HybridSimpleRnBLE::logToSystem() {
    log_to_system();
    log_owner = nullptr;
}

void HybridSimpleRnBLE::logToFile(const std::optional<std::string>& path) {
    if (path) {
        Logger::get()->log_default_file(*path);
    } else {
        Logger::get()->log_default_file();
    }
    log_owner = nullptr;
}

}  // namespace margelo::nitro::simplernble

