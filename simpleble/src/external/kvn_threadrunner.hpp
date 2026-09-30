/*
 * SPDX-FileCopyrightText: 2025 Kevin Dewald <kevin@dewald.me>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef KVN_THREADRUNNER_HPP
#define KVN_THREADRUNNER_HPP

#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>

namespace kvn {

/**
 * Runs enqueued functions one at a time, in order, on a dedicated thread.
 *
 * The runner may be destroyed from one of its own functions. The functions
 * still queued at that point are dropped, since they would otherwise run after
 * the runner is gone.
 */
class thread_runner {
  public:
    thread_runner() : _state(std::make_shared<state>()) {
        _thread = std::thread(&thread_runner::thread_func, _state);
        _thread_id = _thread.get_id();
    }

    // Remove copy constructor and copy assignment
    thread_runner(const thread_runner&) = delete;
    thread_runner& operator=(const thread_runner&) = delete;

    virtual ~thread_runner() {
        if (std::this_thread::get_id() != _thread_id) {
            stop();
            return;
        }

        // Destroyed from one of its own functions: the thread cannot join itself. It keeps its
        // own reference to the state and ends once the current function returns.
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
            _state->queue.push(std::move(func));
        }
        _state->cv.notify_one();
    }

    /**
     * Run the functions already enqueued, then end the thread.
     *
     * Safe to call more than once. When called from one of the runner's own
     * functions, it returns right away and the thread ends once the queue is
     * empty.
     */
    void stop() {
        {
            std::lock_guard<std::mutex> lock(_state->mutex);
            _state->stop = true;
        }
        _state->cv.notify_one();

        if (std::this_thread::get_id() == _thread_id) return;

        std::lock_guard<std::mutex> lock(_join_mutex);
        if (_thread.joinable()) {
            _thread.join();
        }
    }

  protected:
    /** Shared with the thread, so the thread never touches the runner itself. */
    struct state {
        std::mutex mutex;
        std::condition_variable cv;
        std::queue<std::function<void()>> queue;
        bool stop = false;
    };

    static void thread_func(std::shared_ptr<state> shared) {
        while (true) {
            std::function<void()> func;
            {
                std::unique_lock<std::mutex> lock(shared->mutex);
                shared->cv.wait(lock, [&shared] { return shared->stop || !shared->queue.empty(); });
                if (shared->stop && shared->queue.empty()) {
                    return;
                }
                func = std::move(shared->queue.front());
                shared->queue.pop();
            }
            func();
        }
    }

    std::shared_ptr<state> _state;
    std::thread _thread;
    std::thread::id _thread_id;
    std::mutex _join_mutex;
};

}  // namespace kvn

#endif  // KVN_THREADRUNNER_HPP
