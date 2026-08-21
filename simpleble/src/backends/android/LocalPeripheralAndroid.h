#pragma once

#include <atomic>
#include <condition_variable>
#include <deque>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <vector>

#include <kvn_safe_callback.hpp>

#include "../common/LocalPeripheralBase.h"
#include "bridge/AdvertiseCallback.h"
#include "bridge/GattServerCallback.h"
#include "simplejni/Common.hpp"
#include "types/android/bluetooth/BluetoothAdapter.h"
#include "types/android/bluetooth/BluetoothGattServer.h"
#include "types/android/bluetooth/le/BluetoothLeAdvertiser.h"

namespace SimpleBLE::Local {

class CharacteristicAndroid;
class ServiceAndroid;

class PeripheralAndroid : public PeripheralBase, public std::enable_shared_from_this<PeripheralAndroid> {
  public:
    explicit PeripheralAndroid(Android::BluetoothAdapter adapter);
    ~PeripheralAndroid() override;

    void* underlying() const override;
    Advertisement advertisement() override;
    void set_advertisement(Advertisement advertisement) override;
    std::shared_ptr<ServiceBase> add_service(BluetoothUUID uuid) override;
    std::vector<std::shared_ptr<ServiceBase>> services() override;
    void remove_all_services() override;
    void start() override;
    void stop() override;
    bool is_started() override;
    bool is_advertising() override;
    void set_callback_on_client_connected(
        std::function<void(BluetoothAddress client_address)> on_client_connected) override;
    void set_callback_on_client_disconnected(
        std::function<void(BluetoothAddress client_address)> on_client_disconnected) override;

    void register_characteristic(const std::shared_ptr<CharacteristicAndroid>& characteristic);
    void publish(const std::shared_ptr<CharacteristicAndroid>& characteristic, const ByteArray& value);

  private:
    using JavaObject = SimpleJNI::Object<SimpleJNI::GlobalRef, jobject>;
    using JavaObjectComparator = SimpleJNI::ObjectComparator<SimpleJNI::GlobalRef, jobject>;

    struct PendingNotification {
        Android::BluetoothDevice device;
        Android::BluetoothGattCharacteristic characteristic;
        ByteArray value;
        bool confirm;
    };

    Android::BluetoothAdapter _adapter;
    Android::BluetoothGattServer _server;
    Android::BluetoothLeAdvertiser _advertiser;
    Android::Bridge::GattServerCallback _server_callback;
    Android::Bridge::AdvertiseCallback _advertise_callback;

    Advertisement _advertisement;
    std::vector<std::shared_ptr<ServiceAndroid>> _services;
    std::map<JavaObject, std::weak_ptr<CharacteristicAndroid>, JavaObjectComparator> _characteristics;
    std::map<JavaObject, std::weak_ptr<CharacteristicAndroid>, JavaObjectComparator> _descriptors;
    std::set<BluetoothAddress> _connected_clients;
    std::deque<PendingNotification> _notifications;

    std::atomic_bool _started{false};
    std::atomic_bool _advertising{false};
    mutable std::mutex _lifecycle_mutex;
    std::mutex _server_mutex;
    std::mutex _characteristics_mutex;
    std::mutex _clients_mutex;
    std::mutex _notification_mutex;
    std::mutex _service_status_mutex;
    std::condition_variable _service_status_cv;
    bool _service_status_received{false};
    int _service_status{0};
    std::mutex _advertise_status_mutex;
    std::condition_variable _advertise_status_cv;
    bool _advertise_status_received{false};
    int _advertise_error{0};

    kvn::safe_callback<void(BluetoothAddress)> _callback_on_client_connected;
    kvn::safe_callback<void(BluetoothAddress)> _callback_on_client_disconnected;

    void _ensure_mutable() const;
    void _shutdown() noexcept;
    void _respond(const Android::BluetoothDevice& device, int request_id, int status, int offset,
                  const ByteArray& value = {});
    std::shared_ptr<CharacteristicAndroid> _characteristic_for(const JavaObject& object, bool descriptor);
    void _handle_connection(Android::BluetoothDevice device, int status, int new_state);
    void _handle_characteristic_read(Android::BluetoothDevice device, int request_id, int offset,
                                     Android::BluetoothGattCharacteristic characteristic);
    void _handle_characteristic_write(Android::BluetoothDevice device, int request_id,
                                      Android::BluetoothGattCharacteristic characteristic, bool prepared_write,
                                      bool response_needed, int offset, ByteArray value);
    void _handle_descriptor_read(Android::BluetoothDevice device, int request_id, int offset,
                                 Android::BluetoothGattDescriptor descriptor);
    void _handle_descriptor_write(Android::BluetoothDevice device, int request_id,
                                  Android::BluetoothGattDescriptor descriptor, bool prepared_write,
                                  bool response_needed, int offset, ByteArray value);
    void _send_next_notification_locked();
};

}  // namespace SimpleBLE::Local
