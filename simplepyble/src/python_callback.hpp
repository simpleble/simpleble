#pragma once

#include <pybind11/pybind11.h>

#include <atomic>
#include <functional>
#include <memory>
#include <optional>
#include <stdexcept>
#include <utility>

namespace py = pybind11;

namespace simplepyble {

// Holds a Python callable for SimpleBLE to call from its own threads.
//
// NOTE: pybind11's std::function conversion takes the GIL when its last copy is destroyed, which aborts the process
// when SimpleBLE drops a callback after the interpreter has finalized. Once Python is exiting, this holder doesn't
// call into Python and leaks the callable instead.
class PythonCallback {
  public:
    explicit PythonCallback(py::function callable) : holder_(std::make_shared<Holder>(std::move(callable))) {}

    // Registers the atexit hook that stops every PythonCallback from calling into Python. Called once at import.
    // NOTE: atexit runs hooks in reverse order, so this one runs after those registered later, like the aio cleanup.
    static void register_exit_hook() {
        py::module_::import("atexit").attr("register")(py::cpp_function([]() { python_exiting_ = true; }));
    }

    // Runs body(callable) with the GIL held and returns its result, for callbacks whose answer SimpleBLE needs.
    // Once Python is exiting it throws instead, which SimpleBLE treats like a callback that raised.
    template <typename Body>
    auto call(Body&& body) const {
        if (!python_running()) throw std::runtime_error("Python is exiting");

        py::gil_scoped_acquire gil;
        return body(holder_->callable);
    }

    // Runs body(callable) with the GIL held, for events. Once Python is exiting the event is dropped.
    // NOTE: Throwing here instead would make SimpleBLE log every event that arrives during exit.
    template <typename Body>
    void deliver(Body&& body) const {
        if (!python_running()) return;

        py::gil_scoped_acquire gil;
        body(holder_->callable);
    }

    template <typename... Args>
    void operator()(Args&&... args) const {
        deliver([&](const py::function& callable) { callable(std::forward<Args>(args)...); });
    }

  private:
    struct Holder {
        explicit Holder(py::function callable) : callable(std::move(callable)) {}

        ~Holder() {
            if (!python_running()) {
                callable.release();
                return;
            }

            py::gil_scoped_acquire gil;
            callable = py::function();
        }

        py::function callable;
    };

    // NOTE: Taking the GIL from a native thread once the interpreter is finalizing aborts the process or hangs the
    // thread, so every call into Python checks this first. The atexit hook sets the flag before finalization starts.
    static bool python_running() { return !python_exiting_ && Py_IsInitialized(); }

    static inline std::atomic_bool python_exiting_{false};

    // NOTE: SimpleBLE copies callbacks without the GIL, so copies share one holder instead of touching the
    // reference count.
    std::shared_ptr<Holder> holder_;
};

// Converts a Python callable into a native callback. None unregisters the callback.
template <typename Signature>
std::function<Signature> native_callback(std::optional<py::function> callable) {
    if (!callable) return nullptr;
    return PythonCallback(std::move(*callable));
}

// Wraps a SimpleBLE callback setter whose arguments pybind11 can convert, for use with py::gil_scoped_release.
template <typename Class, typename Signature>
auto callback_setter(void (Class::*setter)(std::function<Signature>)) {
    return [setter](Class& object, std::optional<py::function> callable) {
        (object.*setter)(native_callback<Signature>(std::move(callable)));
    };
}

}  // namespace simplepyble
