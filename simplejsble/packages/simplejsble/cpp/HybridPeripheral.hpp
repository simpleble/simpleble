#pragma once

#include "HybridPeripheralSpec.hpp"
#include "BluetoothAddressType.hpp"
#include <simpleble/SimpleBLE.h>
#include <functional>
#include <string>

namespace margelo::nitro::simplejsble {

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

    SimpleBLE::Peripheral& getInternal() { return _peripheral; }
    const SimpleBLE::Peripheral& getInternal() const { return _peripheral; }

  private:
    SimpleBLE::Peripheral _peripheral;

    std::function<void()> _onConnected;
    std::function<void()> _onDisconnected;
};

}  // namespace margelo::nitro::simplejsble
