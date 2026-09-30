#pragma once

#include <simpleble/Logging.h>

#include <condition_variable>
#include <exception>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <utility>

#include "simplejni/Common.hpp"
#include "simplejni/VM.hpp"

/**
 * Runs Java callbacks one at a time, in order, on a single thread attached to the JVM.
 *
 * Follows kvn::thread_runner: stop() runs the queued functions, functions enqueued afterwards are dropped, and the
 * runner may be destroyed from one of its own functions. On top of that, it does what a JNI callback thread needs:
 * - The thread starts on the first enqueue, since the JavaVM is only known after JNI_OnLoad.
 * - The thread stays attached as "SimpleDroidBLE", the name debuggers and ANR traces show, and detaches before it ends,
 *   as ART requires.
 * - A function that throws, or that leaves a Java exception pending, is logged and the thread carries on.
 */
class ThreadRunner final {
  public:
    ThreadRunner() : _state(std::make_shared<state>()) {}

    // Remove copy constructor and copy assignment
    ThreadRunner(const ThreadRunner&) = delete;
    ThreadRunner& operator=(const ThreadRunner&) = delete;

    ~ThreadRunner() {
        if (!on_own_thread()) {
            stop();
            return;
        }

        // Destroyed from one of its own functions: the thread cannot join itself. It keeps its own reference to the
        // state, and detaches from the JVM and ends once the current function returns.
        std::queue<std::function<void()>> dropped;
        {
            std::lock_guard<std::mutex> lock(_state->mutex);
            _state->stop = true;
            dropped.swap(_state->queue);
        }
        _state->cv.notify_one();
        _thread.detach();
    }

    /**
     * Functions enqueued after stop() are dropped.
     */
    void enqueue(std::function<void()> func) {
        {
            std::lock_guard<std::mutex> lock(_state->mutex);
            if (_state->stop) return;
            if (!_thread.joinable()) {
                _thread = std::thread(&ThreadRunner::thread_func, _state);
                _thread_id = _thread.get_id();
            }
            _state->queue.push(std::move(func));
        }
        _state->cv.notify_one();
    }

    /**
     * Run the functions already enqueued, then end the thread.
     *
     * Safe to call more than once. When called from one of the runner's own functions, it returns right away and the
     * thread ends once the queue is empty.
     */
    void stop() {
        {
            std::lock_guard<std::mutex> lock(_state->mutex);
            _state->stop = true;
        }
        _state->cv.notify_one();

        // Once stopped, enqueue() no longer starts the thread, so it is safe to read here.
        if (on_own_thread()) return;

        std::lock_guard<std::mutex> lock(_join_mutex);
        if (_thread.joinable()) {
            _thread.join();
        }
    }

  private:
    /** Shared with the thread, so the thread never touches the runner itself. */
    struct state {
        std::mutex mutex;
        std::condition_variable cv;
        std::queue<std::function<void()>> queue;
        bool stop = false;
    };

    static void log_error(const std::string& message) noexcept {
        SimpleBLE::Logging::Logger::get()->log(SimpleBLE::Logging::Level::Error, "SimpleDroidBLE", __FILE__, __LINE__,
                                               __func__, message);
    }

    static void thread_func(std::shared_ptr<state> shared) noexcept {
        try {
            SimpleJNI::VM::attach("SimpleDroidBLE");
        } catch (const std::exception& exception) {
            log_error(std::string("Failed to attach callback thread: ") + exception.what());

            // Nothing can run without the JVM, so drop the queued functions instead of letting the queue grow.
            std::queue<std::function<void()>> dropped;
            std::lock_guard<std::mutex> lock(shared->mutex);
            shared->stop = true;
            dropped.swap(shared->queue);
            return;
        }

        while (true) {
            std::function<void()> func;
            {
                std::unique_lock<std::mutex> lock(shared->mutex);
                shared->cv.wait(lock, [&shared] { return shared->stop || !shared->queue.empty(); });
                if (shared->stop && shared->queue.empty()) {
                    break;
                }
                func = std::move(shared->queue.front());
                shared->queue.pop();
            }
            run(func);
        }

        SimpleJNI::VM::detach();
    }

    static void run(const std::function<void()>& func) noexcept {
        try {
            func();
        } catch (const std::exception& exception) {
            log_error(std::string("Exception in callback thread: ") + exception.what());
        } catch (...) {
            log_error("Unknown exception in callback thread");
        }

        // A pending Java exception would break every later JNI call on this thread.
        try {
            SimpleJNI::Exception::check(SimpleJNI::VM::env());
        } catch (const std::exception& exception) {
            log_error(std::string("Java exception left pending in callback thread: ") + exception.what());
        }
    }

    bool on_own_thread() const { return std::this_thread::get_id() == _thread_id; }

    std::shared_ptr<state> _state;
    std::thread _thread;
    std::thread::id _thread_id;
    std::mutex _join_mutex;
};
