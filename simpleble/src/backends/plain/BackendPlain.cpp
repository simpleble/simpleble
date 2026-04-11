#include <string>
#include "AdapterPlain.h"
#include "BackendUtils.h"
#include "CommonUtils.h"

namespace SimpleBLE {

class BackendPlain : public BackendSingleton<BackendPlain> {
  public:
    BackendPlain(buildToken) {};
    virtual ~BackendPlain() = default;

    virtual SharedPtrVector<AdapterBase> adapters() override;
    virtual bool bluetooth_enabled() override;
    std::string identifier() const noexcept override;
    virtual bool is_active() override { return true; }
};

std::shared_ptr<BackendBase> BACKEND_PLAIN() { return BackendPlain::get(); }

std::string BackendPlain::identifier() const noexcept { return "Plain"; }

bool BackendPlain::bluetooth_enabled() { return true; }

SharedPtrVector<AdapterBase> BackendPlain::adapters() {
    SharedPtrVector<AdapterBase> adapters;
    adapters.push_back(std::make_shared<AdapterPlain>());
    return adapters;
}

}  // namespace SimpleBLE
