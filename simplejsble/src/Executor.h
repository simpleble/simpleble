#pragma once

#include <condition_variable>
#include <cstddef>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>

namespace SimpleJSBLE {

// Runs blocking SimpleBLE calls away from the JavaScript thread. Node's own
// thread pool is shared with fs, dns and crypto and has four threads by default,
// while a single connect can block for seconds, so the addon owns its threads.
class Executor {
  public:
    using Task = std::function<void()>;

    // Tasks posted to the same strand run one at a time, in posting order.
    class Strand {
      private:
        friend class Executor;
        std::mutex mutex_;
        std::deque<Task> tasks_;
        bool running_ = false;
    };

    // NOTE: The executor is never destroyed, so worker threads blocked in a
    // native call at process exit don't hold up static destruction.
    static Executor& get();

    void post(Task task);
    void post(const std::shared_ptr<Strand>& strand, Task task);

  private:
    Executor() = default;

    void worker(bool permanent);
    static void run_strand(const std::shared_ptr<Strand>& strand);

    std::mutex mutex_;
    std::condition_variable condition_;
    std::deque<Task> tasks_;
    std::size_t threads_ = 0;
    std::size_t idle_ = 0;
};

}  // namespace SimpleJSBLE
