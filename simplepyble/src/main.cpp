#include <pybind11/functional.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "simpleble/SimpleBLE.h"

/**
 * Useful links for documentation while I figure this one out.
 * https://www.sphinx-doc.org/en/master/usage/extensions/autodoc.html
 * https://www.sphinx-doc.org/en/master/usage/extensions/napoleon.html
 * https://www.sphinx-doc.org/en/master/usage/extensions/autosummary.html
 */

namespace py = pybind11;

void wrap_types(py::module& m);
void wrap_descriptor(py::module& m);
void wrap_characteristic(py::module& m);
void wrap_service(py::module& m);
void wrap_peripheral(py::module& m);
void wrap_adapter(py::module& m);
void wrap_config(py::module& m);

PYBIND11_MODULE(_simplepyble, m) {
    m.attr("__version__") = SIMPLEPYBLE_VERSION;

    // m.doc() = R"pbdoc(
    //     ====================
    //     Python API Reference
    //     ====================

    //     .. currentmodule:: simplepyble
    //     .. autosummary::
    //        :toctree: _build

    //         get_operating_system

    // )pbdoc";

    m.def("get_operating_system", &SimpleBLE::get_operating_system, R"pbdoc(
        Returns the currently-running operating system.
    )pbdoc");

    m.def("get_simpleble_version", &SimpleBLE::get_simpleble_version, R"pbdoc(
        Returns the version of SimpleBLE.
    )pbdoc");

    m.def("get_company_name", &SimpleBLE::get_company_name, py::arg("company_id"), R"pbdoc(
        Returns the company name for a given Bluetooth company identifier code.

        Args:
            company_id: The Bluetooth SIG assigned company identifier (uint16).

        Returns:
            The company name string, or "Unknown" if the identifier is not found.
    )pbdoc");

    wrap_types(m);
    wrap_descriptor(m);
    wrap_characteristic(m);
    wrap_service(m);
    wrap_peripheral(m);
    wrap_adapter(m);
    wrap_config(m);
}
