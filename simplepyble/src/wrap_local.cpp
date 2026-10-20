#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <memory>
#include <optional>
#include <set>
#include <string>
#include <vector>

#include "simpleble/local/Peripheral.h"

#include "python_callback.hpp"

namespace py = pybind11;

using SimpleBLE::Local::CharacteristicCapability;

constexpr auto kDocsLocalCharacteristicCapability = R"pbdoc(
    Capability of a local characteristic
)pbdoc";

constexpr auto kDocsLocalCharacteristic = R"pbdoc(
    Characteristic hosted by a local peripheral
)pbdoc";

constexpr auto kDocsLocalCharacteristicInitialized = R"pbdoc(
    Whether the local characteristic is initialized
)pbdoc";

constexpr auto kDocsLocalCharacteristicUuid = R"pbdoc(
    UUID of the local characteristic
)pbdoc";

constexpr auto kDocsLocalCharacteristicCapabilities = R"pbdoc(
    Capabilities of the local characteristic
)pbdoc";

constexpr auto kDocsLocalCharacteristicValue = R"pbdoc(
    Current value of the local characteristic
)pbdoc";

constexpr auto kDocsLocalCharacteristicSetValue = R"pbdoc(
    Update the value and publish it to subscribed clients
)pbdoc";

constexpr auto kDocsLocalCharacteristicSetCallbackOnRead = R"pbdoc(
    Set the callback that supplies the value for client reads
)pbdoc";

constexpr auto kDocsLocalCharacteristicSetCallbackOnWrite = R"pbdoc(
    Set the callback to be called when a client writes the characteristic
)pbdoc";

constexpr auto kDocsLocalCharacteristicSetCallbackOnSubscribed = R"pbdoc(
    Set the callback to be called when the first client subscribes
)pbdoc";

constexpr auto kDocsLocalCharacteristicSetCallbackOnUnsubscribed = R"pbdoc(
    Set the callback to be called when the last client unsubscribes
)pbdoc";

constexpr auto kDocsLocalService = R"pbdoc(
    Service hosted by a local peripheral
)pbdoc";

constexpr auto kDocsLocalServiceInitialized = R"pbdoc(
    Whether the local service is initialized
)pbdoc";

constexpr auto kDocsLocalServiceUuid = R"pbdoc(
    UUID of the local service
)pbdoc";

constexpr auto kDocsLocalServiceAddCharacteristic = R"pbdoc(
    Add a characteristic with the given capabilities
)pbdoc";

constexpr auto kDocsLocalServiceCharacteristics = R"pbdoc(
    Characteristics of the local service
)pbdoc";

constexpr auto kDocsLocalPeripheral = R"pbdoc(
    BLE peripheral hosted by this process
)pbdoc";

constexpr auto kDocsLocalPeripheralInitialized = R"pbdoc(
    Whether the local peripheral is initialized
)pbdoc";

constexpr auto kDocsLocalPeripheralAddAdvertisedService = R"pbdoc(
    Add one or more service UUIDs to the advertisement
)pbdoc";

constexpr auto kDocsLocalPeripheralSetAdvertisedManufacturerData = R"pbdoc(
    Add manufacturer specific data to the advertisement

    Setting data for the same company identifier again replaces it.
)pbdoc";

constexpr auto kDocsLocalPeripheralSetConnectable = R"pbdoc(
    Choose whether centrals can connect

    Local peripherals are connectable by default.
)pbdoc";

constexpr auto kDocsLocalPeripheralAddService = R"pbdoc(
    Add a primary GATT service
)pbdoc";

constexpr auto kDocsLocalPeripheralServices = R"pbdoc(
    Services of the local peripheral
)pbdoc";

constexpr auto kDocsLocalPeripheralRemoveAllServices = R"pbdoc(
    Remove all services from the local peripheral
)pbdoc";

constexpr auto kDocsLocalPeripheralStart = R"pbdoc(
    Publish services and start advertising
)pbdoc";

constexpr auto kDocsLocalPeripheralStop = R"pbdoc(
    Stop advertising and hosting the services
)pbdoc";

constexpr auto kDocsLocalPeripheralIsStarted = R"pbdoc(
    Whether the local peripheral is started
)pbdoc";

constexpr auto kDocsLocalPeripheralIsAdvertising = R"pbdoc(
    Whether the local peripheral is advertising
)pbdoc";

constexpr auto kDocsLocalPeripheralSetCallbackOnClientConnected = R"pbdoc(
    Set the callback to be called when a client connects
)pbdoc";

constexpr auto kDocsLocalPeripheralSetCallbackOnClientDisconnected = R"pbdoc(
    Set the callback to be called when a client disconnects
)pbdoc";

namespace {

// NOTE: Destroying the last reference to a local object can stop advertising and wait
// for callbacks that need the GIL, so we release the GIL while deleting it.
template <typename T>
struct ReleaseGilDeleter {
    void operator()(T* object) const {
        py::gil_scoped_release release;
        delete object;
    }
};

template <typename T>
using LocalHolder = std::unique_ptr<T, ReleaseGilDeleter<T>>;

// Accepts any contiguous bytes-like object, such as bytes, bytearray, or memoryview.
SimpleBLE::ByteArray to_byte_array(py::handle value) {
    Py_buffer view;
    if (PyObject_GetBuffer(value.ptr(), &view, PyBUF_SIMPLE) != 0) throw py::error_already_set();

    SimpleBLE::ByteArray bytes(static_cast<const uint8_t*>(view.buf), static_cast<size_t>(view.len));
    PyBuffer_Release(&view);
    return bytes;
}

std::set<CharacteristicCapability> to_capabilities(py::iterable capabilities) {
    std::set<CharacteristicCapability> result;
    for (auto capability : capabilities) {
        if (!py::isinstance<CharacteristicCapability>(capability)) {
            throw py::type_error("capabilities must contain LocalCharacteristicCapability values");
        }
        result.insert(capability.cast<CharacteristicCapability>());
    }
    return result;
}

}  // namespace

// clang-format off

void wrap_local(py::module& m) {
    py::enum_<CharacteristicCapability>(m, "LocalCharacteristicCapability", kDocsLocalCharacteristicCapability)
        .value("READ", CharacteristicCapability::READ)
        .value("WRITE_REQUEST", CharacteristicCapability::WRITE_REQUEST)
        .value("WRITE_COMMAND", CharacteristicCapability::WRITE_COMMAND)
        .value("NOTIFY", CharacteristicCapability::NOTIFY)
        .value("INDICATE", CharacteristicCapability::INDICATE);

    py::class_<SimpleBLE::Local::Characteristic, LocalHolder<SimpleBLE::Local::Characteristic>>(
        m, "LocalCharacteristic", kDocsLocalCharacteristic)
        .def("initialized", &SimpleBLE::Local::Characteristic::initialized, kDocsLocalCharacteristicInitialized)
        .def("uuid", &SimpleBLE::Local::Characteristic::uuid, kDocsLocalCharacteristicUuid)
        .def("capabilities", &SimpleBLE::Local::Characteristic::capabilities, kDocsLocalCharacteristicCapabilities)
        .def(
            "value",
            [](SimpleBLE::Local::Characteristic& c) {
                SimpleBLE::ByteArray value = c.value();
                py::gil_scoped_acquire gil;
                return py::bytes(value);
            },
            py::call_guard<py::gil_scoped_release>(),
            kDocsLocalCharacteristicValue)
        .def(
            "set_value",
            [](SimpleBLE::Local::Characteristic& c, py::object value) {
                SimpleBLE::ByteArray cpp_value = to_byte_array(value);
                py::gil_scoped_release release;
                c.set_value(std::move(cpp_value));
            },
            kDocsLocalCharacteristicSetValue)
        .def(
            "set_callback_on_read",
            [](SimpleBLE::Local::Characteristic& c, std::optional<py::function> cb) {
                if (!cb) {
                    c.set_callback_on_read(nullptr);
                    return;
                }
                simplepyble::PythonCallback callback(std::move(*cb));
                c.set_callback_on_read([callback]() {
                    return callback.call([](const py::function& callable) { return to_byte_array(callable()); });
                });
            },
            py::call_guard<py::gil_scoped_release>(),
            kDocsLocalCharacteristicSetCallbackOnRead)
        .def(
            "set_callback_on_write",
            [](SimpleBLE::Local::Characteristic& c, std::optional<py::function> cb) {
                if (!cb) {
                    c.set_callback_on_write(nullptr);
                    return;
                }
                simplepyble::PythonCallback callback(std::move(*cb));
                c.set_callback_on_write([callback](SimpleBLE::ByteArray value) {
                    callback.deliver([&](const py::function& callable) { callable(py::bytes(value)); });
                });
            },
            py::call_guard<py::gil_scoped_release>(),
            kDocsLocalCharacteristicSetCallbackOnWrite)
        .def("set_callback_on_subscribed", simplepyble::callback_setter(&SimpleBLE::Local::Characteristic::set_callback_on_subscribed),
             py::call_guard<py::gil_scoped_release>(), kDocsLocalCharacteristicSetCallbackOnSubscribed)
        .def("set_callback_on_unsubscribed", simplepyble::callback_setter(&SimpleBLE::Local::Characteristic::set_callback_on_unsubscribed),
             py::call_guard<py::gil_scoped_release>(), kDocsLocalCharacteristicSetCallbackOnUnsubscribed);

    py::class_<SimpleBLE::Local::Service, LocalHolder<SimpleBLE::Local::Service>>(m, "LocalService", kDocsLocalService)
        .def("initialized", &SimpleBLE::Local::Service::initialized, kDocsLocalServiceInitialized)
        .def("uuid", &SimpleBLE::Local::Service::uuid, kDocsLocalServiceUuid)
        .def(
            "add_characteristic",
            [](SimpleBLE::Local::Service& s, std::string uuid, py::iterable capabilities) {
                std::set<CharacteristicCapability> cpp_capabilities = to_capabilities(capabilities);
                py::gil_scoped_release release;
                return s.add_characteristic(std::move(uuid), std::move(cpp_capabilities));
            },
            kDocsLocalServiceAddCharacteristic)
        .def("characteristics", &SimpleBLE::Local::Service::characteristics, py::call_guard<py::gil_scoped_release>(),
             kDocsLocalServiceCharacteristics);

    py::class_<SimpleBLE::Local::Peripheral, LocalHolder<SimpleBLE::Local::Peripheral>>(
        m, "LocalPeripheral", kDocsLocalPeripheral)
        .def("initialized", &SimpleBLE::Local::Peripheral::initialized, kDocsLocalPeripheralInitialized)
        .def("add_advertised_service",
             py::overload_cast<SimpleBLE::BluetoothUUID>(&SimpleBLE::Local::Peripheral::add_advertised_service),
             py::call_guard<py::gil_scoped_release>(), kDocsLocalPeripheralAddAdvertisedService)
        .def("add_advertised_service",
             py::overload_cast<std::vector<SimpleBLE::BluetoothUUID>>(&SimpleBLE::Local::Peripheral::add_advertised_service),
             py::call_guard<py::gil_scoped_release>(), kDocsLocalPeripheralAddAdvertisedService)
        .def(
            "set_advertised_manufacturer_data",
            [](SimpleBLE::Local::Peripheral& p, uint16_t company_id, py::object data) {
                SimpleBLE::ByteArray cpp_data = to_byte_array(data);
                py::gil_scoped_release release;
                p.set_advertised_manufacturer_data(company_id, std::move(cpp_data));
            },
            kDocsLocalPeripheralSetAdvertisedManufacturerData)
        .def("set_connectable", &SimpleBLE::Local::Peripheral::set_connectable,
             py::call_guard<py::gil_scoped_release>(), kDocsLocalPeripheralSetConnectable)
        .def("add_service", &SimpleBLE::Local::Peripheral::add_service, py::call_guard<py::gil_scoped_release>(),
             kDocsLocalPeripheralAddService)
        .def("services", &SimpleBLE::Local::Peripheral::services, py::call_guard<py::gil_scoped_release>(),
             kDocsLocalPeripheralServices)
        .def("remove_all_services", &SimpleBLE::Local::Peripheral::remove_all_services,
             py::call_guard<py::gil_scoped_release>(), kDocsLocalPeripheralRemoveAllServices)
        .def("start", &SimpleBLE::Local::Peripheral::start, py::call_guard<py::gil_scoped_release>(),
             kDocsLocalPeripheralStart)
        .def("stop", &SimpleBLE::Local::Peripheral::stop, py::call_guard<py::gil_scoped_release>(),
             kDocsLocalPeripheralStop)
        .def("is_started", &SimpleBLE::Local::Peripheral::is_started, py::call_guard<py::gil_scoped_release>(),
             kDocsLocalPeripheralIsStarted)
        .def("is_advertising", &SimpleBLE::Local::Peripheral::is_advertising, py::call_guard<py::gil_scoped_release>(),
             kDocsLocalPeripheralIsAdvertising)
        .def("set_callback_on_client_connected", simplepyble::callback_setter(&SimpleBLE::Local::Peripheral::set_callback_on_client_connected),
             py::call_guard<py::gil_scoped_release>(), kDocsLocalPeripheralSetCallbackOnClientConnected)
        .def("set_callback_on_client_disconnected", simplepyble::callback_setter(&SimpleBLE::Local::Peripheral::set_callback_on_client_disconnected),
             py::call_guard<py::gil_scoped_release>(), kDocsLocalPeripheralSetCallbackOnClientDisconnected);
}

// clang-format on
