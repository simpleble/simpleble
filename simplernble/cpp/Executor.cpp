#include "Executor.hpp"

#include <chrono>
#include <thread>

#if defined(__ANDROID__)
#include <simplejni/VM.hpp>
#endif

namespace margelo::nitro::simplernble {

namespace {
// NOTE: Threads are created on demand and exit after being idle for a while,
// except the first one. The limit only guards against runaway growth; tasks
// queue beyond it.
constexpr std::size_t MAX_THREADS = 128;
constexpr auto IDLE_TIMEOUT = std::chrono::seconds(30);
}  // namespace

Executor& Executor::get() {
    static Executor* executor = [] {
        auto created = new Executor();
        created->threads_ = 1;
        std::thread([created] { created->worker(true); }).detach();
        return created;
    }();
    return *executor;
}

void Executor::post(Task task) {
    std::lock_guard<std::mutex> lock(mutex_);
    tasks_.push_back(std::move(task));

    if (tasks_.size() <= idle_ || threads_ >= MAX_THREADS) {
        condition_.notify_one();
        return;
    }

    try {
        std::thread([this] { worker(false); }).detach();
        threads_++;
    } catch (...) {
        // The permanent thread will get to the task.
        condition_.notify_one();
    }
}

void Executor::post(const std::shared_ptr<Strand>& strand, Task task) {
    {
        std::lock_guard<std::mutex> lock(strand->mutex_);
        strand->tasks_.push_back(std::move(task));
        if (strand->running_) return;
        strand->running_ = true;
    }

    post([strand] { run_strand(strand); });
}

void Executor::run_strand(const std::shared_ptr<Strand>& strand) {
    while (true) {
        Task task;
        {
            std::lock_guard<std::mutex> lock(strand->mutex_);
            if (strand->tasks_.empty()) {
                strand->running_ = false;
                return;
            }
            task = std::move(strand->tasks_.front());
            strand->tasks_.pop_front();
        }
        task();
    }
}

#if defined(__ANDROID__)
// NOTE: SimpleBLE attaches a thread to the JVM on its first JNI call, and ART
// aborts the app when an attached thread exits, so workers attach up front and
// detach before they end.
struct JvmAttachment {
    JvmAttachment() {
        if (SimpleJNI::VM::jvm() != nullptr) SimpleJNI::VM::attach("SimpleRnBLE");
    }
    ~JvmAttachment() {
        if (SimpleJNI::VM::jvm() != nullptr) SimpleJNI::VM::detach();
    }
};
#else
struct JvmAttachment {};
#endif

void Executor::worker(bool permanent) {
    JvmAttachment attachment;
    std::unique_lock<std::mutex> lock(mutex_);
    while (true) {
        idle_++;
        if (permanent) {
            condition_.wait(lock, [this] { return !tasks_.empty(); });
        } else if (!condition_.wait_for(lock, IDLE_TIMEOUT, [this] { return !tasks_.empty(); })) {
            idle_--;
            threads_--;
            return;
        }
        idle_--;

        Task task = std::move(tasks_.front());
        tasks_.pop_front();
        lock.unlock();
        task();
        task = nullptr;
        lock.lock();
    }
}

}  // namespace margelo::nitro::simplernble
