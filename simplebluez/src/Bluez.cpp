#include <simplebluez/Bluez.h>
#include <simpledbus/interfaces/ObjectManager.h>

using namespace SimpleBluez;

#ifdef SIMPLEBLUEZ_USE_SESSION_DBUS
#define DBUS_BUS DBUS_BUS_SESSION
#else
#define DBUS_BUS DBUS_BUS_SYSTEM
#endif

Bluez::Bluez() : _conn(std::make_shared<SimpleDBus::Connection>(DBUS_BUS)) {

}

Bluez::~Bluez() {
    if (_conn->is_initialized()) {
        _conn->remove_match("type='signal',sender='org.bluez'");
    }
}

void Bluez::init() {
    _conn->init();
    _conn->add_match("type='signal',sender='org.bluez'");

    _bluez_root = std::make_shared<BluezRoot>(_conn, "org.bluez", "/");
    _bluez_root->load_managed_objects();
}


void Bluez::run_async() {
    _conn->read_write_dispatch();
}

std::vector<std::shared_ptr<Adapter>> Bluez::get_adapters() {
    return _bluez_root->get_adapters();
}

std::shared_ptr<Agent> Bluez::get_agent() { return _bluez_root->get_agent(); }

void Bluez::register_agent() { _bluez_root->register_agent(); }

