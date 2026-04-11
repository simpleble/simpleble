#include <string>
#include "AdapterDongl.h"
#include "BackendUtils.h"
#include "CommonUtils.h"

#include "usb/UsbHelper.h"
#include <simpleble/Config.h>
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
};

std::shared_ptr<BackendBase> BACKEND_DONGL() { return BackendDongl::get(); }

std::string BackendDongl::identifier() const noexcept { return "Dongl"; }

bool BackendDongl::bluetooth_enabled() { return true; }

bool BackendDongl::is_active() { return Config::Dongl::use_dongl_backend; }

SharedPtrVector<AdapterBase> BackendDongl::adapters() {
    SharedPtrVector<AdapterBase> adapters;
    for (const auto& device_path : Dongl::USB::UsbHelper::get_dongl_devices()) {
        adapters.push_back(std::make_shared<AdapterDongl>(device_path));
    }
    return adapters;
}

}  // namespace SimpleBLE
