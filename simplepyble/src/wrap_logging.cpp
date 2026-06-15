#include <pybind11/pybind11.h>

#include "simpleble/Logging.h"

#include <string>

namespace py = pybind11;

namespace {

int to_python_log_level(SimpleBLE::Logging::Level level) {
    switch (level) {
        case SimpleBLE::Logging::Level::Fatal:
            return 50;  // logging.CRITICAL
        case SimpleBLE::Logging::Level::Error:
            return 40;  // logging.ERROR
        case SimpleBLE::Logging::Level::Warn:
            return 30;  // logging.WARNING
        case SimpleBLE::Logging::Level::Info:
            return 20;  // logging.INFO
        case SimpleBLE::Logging::Level::Debug:
        case SimpleBLE::Logging::Level::Verbose:
            return 10;  // logging.DEBUG
        case SimpleBLE::Logging::Level::None:
            return 0;  // logging.NOTSET
    }

    return 0;
}

void forward_to_python_logging(SimpleBLE::Logging::Level level, const std::string& module, const std::string& file,
                               uint32_t line, const std::string& function, const std::string& message) {
    if (!Py_IsInitialized()) {
        return;
    }

    try {
        py::gil_scoped_acquire gil;

        py::object logger = py::module_::import("logging").attr("getLogger")("simplepyble");
        py::dict extra;
        extra["simpleble_module"] = module;
        extra["simpleble_file"] = file;
        extra["simpleble_line"] = line;
        extra["simpleble_function"] = function;

        logger.attr("log")(to_python_log_level(level), message, py::arg("extra") = extra);
    } catch (...) {
        // Logging must never interrupt Bluetooth operations.
    }
}

}  // namespace

void wrap_logging(py::module& /*m*/) {
    SimpleBLE::Logging::Logger::get()->set_callback(forward_to_python_logging);
}
