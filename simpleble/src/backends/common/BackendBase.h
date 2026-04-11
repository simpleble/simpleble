#pragma once

#include <memory>
#include <string>
#include <vector>

namespace SimpleBLE {

class AdapterBase;

class BackendBase {
  public:
    virtual ~BackendBase() = default;

    virtual std::vector<std::shared_ptr<AdapterBase>> adapters() = 0;
    virtual bool bluetooth_enabled() = 0;
    virtual std::string identifier() const noexcept = 0;
    virtual bool is_active() = 0;
};

}  // namespace SimpleBLE
