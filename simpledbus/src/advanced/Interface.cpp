#include <simpledbus/advanced/Interface.h>
#include <simpledbus/advanced/Proxy.h>
#include <simpledbus/base/Exceptions.h>
#include <simpledbus/interfaces/Properties.h>

using namespace SimpleDBus;

Interface::Interface(std::shared_ptr<Connection> conn, std::shared_ptr<Proxy> proxy, const std::string& interface_name)
    : _conn(conn),
      _proxy(proxy),
      _bus_name(proxy->bus_name()),
      _path(proxy->path()),
      _interface_name(interface_name),
      _loaded(true) {}

std::shared_ptr<Proxy> Interface::proxy() const { return _proxy.lock(); }

// ----- LIFE CYCLE -----

void Interface::load(Holder options) {
    auto changed_options = options.get_dict_string();

    // NEW PROPERTY UPDATE
    // Note: Properties that have not been defined inside _property_bases will explicitly be ignored.
    for (auto& [name, value] : changed_options) {
        if (_property_bases.find(name) == _property_bases.end()) continue;

        _property_bases[name]->update(value);
    }

    // Notify the user of all properties that have been created.
    for (auto& [name, value] : changed_options) {
        property_changed(name);
    }

    _loaded = true;
}

void Interface::unload() { _loaded = false; }

bool Interface::is_loaded() const { return _loaded; }

// ----- METHODS -----

Message Interface::create_method_call(const std::string& method_name) {
    return Message::create_method_call(_bus_name, _path, _interface_name, method_name);
}

// ----- PROPERTIES -----

void Interface::property_refresh_new(const std::string& property_name) {
    if (!_loaded || _property_bases.count(property_name) == 0) {
        return;
    }

    bool cb_property_changed_required = false;

    try {
        // NOTE: Due to the way Bluez handles underlying devices and the fact that
        //       they can be removed before the callback reaches back (race condition),
        //       `property_get` can sometimes fail. Because of this, the update
        //       statement is surrounded by a try-catch statement.
        auto properties_interface = std::dynamic_pointer_cast<SimpleDBus::Interfaces::Properties>(
            proxy()->interface_get("org.freedesktop.DBus.Properties"));
        Holder property_latest = properties_interface->Get(_interface_name, property_name);

        if (*_property_bases[property_name] != property_latest) {
            _property_bases[property_name]->update(property_latest);
            cb_property_changed_required = true;
        }
    } catch (const Exception::SendFailed& e) {
        // TODO: Log error
    }

    if (cb_property_changed_required) {
        property_changed(property_name);
    }
}

bool Interface::property_exists(const std::string& property_name) {
    return _property_bases.count(property_name) > 0;
}

// ----- HANDLES -----

void Interface::handle_properties_changed(Holder changed_properties, Holder invalidated_properties) {
    auto changed_options = changed_properties.get_dict_string();
    for (auto& [name, value] : changed_options) {
        if (_property_bases.find(name) == _property_bases.end()) continue;

        _property_bases[name]->update(value);
    }

    auto removed_options = invalidated_properties.get_array();
    for (auto& removed_option : removed_options) {
        if (_property_bases.find(removed_option.get_string()) == _property_bases.end()) continue;

        _property_bases[removed_option.get_string()]->invalidate();
    }

    // Once all properties have been updated, notify the user.
    for (auto& [name, value] : changed_options) {
        property_changed(name);
    }
}

void Interface::handle_property_set(std::string property_name, Holder value) {
    if (_property_bases.find(property_name) == _property_bases.end()) return;

    _property_bases[property_name]->update(value);
    property_changed(property_name);
}

Holder Interface::handle_property_get(std::string property_name) {
    if (_property_bases.find(property_name) == _property_bases.end()) return Holder();

    return _property_bases[property_name]->get();
}

Holder Interface::handle_property_get_all() {
    Holder properties = Holder::create_dict();
    for (auto& [name, value] : _property_bases) {
        properties.dict_append(Holder::STRING, name, value->get());
    }
    return properties;
}