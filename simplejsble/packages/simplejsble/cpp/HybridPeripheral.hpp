#pragma once

#include "HybridPeripheralSpec.hpp"
#include "BluetoothAddressType.hpp"
#include <simpleble/SimpleBLE.h>
#include <functional>
#include <string>
#include <map>
#include <utility>

namespace margelo::nitro::simplejsble {

class HybridService;

class HybridPeripheral : public HybridPeripheralSpec {
  public:
    HybridPeripheral() : HybridObject(TAG) {}
    explicit HybridPeripheral(SimpleBLE::Peripheral peripheral)
        : HybridObject(TAG), _peripheral(std::move(peripheral)) {}

    bool initialized() override;
    std::string identifier() override;
    std::string address() override;
    BluetoothAddressType address_type() override;
    double rssi() override;
    double tx_power() override;
    double mtu() override;

    void connect() override;
    void disconnect() override;
    bool is_connected() override;
    bool is_connectable() override;
    bool is_paired() override;
    void unpair() override;

    void set_callback_on_connected(const std::function<void()>& callback) override;
    void set_callback_on_disconnected(const std::function<void()>& callback) override;

    std::vector<std::shared_ptr<HybridService>> services() override;
    std::unordered_map<double, ArrayBuffer> manufacturer_data() override;

    ArrayBuffer read(const std::string& service, const std::string& characteristic) override;
    void write_request(const std::string& service, const std::string& characteristic, ArrayBuffer data) override;
    void write_command(const std::string& service, const std::string& characteristic, ArrayBuffer data) override;
    void notify(const std::string& service, const std::string& characteristic, 
                const std::function<void(ArrayBuffer)>& callback) override;
    void indicate(const std::string& service, const std::string& characteristic, 
                  const std::function<void(ArrayBuffer)>& callback) override;
    void unsubscribe(const std::string& service, const std::string& characteristic) override;

    ArrayBuffer read_descriptor(const std::string& service, const std::string& characteristic, 
                                const std::string& descriptor) override;
    void write_descriptor(const std::string& service, const std::string& characteristic, 
                         const std::string& descriptor, ArrayBuffer data) override;

    SimpleBLE::Peripheral& getInternal() { return _peripheral; }
    const SimpleBLE::Peripheral& getInternal() const { return _peripheral; }

  private:
    SimpleBLE::Peripheral _peripheral;

    std::function<void()> _onConnected;
    std::function<void()> _onDisconnected;

    std::map<std::pair<std::string, std::string>, std::function<void(ArrayBuffer)>> _notifyCallbacks;

    // Helper methods to cast between SimpleBLE::ByteArray and ArrayBuffer (NitroModules native type)
    static ArrayBuffer toArrayBuffer(const SimpleBLE::ByteArray& data);
    static SimpleBLE::ByteArray fromArrayBuffer(const ArrayBuffer& buffer);
};

}  // namespace margelo::nitro::simplejsble
