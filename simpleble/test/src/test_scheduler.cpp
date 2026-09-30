#include <gtest/gtest.h>

#include <kvn_scheduler.hpp>

#include <atomic>
#include <chrono>
#include <future>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

using namespace std::chrono_literals;
using Clock = kvn::scheduler::clock;

namespace {

template <typename Condition>
bool eventually(Condition condition, std::chrono::milliseconds timeout = 2s) {
    auto deadline = Clock::now() + timeout;
    while (Clock::now() < deadline) {
        if (condition()) return true;
        std::this_thread::sleep_for(2ms);
    }
    return condition();
}

class Recorder {
  public:
    std::function<void()> record(int value) {
        return [this, value]() {
            std::lock_guard<std::mutex> lock(mutex_);
            values_.push_back(value);
        };
    }

    std::vector<int> values() {
        std::lock_guard<std::mutex> lock(mutex_);
        return values_;
    }

  private:
    std::mutex mutex_;
    std::vector<int> values_;
};

}  // namespace

TEST(Scheduler, RunsFunctionsInTimeOrder) {
    Recorder recorder;
    kvn::scheduler scheduler;
    scheduler.schedule_after(30ms, recorder.record(3));
    scheduler.schedule_after(10ms, recorder.record(2));
    scheduler.enqueue(recorder.record(1));

    EXPECT_TRUE(eventually([&]() { return recorder.values().size() == 3; }));
    EXPECT_EQ(recorder.values(), (std::vector<int>{1, 2, 3}));
}

TEST(Scheduler, FunctionsDueTogetherRunInSchedulingOrder) {
    Recorder recorder;
    kvn::scheduler scheduler;
    auto when = Clock::now() + 20ms;
    for (int i = 0; i < 10; i++) {
        scheduler.schedule_at(when, recorder.record(i));
    }

    EXPECT_TRUE(eventually([&]() { return recorder.values().size() == 10; }));
    EXPECT_EQ(recorder.values(), (std::vector<int>{0, 1, 2, 3, 4, 5, 6, 7, 8, 9}));
}

TEST(Scheduler, NeverRunsFunctionsEarly) {
    // Every call wakes the scheduler's thread while it waits for an earlier deadline.
    std::atomic<int> ran{0};
    std::atomic<int> early{0};
    kvn::scheduler scheduler;
    for (int i = 0; i < 20; i++) {
        auto when = Clock::now() + std::chrono::milliseconds(60 - 2 * i);
        scheduler.schedule_at(when, [&, when]() {
            early += Clock::now() < when ? 1 : 0;
            ran++;
        });
        std::this_thread::sleep_for(1ms);
    }

    EXPECT_TRUE(eventually([&]() { return ran.load() == 20; }));
    EXPECT_EQ(early.load(), 0);
}

TEST(Scheduler, StopDropsPendingFunctions) {
    std::atomic<bool> ran{false};
    kvn::scheduler scheduler;
    scheduler.schedule_after(50ms, [&]() { ran = true; });
    scheduler.stop();
    scheduler.enqueue([&]() { ran = true; });
    scheduler.stop();

    std::this_thread::sleep_for(100ms);
    EXPECT_FALSE(ran.load());
}

TEST(Scheduler, StopFromOwnFunction) {
    std::atomic<bool> stopped{false};
    std::atomic<bool> later{false};
    {
        kvn::scheduler scheduler;
        scheduler.enqueue([&]() {
            scheduler.stop();
            stopped = true;
        });
        scheduler.schedule_after(20ms, [&]() { later = true; });
        EXPECT_TRUE(eventually([&]() { return stopped.load(); }));
    }
    EXPECT_FALSE(later.load());
}

TEST(Scheduler, DestroyedFromOwnFunction) {
    std::atomic<bool> finished{false};
    std::atomic<bool> later{false};
    std::promise<void> scheduled;
    auto all_scheduled = scheduled.get_future();

    auto scheduler = std::make_unique<kvn::scheduler>();
    scheduler->enqueue([&]() {
        all_scheduled.wait();
        scheduler.reset();
        // Still on the scheduler's thread, after the scheduler is gone.
        finished = true;
    });
    scheduler->schedule_after(20ms, [&]() { later = true; });
    scheduled.set_value();

    EXPECT_TRUE(eventually([&]() { return finished.load(); }));
    std::this_thread::sleep_for(50ms);
    EXPECT_FALSE(later.load());
}
