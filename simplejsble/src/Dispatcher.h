#pragma once

#include <napi.h>

#include <cstddef>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <set>
#include <utility>

namespace SimpleJSBLE {

// Runs jobs on the JavaScript thread of one Node environment. Any thread can
// post without blocking, and jobs run in posting order. Nothing is dropped,
// except a coalesced job whose key is already queued. After close(), posted
// jobs are discarded and queued ones are destroyed without running.
class Dispatcher {
  public:
    using Job = std::function<void(Napi::Env)>;
    using Key = std::pair<const void*, const void*>;

    explicit Dispatcher(Napi::Env env);

    void post(Job job);
    void post_coalesced(Key key, Job job);

    // JavaScript thread only. While anything is held, the event loop stays alive.
    void hold(Napi::Env env);
    void release(Napi::Env env);

    void close();

  private:
    struct Entry {
        Job job;
        std::optional<Key> key;
    };

    static void drain(Napi::Env env, Napi::Function callback, Dispatcher* self, std::nullptr_t* data);
    static bool report(Napi::Env env, const Napi::Error& error);
    void enqueue(Entry entry);

    std::mutex mutex_;
    std::deque<Entry> queue_;
    std::set<Key> queued_keys_;
    bool scheduled_ = false;
    bool closed_ = false;
    std::size_t holds_ = 0;
    Napi::TypedThreadSafeFunction<Dispatcher, std::nullptr_t, drain> function_;
};

}  // namespace SimpleJSBLE
