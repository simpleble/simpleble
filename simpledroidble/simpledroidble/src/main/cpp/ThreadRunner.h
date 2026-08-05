#pragma once

#include <simpleble/Logging.h>

#include <condition_variable>
#include <functional>
#include <mutex>
#include <optional>
#include <queue>
#include <string>
#include <thread>

#include "simplejni/VM.hpp"

class ThreadRunner {
  public:
    ThreadRunner() = default;

    ~ThreadRunner() {
        {
            std::unique_lock<std::mutex> lock(_mutex);
            _stop = true;
            _cv.notify_one();
        }
        if (_thread && _thread->joinable()) {
            _thread->join();
        }
    }

    void enqueue(std::function<void()> func) {
        {
            std::unique_lock<std::mutex> lock(_mutex);
            if (!_thread) {
                // Thread is lazily started upon the first task enqueue
                _thread.emplace(&ThreadRunner::threadFunc, this);
            }
            _queue.push(std::move(func));
        }
        _cv.notify_one();
    }

  private:
    static void log_error(const std::string& message) noexcept {
        SimpleBLE::Logging::Logger::get()->log(SimpleBLE::Logging::Level::Error, "SimpleDroidBLE", __FILE__, __LINE__,
                                               __func__, message);
    }

    void threadFunc() noexcept {
        try {
            SimpleJNI::VM::attach();
        } catch (const std::exception& exception) {
            log_error(std::string("Failed to attach callback thread: ") + exception.what());
            return;
        }

        // Run the thread loop
        while (true) {
            std::function<void()> func;

            // Retrieve and execute the next task
            {
                std::unique_lock<std::mutex> lock(_mutex);
                _cv.wait(lock, [this] { return _stop || !_queue.empty(); });
                if (_stop && _queue.empty()) {
                    break;
                }
                func = std::move(_queue.front());
                _queue.pop();
            }

            try {
                func();
            } catch (const std::exception& e) {
                log_error(std::string("Exception in callback thread: ") + e.what());
            } catch (...) {
                log_error("Unknown exception in callback thread");
            }
        }

        SimpleJNI::VM::detach();
    }

    std::optional<std::thread> _thread;
    std::mutex _mutex;
    std::condition_variable _cv;
    std::queue<std::function<void()>> _queue;
    bool _stop = false;
};
