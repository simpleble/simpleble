#pragma once

#include "../common/BackendBase.h"
#include "../common/BackendUtils.h"

#include "jni/Common.hpp"

namespace SimpleBLE {

class BackendAndroid : public BackendSingleton<BackendAndroid> {
  public:
    BackendAndroid(buildToken);
    virtual ~BackendAndroid() = default;

    virtual std::vector<std::shared_ptr<AdapterBase>> get_adapters() override;
    virtual bool bluetooth_enabled() override;
    virtual std::string name() const noexcept override;

  private:
};

}  // namespace SimpleBLE
