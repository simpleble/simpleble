#include <gtest/gtest.h>

#include <kvn_threadrunner.hpp>

#include <atomic>
#include <chrono>
#include <future>
#include <memory>
#include <thread>

using namespace std::chrono_literals;

namespace {

template <typename Condition>
bool eventually(Condition condition, std::chrono::milliseconds timeout = 2s) {
    auto deadline = std::chrono::steady_clock::now() + timeout;
    while (std::chrono::steady_clock::now() < deadline) {
        if (condition()) return true;
        std::this_thread::sleep_for(2ms);
    }
    return condition();
}

}  // namespace

TEST(ThreadRunner, StopRunsQueuedFunctions) {
    std::atomic<int> ran{0};
    kvn::thread_runner runner;
    runner.enqueue([&]() {
        std::this_thread::sleep_for(20ms);
        ran++;
    });
    runner.enqueue([&]() { ran++; });
    runner.enqueue([&]() { ran++; });

    runner.stop();
    EXPECT_EQ(ran.load(), 3);
}

TEST(ThreadRunner, EnqueueAfterStopIsDropped) {
    std::atomic<bool> ran{false};
    kvn::thread_runner runner;
    runner.stop();
    runner.enqueue([&]() { ran = true; });
    runner.stop();

    std::this_thread::sleep_for(20ms);
    EXPECT_FALSE(ran.load());
}

TEST(ThreadRunner, StopFromOwnFunction) {
    std::atomic<int> ran{0};
    std::promise<void> queued;
    auto all_queued = queued.get_future();
    {
        kvn::thread_runner runner;
        runner.enqueue([&]() {
            all_queued.wait();
            runner.stop();
            ran++;
        });
        runner.enqueue([&]() { ran++; });
        queued.set_value();
    }
    // Functions queued before stop() still ran, and destruction joined the thread.
    EXPECT_EQ(ran.load(), 2);
}

TEST(ThreadRunner, DestroyedFromOwnFunction) {
    std::atomic<bool> finished{false};
    std::atomic<bool> dropped_ran{false};
    std::promise<void> queued;
    auto all_queued = queued.get_future();

    auto runner = std::make_unique<kvn::thread_runner>();
    runner->enqueue([&]() {
        all_queued.wait();
        runner.reset();
        // Still on the runner's thread, after the runner is gone.
        finished = true;
    });
    runner->enqueue([&]() { dropped_ran = true; });
    queued.set_value();

    EXPECT_TRUE(eventually([&]() { return finished.load(); }));
    std::this_thread::sleep_for(20ms);
    // It would have run after the runner was destroyed, so it is dropped.
    EXPECT_FALSE(dropped_ran.load());
}
