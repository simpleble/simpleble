#pragma once

#include <winrt/base.h>
#include <atomic>
#include <condition_variable>
#include <functional>
#include <future>
#include <mutex>
#include <queue>
#include <thread>

class MtaManager {
public:
    static MtaManager& get() {
        static MtaManager instance;
        return instance;
    }

    void submit_task(std::function<void()> task);
    void stop();

    template<typename T>
    T execute_sync(std::function<T()> task) {
        std::promise<T> result_promise;
        auto result_future = result_promise.get_future();
        submit_task([&result_promise, task]() {
            try {
                T result = task();
                result_promise.set_value(result);
            } catch (...) {
                result_promise.set_exception(std::current_exception());
            }
        });
        return result_future.get();
    }

    void execute_sync(std::function<void()> task) {
        std::promise<void> result_promise;
        auto result_future = result_promise.get_future();
        submit_task([&result_promise, task]() {
            try {
                task();
                result_promise.set_value();
            } catch (...) {
                result_promise.set_exception(std::current_exception());
            }
        });
        result_future.get();
    }

private:
    MtaManager();
    ~MtaManager();
    MtaManager(const MtaManager&) = delete;
    MtaManager& operator=(const MtaManager&) = delete;

    std::thread mta_thread_;
    std::mutex task_mutex_;
    std::condition_variable task_cv_;
    std::queue<std::function<void()>> task_queue_;
    std::atomic<bool> running_{true};

    void mta_thread_func();
};