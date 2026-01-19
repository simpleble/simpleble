#pragma once

#include "HybridPeripheralSpec.hpp"
#include "BluetoothAddressType.hpp"
#include <NitroModules/Promise.hpp>
#include <simpleble/SimpleBLE.h>
#include <functional>
#include <string>
#include <map>
#include <unordered_map>
#include <utility>

namespace margelo::nitro::simplejsble {

class HybridServiceSpec;

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

    std::shared_ptr<Promise<void>> connect() override;
    std::shared_ptr<Promise<void>> disconnect() override;
    bool is_connected() override;
    bool is_connectable() override;
    bool is_paired() override;
    void unpair() override;

    void set_callback_on_connected(const std::function<void()>& callback) override;
    void set_callback_on_disconnected(const std::function<void()>& callback) override;

    std::vector<std::shared_ptr<HybridServiceSpec>> services() override;
    std::unordered_map<std::string, std::shared_ptr<ArrayBuffer>> manufacturer_data() override;

    std::shared_ptr<Promise<std::shared_ptr<ArrayBuffer>>> read(const std::string& service, const std::string& characteristic) override;
    std::shared_ptr<Promise<void>> write_request(const std::string& service, const std::string& characteristic, const std::shared_ptr<ArrayBuffer>& data) override;
    void write_command(const std::string& service, const std::string& characteristic, const std::shared_ptr<ArrayBuffer>& data) override;
    std::shared_ptr<Promise<void>> notify(const std::string& service, const std::string& characteristic, 
                const std::function<void(const std::shared_ptr<ArrayBuffer>&)>& callback) override;
    std::shared_ptr<Promise<void>> indicate(const std::string& service, const std::string& characteristic, 
                  const std::function<void(const std::shared_ptr<ArrayBuffer>&)>& callback) override;
    std::shared_ptr<Promise<void>> unsubscribe(const std::string& service, const std::string& characteristic) override;

    std::shared_ptr<Promise<std::shared_ptr<ArrayBuffer>>> read_descriptor(const std::string& service, const std::string& characteristic, 
                                const std::string& descriptor) override;
    std::shared_ptr<Promise<void>> write_descriptor(const std::string& service, const std::string& characteristic, 
                         const std::string& descriptor, const std::shared_ptr<ArrayBuffer>& data) override;

    SimpleBLE::Peripheral& getInternal() { return _peripheral; }
    const SimpleBLE::Peripheral& getInternal() const { return _peripheral; }

  private:
    SimpleBLE::Peripheral _peripheral;

    std::function<void()> _onConnected;
    std::function<void()> _onDisconnected;

    std::map<std::pair<std::string, std::string>, std::function<void(const std::shared_ptr<ArrayBuffer>&)>> _notifyCallbacks;

    // Helper methods to cast between SimpleBLE::ByteArray and ArrayBuffer (NitroModules native type)
    static std::shared_ptr<ArrayBuffer> toArrayBuffer(const SimpleBLE::ByteArray& data);
    static SimpleBLE::ByteArray fromArrayBuffer(const std::shared_ptr<ArrayBuffer>& buffer);
};

}  // namespace margelo::nitro::simplejsble
