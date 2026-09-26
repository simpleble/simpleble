#include "../common/BackendBase.h"
#include "../common/BackendUtils.h"
#include "CommonUtils.h"

#include "AdapterMac.h"

namespace SimpleBLE {

class BackendCoreBluetooth : public BackendSingleton<BackendCoreBluetooth> {
  public:
    BackendCoreBluetooth(buildToken);
    virtual ~BackendCoreBluetooth() = default;

    virtual std::vector<std::shared_ptr<AdapterBase>> adapters() override;
    virtual bool bluetooth_enabled() override;
    virtual std::string identifier() const noexcept override;
    virtual bool is_active() override { return true; }
};

std::shared_ptr<BackendBase> BACKEND_MACOS() { return BackendCoreBluetooth::get(); }

// Apple devices have a single Bluetooth adapter, shared by every user of this backend. It is created on first use
// because creating it starts CoreBluetooth, which asks the user for Bluetooth permission.
static std::shared_ptr<AdapterMac> adapter() {
    static auto adapter = std::make_shared<AdapterMac>();
    return adapter;
}

BackendCoreBluetooth::BackendCoreBluetooth(buildToken) {}

SharedPtrVector<AdapterBase> BackendCoreBluetooth::adapters() {
    SharedPtrVector<AdapterBase> adapter_list;
    adapter_list.push_back(adapter());
    return adapter_list;
}

bool BackendCoreBluetooth::bluetooth_enabled() {
    // Because CBCentralManager requires an instance of an object to properly operate,
    // we'll fabricate a local AdapterBase object and query it's internal AdapterBaseMacOS
    // to see if Bluetooth is enabled.
    // TODO: Find a better alternative for this.
    return adapter()->bluetooth_enabled();
}

std::string BackendCoreBluetooth::identifier() const noexcept { return "CoreBluetooth"; }

}  // namespace SimpleBLE
