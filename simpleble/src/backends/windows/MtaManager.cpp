#include "MtaManager.h"

#include <windows.h>

namespace SimpleBLE {
namespace WinRT {

MtaManager::MtaManager() {
    mta_thread_ = std::thread(&MtaManager::mta_thread_func, this);
}

MtaManager::~MtaManager() {
    // This destructor runs under the CRT exit lock. If COM unloads rometadata.dll
    // while the MTA thread exits, that DLL's teardown can wait for the same lock
    // and deadlock the join below. Keep an already-loaded copy until process exit.
    HMODULE metadata_module = nullptr;
    (void)GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_PIN, L"rometadata.dll", &metadata_module);
    stop();
    if (mta_thread_.joinable()) {
        mta_thread_.join();
    }
}

void MtaManager::submit_task(std::function<void()> task) {
    {
        std::lock_guard<std::mutex> lock(task_mutex_);
        task_queue_.push(std::move(task));
    }
    task_cv_.notify_one();
}

void MtaManager::stop() {
    {
        std::lock_guard<std::mutex> lock(task_mutex_);
        running_ = false;
    }
    task_cv_.notify_one();
}

void MtaManager::mta_thread_func() {
    winrt::init_apartment(winrt::apartment_type::multi_threaded);
    while (running_) {
        std::function<void()> task;
        {
            std::unique_lock<std::mutex> lock(task_mutex_);
            task_cv_.wait(lock, [this] { return !task_queue_.empty() || !running_; });
            if (!running_ && task_queue_.empty()) break;
            task = std::move(task_queue_.front());
            task_queue_.pop();
        }
        if (task) task();
    }
}

}  // namespace WinRT
}  // namespace SimpleBLE
