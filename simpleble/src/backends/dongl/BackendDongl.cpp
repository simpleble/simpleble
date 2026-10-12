#include <map>
#include <memory>
#include <mutex>
#include <string>
#include "AdapterDongl.h"
#include "BackendUtils.h"
#include "CommonUtils.h"
#include "LoggingInternal.h"

#include "usb/UsbHelper.h"
#include <simpleble/Config.h>
#include <simpleble/Exceptions.h>
#include <fmt/core.h>

namespace SimpleBLE {

class BackendDongl : public BackendSingleton<BackendDongl> {
  public:
    BackendDongl(buildToken) {};
    virtual ~BackendDongl() = default;

    virtual SharedPtrVector<AdapterBase> adapters() override;
    virtual bool bluetooth_enabled() override;
    std::string identifier() const noexcept override;
    virtual bool is_active() override;

  private:
    // A Dongl can only be opened once, so the adapter of each port is reused for as long as anyone holds it.
    std::mutex adapters_mutex_;
    std::map<std::string, std::weak_ptr<AdapterDongl>> adapters_;
};

std::shared_ptr<BackendBase> BACKEND_DONGL() { return BackendDongl::get(); }

std::string BackendDongl::identifier() const noexcept { return "Dongl"; }

bool BackendDongl::bluetooth_enabled() { return true; }

bool BackendDongl::is_active() { return Config::Dongl::use_dongl_backend; }

SharedPtrVector<AdapterBase> BackendDongl::adapters() {
    SharedPtrVector<AdapterBase> adapters;
    std::scoped_lock lock(adapters_mutex_);
    for (const auto& device_path : Dongl::USB::UsbHelper::get_dongl_devices()) {
        auto adapter = adapters_[device_path].lock();
        if (!adapter) {
            try {
                adapter = std::make_shared<AdapterDongl>(device_path);
            } catch (const Exception::AdapterInUse&) {
                // Skip Dongls held by other processes, so that the rest remain available.
                SIMPLEBLE_LOG_WARN(fmt::format("Skipping Dongl in use by another process: {}", device_path));
                continue;
            } catch (const std::exception& e) {
                SIMPLEBLE_LOG_ERROR(fmt::format("Failed to open Dongl {}: {}", device_path, e.what()));
                throw;
            }
            adapters_[device_path] = adapter;
        }
        adapters.push_back(adapter);
    }
    return adapters;
}

}  // namespace SimpleBLE
