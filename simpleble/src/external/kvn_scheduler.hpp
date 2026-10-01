/*
 * SPDX-FileCopyrightText: 2026 Kevin Dewald <kevin@dewald.me>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef KVN_SCHEDULER_HPP
#define KVN_SCHEDULER_HPP

#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <thread>
#include <utility>

namespace kvn {

/**
 * Runs functions at their scheduled time on a dedicated thread.
 *
 * Functions never run before their time. Functions due at the same time run
 * in the order they were scheduled. The scheduler may be destroyed from one of
 * its own functions.
 */
class scheduler {
  public:
    using clock = std::chrono::steady_clock;

    scheduler() : _state(std::make_shared<state>()) {
        _thread = std::thread(&scheduler::thread_func, _state);
        _thread_id = _thread.get_id();
    }

    // Remove copy constructor and copy assignment
    scheduler(const scheduler&) = delete;
    scheduler& operator=(const scheduler&) = delete;

    virtual ~scheduler() {
        stop();
        // Destroyed from one of its own functions: the thread cannot join itself. It keeps its
        // own reference to the state and ends once the current function returns.
        if (_thread.joinable()) {
            _thread.detach();
        }
    }

    /**
     * Functions scheduled after stop() are dropped.
     */
    void schedule_at(clock::time_point when, std::function<void()> func) {
        {
            std::lock_guard<std::mutex> lock(_state->mutex);
            if (_state->stop) return;
            _state->queue.emplace(std::make_pair(when, _state->next_sequence++), std::move(func));
        }
        _state->cv.notify_one();
    }

    void schedule_after(clock::duration delay, std::function<void()> func) {
        schedule_at(clock::now() + delay, std::move(func));
    }

    /**
     * Run a function as soon as possible, after the ones already due.
     */
    void enqueue(std::function<void()> func) { schedule_at(clock::now(), std::move(func)); }

    /**
     * Drop the pending functions and end the thread. A function that is
     * already running finishes first.
     *
     * Safe to call more than once. When called from one of the scheduler's own
     * functions, it returns right away and the thread ends after that function.
     */
    void stop() {
        // Destroyed outside the lock, in case a function's captures schedule from their destructors.
        decltype(state::queue) dropped;
        {
            std::lock_guard<std::mutex> lock(_state->mutex);
            _state->stop = true;
            dropped.swap(_state->queue);
        }
        _state->cv.notify_one();
        dropped.clear();

        if (std::this_thread::get_id() == _thread_id) return;

        std::lock_guard<std::mutex> lock(_join_mutex);
        if (_thread.joinable()) {
            _thread.join();
        }
    }

  protected:
    /** Shared with the thread, so the thread never touches the scheduler itself. */
    struct state {
        std::mutex mutex;
        std::condition_variable cv;
        std::map<std::pair<clock::time_point, uint64_t>, std::function<void()>> queue;
        uint64_t next_sequence = 0;
        bool stop = false;
    };

    static void thread_func(std::shared_ptr<state> shared) {
        std::unique_lock<std::mutex> lock(shared->mutex);
        while (!shared->stop) {
            if (shared->queue.empty()) {
                shared->cv.wait(lock);
                continue;
            }

            // Re-evaluated after every wake-up: a new function may be due sooner, and
            // wake-ups can be spurious.
            const clock::time_point next = shared->queue.begin()->first.first;
            if (next > clock::now()) {
                shared->cv.wait_until(lock, next);
                continue;
            }

            {
                // The function and its captures are released before the lock is taken again.
                auto node = shared->queue.extract(shared->queue.begin());
                lock.unlock();
                node.mapped()();
            }
            lock.lock();
        }
    }

    std::shared_ptr<state> _state;
    std::thread _thread;
    std::thread::id _thread_id;
    std::mutex _join_mutex;
};

}  // namespace kvn

#endif  // KVN_SCHEDULER_HPP
