#include <simpleble/PeripheralServer.h>

#include "PeripheralServerBase.h"

#include <utility>

using namespace SimpleBLE;

Central::Central(std::string identifier, BluetoothAddress address, BluetoothAddressType address_type, uint16_t mtu)
    : identifier_(std::move(identifier)), address_(std::move(address)), address_type_(address_type), mtu_(mtu) {}

bool Central::initialized() const { return !identifier_.empty(); }

std::string Central::identifier() const { return identifier_; }

BluetoothAddress Central::address() const { return address_; }

BluetoothAddressType Central::address_type() const { return address_type_; }

uint16_t Central::mtu() const { return mtu_; }

ReadRequest::ReadRequest(Central central, BluetoothUUID service, BluetoothUUID characteristic,
                         std::optional<BluetoothUUID> descriptor, std::size_t offset, uint16_t mtu)
    : central_(std::move(central)),
      service_(std::move(service)),
      characteristic_(std::move(characteristic)),
      descriptor_(std::move(descriptor)),
      offset_(offset),
      mtu_(mtu) {}

Central ReadRequest::central() const { return central_; }

BluetoothUUID ReadRequest::service() const { return service_; }

BluetoothUUID ReadRequest::characteristic() const { return characteristic_; }

std::optional<BluetoothUUID> ReadRequest::descriptor() const { return descriptor_; }

std::size_t ReadRequest::offset() const { return offset_; }

uint16_t ReadRequest::mtu() const { return mtu_; }

WriteRequest::WriteRequest(Central central, BluetoothUUID service, BluetoothUUID characteristic,
                           std::optional<BluetoothUUID> descriptor, ByteArray value, std::size_t offset,
                           bool response_required, bool prepared, uint16_t mtu)
    : central_(std::move(central)),
      service_(std::move(service)),
      characteristic_(std::move(characteristic)),
      descriptor_(std::move(descriptor)),
      value_(std::move(value)),
      offset_(offset),
      response_required_(response_required),
      prepared_(prepared),
      mtu_(mtu) {}

Central WriteRequest::central() const { return central_; }

BluetoothUUID WriteRequest::service() const { return service_; }

BluetoothUUID WriteRequest::characteristic() const { return characteristic_; }

std::optional<BluetoothUUID> WriteRequest::descriptor() const { return descriptor_; }

ByteArray WriteRequest::value() const { return value_; }

std::size_t WriteRequest::offset() const { return offset_; }

bool WriteRequest::response_required() const { return response_required_; }

bool WriteRequest::prepared() const { return prepared_; }

uint16_t WriteRequest::mtu() const { return mtu_; }

ReadResponse ReadResponse::success(ByteArray value) { return ReadResponse(std::move(value), AttributeError::SUCCESS); }

ReadResponse ReadResponse::error(AttributeError error) { return ReadResponse(ByteArray(), error); }

ReadResponse::ReadResponse(ByteArray value, AttributeError error) : value_(std::move(value)), error_(error) {}

bool ReadResponse::is_success() const { return error_ == AttributeError::SUCCESS; }

ByteArray ReadResponse::value() const { return value_; }

AttributeError ReadResponse::error() const { return error_; }

WriteResponse WriteResponse::success() { return WriteResponse(AttributeError::SUCCESS); }

WriteResponse WriteResponse::error(AttributeError error) { return WriteResponse(error); }

WriteResponse::WriteResponse(AttributeError error) : error_(error) {}

bool WriteResponse::is_success() const { return error_ == AttributeError::SUCCESS; }

AttributeError WriteResponse::error() const { return error_; }

bool PeripheralServer::initialized() const { return internal_ != nullptr; }

PeripheralServerBase* PeripheralServer::operator->() {
    if (!initialized()) throw Exception::NotInitialized();

    return internal_.get();
}

const PeripheralServerBase* PeripheralServer::operator->() const {
    if (!initialized()) throw Exception::NotInitialized();

    return internal_.get();
}

void* PeripheralServer::underlying() const { return (*this)->underlying(); }

void PeripheralServer::add_service(ServiceDefinition service) { (*this)->add_service(std::move(service)); }

void PeripheralServer::remove_service(BluetoothUUID const& service) { (*this)->remove_service(service); }

std::vector<ServiceDefinition> PeripheralServer::services() { return (*this)->services(); }

void PeripheralServer::start() { (*this)->start(); }

void PeripheralServer::stop() { (*this)->stop(); }

bool PeripheralServer::is_active() { return (*this)->is_active(); }

void PeripheralServer::start_advertising(AdvertisingData advertising_data) {
    (*this)->start_advertising(std::move(advertising_data));
}

void PeripheralServer::stop_advertising() { (*this)->stop_advertising(); }

bool PeripheralServer::is_advertising() { return (*this)->is_advertising(); }

void PeripheralServer::notify(BluetoothUUID const& service, BluetoothUUID const& characteristic, ByteArray const& data) {
    (*this)->notify(service, characteristic, data);
}

void PeripheralServer::notify(Central const& central, BluetoothUUID const& service, BluetoothUUID const& characteristic,
                              ByteArray const& data) {
    (*this)->notify(central, service, characteristic, data);
}

void PeripheralServer::indicate(BluetoothUUID const& service, BluetoothUUID const& characteristic,
                                ByteArray const& data) {
    (*this)->indicate(service, characteristic, data);
}

void PeripheralServer::indicate(Central const& central, BluetoothUUID const& service,
                                BluetoothUUID const& characteristic, ByteArray const& data) {
    (*this)->indicate(central, service, characteristic, data);
}

std::vector<Central> PeripheralServer::centrals() { return (*this)->centrals(); }

void PeripheralServer::set_callback_on_started(std::function<void()> on_started) {
    (*this)->set_callback_on_started(std::move(on_started));
}

void PeripheralServer::set_callback_on_stopped(std::function<void()> on_stopped) {
    (*this)->set_callback_on_stopped(std::move(on_stopped));
}

void PeripheralServer::set_callback_on_advertising_started(std::function<void()> on_started) {
    (*this)->set_callback_on_advertising_started(std::move(on_started));
}

void PeripheralServer::set_callback_on_advertising_stopped(std::function<void()> on_stopped) {
    (*this)->set_callback_on_advertising_stopped(std::move(on_stopped));
}

void PeripheralServer::set_callback_on_central_connected(std::function<void(Central)> on_connected) {
    (*this)->set_callback_on_central_connected(std::move(on_connected));
}

void PeripheralServer::set_callback_on_central_disconnected(std::function<void(Central)> on_disconnected) {
    (*this)->set_callback_on_central_disconnected(std::move(on_disconnected));
}
