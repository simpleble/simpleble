#include <simpleble/simulation/Device.h>

#include "BackendUtils.h"
#include "CommonUtils.h"
#include "backends/simulator/AdapterSimulator.h"
#include "backends/simulator/core/EnvironmentCore.h"

namespace SimpleBLE {
namespace {

using namespace Simulation;
using namespace Simulation::Internal;
using namespace std::chrono_literals;

const BluetoothUUID BATTERY_SERVICE = "0000180f-0000-1000-8000-00805f9b34fb";
const BluetoothUUID BATTERY_VALUE = "00002a19-0000-1000-8000-00805f9b34fb";
const BluetoothUUID TEST_SERVICE = "0000fff0-0000-1000-8000-00805f9b34fb";
const BluetoothUUID TEST_VALUE = "0000fff1-0000-1000-8000-00805f9b34fb";
const BluetoothUUID ERROR_VALUE = "0000fff2-0000-1000-8000-00805f9b34fb";
const BluetoothUUID DESCRIPTION = "00002901-0000-1000-8000-00805f9b34fb";

class PlainDevice final : public Device {
  public:
    PlainDevice() : Device("Plain Peripheral", "11:22:33:44:55:66") {
        set_tx_power(5);
        set_max_connections(8);
        set_max_data_length(251);
        set_manufacturer_data(0x004c, "test");
        set_service_data(TEST_SERVICE, ByteArray{0x00, 0x7f, 0x80, 0xff});

        add_advertised_service(BATTERY_SERVICE);
        add_service(BATTERY_SERVICE);
        add_characteristic(BATTERY_SERVICE, BATTERY_VALUE, {Property::READ, Property::NOTIFY});
        set_value(BATTERY_SERVICE, BATTERY_VALUE, ByteArray{100});

        add_service(TEST_SERVICE);
        add_characteristic(
            TEST_SERVICE, TEST_VALUE,
            {Property::READ, Property::WRITE_REQUEST, Property::WRITE_COMMAND, Property::NOTIFY, Property::INDICATE});
        set_value(TEST_SERVICE, TEST_VALUE, ByteArray{0x00, 0x7f, 0x80, 0xff});
        add_descriptor(TEST_SERVICE, TEST_VALUE, DESCRIPTION);
        set_value(TEST_SERVICE, TEST_VALUE, DESCRIPTION, "Plain value");
        add_characteristic(TEST_SERVICE, ERROR_VALUE, {Property::READ, Property::WRITE_REQUEST});

        start_timer(0, 1s);
    }

    ReadResult on_read(Connection connection, const BluetoothUUID& service,
                       const BluetoothUUID& characteristic) override {
        if (characteristic == ERROR_VALUE) return AttStatus::INSUFFICIENT_AUTHORIZATION;

        return Device::on_read(connection, service, characteristic);
    }

    AttStatus on_write_request(Connection connection, const BluetoothUUID& service, const BluetoothUUID& characteristic,
                               const ByteArray& value) override {
        if (characteristic == ERROR_VALUE) return AttStatus::VALUE_NOT_ALLOWED;

        return Device::on_write_request(connection, service, characteristic, value);
    }

    void on_timer(int) override {
        for (auto& connection : connections()) {
            notify(connection, BATTERY_SERVICE, BATTERY_VALUE, value(BATTERY_SERVICE, BATTERY_VALUE));

            auto payload = value(TEST_SERVICE, TEST_VALUE);
            notify(connection, TEST_SERVICE, TEST_VALUE, payload);
            indicate(connection, TEST_SERVICE, TEST_VALUE, payload);
        }
    }
};

}  // namespace

class BackendPlain : public BackendSingleton<BackendPlain> {
  public:
    BackendPlain(buildToken);
    ~BackendPlain() override;

    SharedPtrVector<AdapterBase> adapters() override;
    bool bluetooth_enabled() override;
    std::string identifier() const noexcept override;
    bool is_active() override { return true; }

  private:
    std::shared_ptr<EnvironmentCore> environment_;
};

std::shared_ptr<BackendBase> BACKEND_PLAIN() { return BackendPlain::get(); }

BackendPlain::BackendPlain(buildToken) : environment_(std::make_shared<EnvironmentCore>()) {
    environment_->add_adapter("Plain Adapter", "AA:BB:CC:DD:EE:FF");
    environment_->add_device(std::make_shared<PlainDevice>());
    environment_->activate(false);
}

BackendPlain::~BackendPlain() { environment_->shutdown(); }

std::string BackendPlain::identifier() const noexcept { return "Plain"; }

bool BackendPlain::bluetooth_enabled() {
    for (auto& adapter : adapters()) {
        if (adapter->is_powered()) return true;
    }
    return false;
}

SharedPtrVector<AdapterBase> BackendPlain::adapters() {
    auto environment = EnvironmentCore::active();
    if (!environment) environment = environment_;

    SharedPtrVector<AdapterBase> adapters;
    for (auto& adapter : environment->adapters()) {
        adapters.push_back(adapter);
    }
    return adapters;
}

}  // namespace SimpleBLE
