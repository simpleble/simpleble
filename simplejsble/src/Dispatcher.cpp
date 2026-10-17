#include "Dispatcher.h"

#include <exception>

namespace SimpleJSBLE {

// NOTE: Node deletes the thread-safe function when it tears down the
// environment, which Electron does before the addon's own cleanup runs. The
// finalizer then marks the dispatcher closed, so nothing touches the function.
Dispatcher::Dispatcher(Napi::Env env) {
    auto finalize = [](Napi::Env, std::shared_ptr<State>* state, Dispatcher*) {
        {
            std::lock_guard<std::mutex> lock((*state)->mutex);
            (*state)->closed = true;
        }
        delete state;
    };
    function_ = decltype(function_)::New(env, "simplejsble", 0, 1, this, finalize, new std::shared_ptr<State>(state_));
    // NOTE: Only pending operations keep Node alive, see hold().
    function_.Unref(env);
}

void Dispatcher::post(Job job) { enqueue({std::move(job), std::nullopt}); }

void Dispatcher::post_coalesced(Key key, Job job) { enqueue({std::move(job), key}); }

void Dispatcher::enqueue(Entry entry) {
    std::lock_guard<std::mutex> lock(state_->mutex);
    if (state_->closed) return;

    if (entry.key) {
        if (!queued_keys_.insert(*entry.key).second) return;
    }
    queue_.push_back(std::move(entry));

    // A single outstanding call drains everything queued until it runs.
    if (scheduled_) return;
    scheduled_ = function_.NonBlockingCall() == napi_ok;
}

void Dispatcher::drain(Napi::Env env, Napi::Function, Dispatcher* self, std::nullptr_t*) {
    // NOTE: Node passes a null environment while tearing down a queue it can no
    // longer run; close() has already discarded the jobs.
    if (env == nullptr) return;

    std::deque<Entry> jobs;
    {
        std::lock_guard<std::mutex> lock(self->state_->mutex);
        self->scheduled_ = false;
        jobs.swap(self->queue_);
        self->queued_keys_.clear();
    }

    for (auto& entry : jobs) {
        Napi::HandleScope scope(env);
        try {
            entry.job(env);
        } catch (const Napi::Error& error) {
            // An exception from a listener behaves like one thrown from a timer.
            // NOTE: JavaScript stops running before a terminated worker stops
            // its event loop. Reporting fails then, and the rest is dropped.
            if (!report(env, error)) return;
        } catch (const std::exception& error) {
            if (!report(env, Napi::Error::New(env, error.what()))) return;
        }
    }
}

bool Dispatcher::report(Napi::Env env, const Napi::Error& error) {
    try {
        return napi_fatal_exception(env, error.Value()) == napi_ok;
    } catch (...) {
        return false;
    }
}

void Dispatcher::hold(Napi::Env env) {
    std::lock_guard<std::mutex> lock(state_->mutex);
    if (state_->closed) return;
    if (holds_++ == 0) function_.Ref(env);
}

void Dispatcher::release(Napi::Env env) {
    std::lock_guard<std::mutex> lock(state_->mutex);
    if (state_->closed) return;
    if (--holds_ == 0) function_.Unref(env);
}

void Dispatcher::close() {
    std::deque<Entry> jobs;
    bool abort = false;
    {
        std::lock_guard<std::mutex> lock(state_->mutex);
        abort = !state_->closed;
        state_->closed = true;
        jobs.swap(queue_);
        queued_keys_.clear();
    }
    if (abort) function_.Abort();
}

}  // namespace SimpleJSBLE
