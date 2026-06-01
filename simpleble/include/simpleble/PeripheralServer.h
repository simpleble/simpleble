#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <simpleble/export.h>

#include <simpleble/Exceptions.h>
#include <simpleble/Types.h>

namespace SimpleBLE {

class PeripheralServerBase;

enum class CharacteristicProperty {
    READ,
    WRITE_REQUEST,
    WRITE_COMMAND,
    NOTIFY,
    INDICATE,
};

enum class AttributePermission {
    READABLE,
    WRITABLE,
    READ_ENCRYPTED,
    WRITE_ENCRYPTED,
    READ_AUTHENTICATED,
    WRITE_AUTHENTICATED,
};

enum class AttributeError {
    SUCCESS,
    INVALID_HANDLE,
    READ_NOT_PERMITTED,
    WRITE_NOT_PERMITTED,
    INVALID_OFFSET,
    INVALID_VALUE_LENGTH,
    INSUFFICIENT_AUTHENTICATION,
    INSUFFICIENT_AUTHORIZATION,
    INSUFFICIENT_ENCRYPTION,
    REQUEST_NOT_SUPPORTED,
    UNLIKELY_ERROR,
};

class SIMPLEBLE_EXPORT Central {
  public:
    Central() = default;
    Central(std::string identifier, BluetoothAddress address = "",
            BluetoothAddressType address_type = BluetoothAddressType::UNSPECIFIED, uint16_t mtu = 0);

    bool initialized() const;
    std::string identifier() const;
    BluetoothAddress address() const;
    BluetoothAddressType address_type() const;
    uint16_t mtu() const;

  private:
    std::string identifier_;
    BluetoothAddress address_;
    BluetoothAddressType address_type_ = BluetoothAddressType::UNSPECIFIED;
    uint16_t mtu_ = 0;
};

class SIMPLEBLE_EXPORT ReadRequest {
  public:
    ReadRequest(Central central, BluetoothUUID service, BluetoothUUID characteristic,
                std::optional<BluetoothUUID> descriptor, std::size_t offset, uint16_t mtu);

    Central central() const;
    BluetoothUUID service() const;
    BluetoothUUID characteristic() const;
    std::optional<BluetoothUUID> descriptor() const;
    std::size_t offset() const;
    uint16_t mtu() const;

  private:
    Central central_;
    BluetoothUUID service_;
    BluetoothUUID characteristic_;
    std::optional<BluetoothUUID> descriptor_;
    std::size_t offset_ = 0;
    uint16_t mtu_ = 0;
};

class SIMPLEBLE_EXPORT WriteRequest {
  public:
    WriteRequest(Central central, BluetoothUUID service, BluetoothUUID characteristic,
                 std::optional<BluetoothUUID> descriptor, ByteArray value, std::size_t offset,
                 bool response_required, bool prepared, uint16_t mtu);

    Central central() const;
    BluetoothUUID service() const;
    BluetoothUUID characteristic() const;
    std::optional<BluetoothUUID> descriptor() const;
    ByteArray value() const;
    std::size_t offset() const;
    bool response_required() const;
    bool prepared() const;
    uint16_t mtu() const;

  private:
    Central central_;
    BluetoothUUID service_;
    BluetoothUUID characteristic_;
    std::optional<BluetoothUUID> descriptor_;
    ByteArray value_;
    std::size_t offset_ = 0;
    bool response_required_ = false;
    bool prepared_ = false;
    uint16_t mtu_ = 0;
};

class SIMPLEBLE_EXPORT ReadResponse {
  public:
    static ReadResponse success(ByteArray value);
    static ReadResponse error(AttributeError error);

    bool is_success() const;
    ByteArray value() const;
    AttributeError error() const;

  private:
    ReadResponse(ByteArray value, AttributeError error);

    ByteArray value_;
    AttributeError error_ = AttributeError::SUCCESS;
};

class SIMPLEBLE_EXPORT WriteResponse {
  public:
    static WriteResponse success();
    static WriteResponse error(AttributeError error);

    bool is_success() const;
    AttributeError error() const;

  private:
    explicit WriteResponse(AttributeError error);

    AttributeError error_ = AttributeError::SUCCESS;
};

struct SIMPLEBLE_EXPORT DescriptorDefinition {
    BluetoothUUID uuid;
    ByteArray value;
    std::vector<AttributePermission> permissions;

    std::function<ReadResponse(ReadRequest)> on_read;
    std::function<WriteResponse(WriteRequest)> on_write;
};

struct SIMPLEBLE_EXPORT CharacteristicDefinition {
    BluetoothUUID uuid;
    std::vector<CharacteristicProperty> properties;
    std::vector<AttributePermission> permissions;
    ByteArray value;
    std::vector<DescriptorDefinition> descriptors;

    std::function<ReadResponse(ReadRequest)> on_read;
    std::function<WriteResponse(WriteRequest)> on_write;
    std::function<void(Central)> on_subscribed;
    std::function<void(Central)> on_unsubscribed;
};

struct SIMPLEBLE_EXPORT ServiceDefinition {
    BluetoothUUID uuid;
    bool primary = true;
    std::vector<CharacteristicDefinition> characteristics;
};

struct SIMPLEBLE_EXPORT AdvertisingData {
    std::string local_name;
    std::vector<BluetoothUUID> service_uuids;
    std::map<uint16_t, ByteArray> manufacturer_data;
    std::map<BluetoothUUID, ByteArray> service_data;
    std::optional<uint16_t> appearance;
    bool connectable = true;
    bool discoverable = true;
};

class SIMPLEBLE_EXPORT PeripheralServer {
  public:
    PeripheralServer() = default;
    virtual ~PeripheralServer() = default;

    bool initialized() const;
    void* underlying() const;

    void add_service(ServiceDefinition service);
    void remove_service(BluetoothUUID const& service);
    std::vector<ServiceDefinition> services();

    void start();
    void stop();
    bool is_active();

    void start_advertising(AdvertisingData advertising_data);
    void stop_advertising();
    bool is_advertising();

    void notify(BluetoothUUID const& service, BluetoothUUID const& characteristic, ByteArray const& data);
    void notify(Central const& central, BluetoothUUID const& service, BluetoothUUID const& characteristic,
                ByteArray const& data);
    void indicate(BluetoothUUID const& service, BluetoothUUID const& characteristic, ByteArray const& data);
    void indicate(Central const& central, BluetoothUUID const& service, BluetoothUUID const& characteristic,
                  ByteArray const& data);

    std::vector<Central> centrals();

    void set_callback_on_started(std::function<void()> on_started);
    void set_callback_on_stopped(std::function<void()> on_stopped);
    void set_callback_on_advertising_started(std::function<void()> on_started);
    void set_callback_on_advertising_stopped(std::function<void()> on_stopped);
    void set_callback_on_central_connected(std::function<void(Central)> on_connected);
    void set_callback_on_central_disconnected(std::function<void(Central)> on_disconnected);

  protected:
    PeripheralServerBase* operator->();
    const PeripheralServerBase* operator->() const;

    std::shared_ptr<PeripheralServerBase> internal_;
};

}  // namespace SimpleBLE
