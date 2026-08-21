#pragma once

#include "../common/BackendBase.h"
#include "../common/BackendUtils.h"
#include "types/android/content/Context.h"

namespace SimpleBLE {

class AdapterAndroid;

class BackendAndroid : public BackendSingleton<BackendAndroid> {
  public:
    BackendAndroid(buildToken);
    virtual ~BackendAndroid() = default;

    virtual std::vector<std::shared_ptr<AdapterBase>> adapters() override;
    virtual bool bluetooth_enabled() override;
    virtual std::string identifier() const noexcept override;
    virtual bool is_active() override { return true; }

    static void set_application_context(jobject context);
    static Android::Context application_context();

  private:
    // Android devices only have a single Bluetooth adapter, so in order to preserve
    // state across multiple instances, a single Adapter object is shared across
    // all users of this backend.
    std::shared_ptr<AdapterAndroid> _adapter;
};

}  // namespace SimpleBLE
