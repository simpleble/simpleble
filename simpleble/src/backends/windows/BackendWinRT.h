#pragma once

#include "../common/BackendBase.h"
#include "../common/BackendUtils.h"

namespace SimpleBLE {

class BackendWinRT : public BackendSingleton<BackendWinRT> {
  public:
    BackendWinRT(buildToken);
    virtual ~BackendWinRT() = default;

    virtual std::vector<std::shared_ptr<AdapterBase>> adapters() override;
    virtual bool bluetooth_enabled() override;
    virtual std::string identifier() const noexcept override;
    virtual bool is_active() override { return true; }
};

}  // namespace SimpleBLE
