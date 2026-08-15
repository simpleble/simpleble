#pragma once

#include "../common/BackendBase.h"
#include "../common/BackendUtils.h"
#include "CommonUtils.h"

#include <simplebluez/Bluez.h>

#include <atomic>
#include <map>
#include <memory>
#include <mutex>
#include <thread>

namespace SimpleBLE {

class AdapterLinux;

class BackendBluez : public BackendSingleton<BackendBluez> {
  public:
    BackendBluez(buildToken);
    virtual ~BackendBluez();

    SimpleBluez::Bluez bluez;

    virtual SharedPtrVector<AdapterBase> adapters() override;
    virtual bool bluetooth_enabled() override;
    std::string identifier() const noexcept override;
    virtual bool is_active() override;

  private:
    std::thread async_thread;
    std::atomic_bool async_thread_active = false;
    std::shared_ptr<SimpleBluez::Agent> agent;
    void async_thread_function();

    std::map<std::string, std::shared_ptr<AdapterLinux>> adapters_;
    std::mutex adapters_mutex_;
};

}  // namespace SimpleBLE
