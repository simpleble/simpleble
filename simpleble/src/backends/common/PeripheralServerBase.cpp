#include "PeripheralServerBase.h"

namespace SimpleBLE {

void PeripheralServerBase::set_callback_on_started(std::function<void()> on_started) {
    if (on_started) {
        _callback_on_started.load(on_started);
    } else {
        _callback_on_started.unload();
    }
}

void PeripheralServerBase::set_callback_on_stopped(std::function<void()> on_stopped) {
    if (on_stopped) {
        _callback_on_stopped.load(on_stopped);
    } else {
        _callback_on_stopped.unload();
    }
}

void PeripheralServerBase::set_callback_on_advertising_started(std::function<void()> on_started) {
    if (on_started) {
        _callback_on_advertising_started.load(on_started);
    } else {
        _callback_on_advertising_started.unload();
    }
}

void PeripheralServerBase::set_callback_on_advertising_stopped(std::function<void()> on_stopped) {
    if (on_stopped) {
        _callback_on_advertising_stopped.load(on_stopped);
    } else {
        _callback_on_advertising_stopped.unload();
    }
}

void PeripheralServerBase::set_callback_on_central_connected(std::function<void(Central)> on_connected) {
    if (on_connected) {
        _callback_on_central_connected.load(on_connected);
    } else {
        _callback_on_central_connected.unload();
    }
}

void PeripheralServerBase::set_callback_on_central_disconnected(std::function<void(Central)> on_disconnected) {
    if (on_disconnected) {
        _callback_on_central_disconnected.load(on_disconnected);
    } else {
        _callback_on_central_disconnected.unload();
    }
}

}  // namespace SimpleBLE
