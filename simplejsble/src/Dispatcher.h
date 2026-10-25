#pragma once

#include <napi.h>

#include <cstddef>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <utility>

namespace SimpleJSBLE {

// Runs jobs on the JavaScript thread of one Node environment. Any thread can
// post without blocking, and jobs run in posting order. Nothing is dropped.
// After close(), posted jobs are discarded and queued ones are destroyed
// without running.
class Dispatcher {
  public:
    using Job = std::function<void(Napi::Env)>;

    explicit Dispatcher(Napi::Env env);

    void post(Job job);

    // JavaScript thread only. While anything is held, the event loop stays alive.
    void hold(Napi::Env env);
    void release(Napi::Env env);

    void close();

  private:
    // Shared with the thread-safe function's finalizer, which can outlive the
    // dispatcher. Its mutex also guards the dispatcher's own fields.
    struct State {
        std::mutex mutex;
        bool closed = false;
    };

    static void drain(Napi::Env env, Napi::Function callback, Dispatcher* self, std::nullptr_t* data);
    static bool report(Napi::Env env, const Napi::Error& error);

    std::shared_ptr<State> state_ = std::make_shared<State>();
    std::deque<Job> queue_;
    bool scheduled_ = false;
    std::size_t holds_ = 0;
    Napi::TypedThreadSafeFunction<Dispatcher, std::nullptr_t, drain> function_;
};

}  // namespace SimpleJSBLE
