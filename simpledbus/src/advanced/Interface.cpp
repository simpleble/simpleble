#include <simpledbus/advanced/Interface.h>
#include <simpledbus/base/Exceptions.h>

using namespace SimpleDBus;

Interface::Interface(std::shared_ptr<Connection> conn, const std::string& bus_name, const std::string& path,
                     const std::string& interface_name)
    : _conn(conn), _bus_name(bus_name), _path(path), _interface_name(interface_name), _loaded(true) {}


// ----- PROPERTY INNER CLASS -----

template<typename T>
Interface::Property<T>::Property(Interface& interface, const std::string& name)
    : _interface(interface), _name(name) {}


template<typename T>
T Interface::Property<T>::get() {
     std::scoped_lock lock(_interface._property_update_mutex);
    return _interface._properties[_name].template get<T>();
}

template<>
std::vector<std::string> Interface::Property<std::vector<std::string>>::get() {
    std::scoped_lock lock(_interface._property_update_mutex);

    std::vector<std::string> items;
    for (SimpleDBus::Holder& item : _interface._properties[_name].get_array()) {
        items.push_back(item.get_string());
    }

    return items;
}


template<typename T>
T Interface::Property<T>::refresh_and_get() {
    _interface.property_refresh(_name);
    return Interface::Property<T>::get();
}

template<typename T>
void Interface::Property<T>::set(T value) {
    std::scoped_lock lock(_interface._property_update_mutex);
    _interface.property_set(_name, SimpleDBus::Holder::create<T>(value));
}


// ----- LIFE CYCLE -----

template<typename T>
Interface::Property<T> Interface::create_property(const std::string& name) {
    return Property<T>(*this, name);
}

template<typename T>
Interface::CachedProperty<T> Interface::create_cached_property(const std::string& name) {
    return CachedProperty<T>(*this, name);
}

void Interface::load(Holder options) {
    _property_update_mutex.lock();
    auto changed_options = options.get_dict_string();
    for (auto& [name, value] : changed_options) {
        _properties[name] = value;
        _property_valid_map[name] = true;
    }
    _property_update_mutex.unlock();

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

Holder Interface::property_get_all() {
    Message query_msg = Message::create_method_call(_bus_name, _path, "org.freedesktop.DBus.Properties", "GetAll");

    Holder h_interface = Holder::create_string(_interface_name);
    query_msg.append_argument(h_interface, "s");

    Message reply_msg = _conn->send_with_reply_and_block(query_msg);
    Holder result = reply_msg.extract();
    return result;
}

Holder Interface::property_get(const std::string& property_name) {
    Message query_msg = Message::create_method_call(_bus_name, _path, "org.freedesktop.DBus.Properties", "Get");

    Holder h_interface = Holder::create_string(_interface_name);
    query_msg.append_argument(h_interface, "s");

    Holder h_name = Holder::create_string(property_name);
    query_msg.append_argument(h_name, "s");

    Message reply_msg = _conn->send_with_reply_and_block(query_msg);
    Holder result = reply_msg.extract();
    return result;
}

void Interface::property_set(const std::string& property_name, const Holder& value) {
    Message query_msg = Message::create_method_call(_bus_name, _path, "org.freedesktop.DBus.Properties", "Set");

    Holder h_interface = Holder::create_string(_interface_name);
    query_msg.append_argument(h_interface, "s");

    Holder h_name = Holder::create_string(property_name);
    query_msg.append_argument(h_name, "s");

    query_msg.append_argument(value, "v");

    _conn->send_with_reply_and_block(query_msg);
}

void Interface::property_refresh(const std::string& property_name) {
    if (!_loaded || !_property_valid_map[property_name]) {
        return;
    }

    bool cb_property_changed_required = false;
    _property_update_mutex.lock();
    try {
        // NOTE: Due to the way Bluez handles underlying devices and the fact that
        //       they can be removed before the callback reaches back (race condition),
        //       `property_get` can sometimes fail. Because of this, the update
        //       statement is surrounded by a try-catch statement.
        Holder property_latest = property_get(property_name);
        _property_valid_map[property_name] = true;
        if (_properties[property_name] != property_latest) {
            _properties[property_name] = property_latest;
            cb_property_changed_required = true;
        }
    } catch (const Exception::SendFailed& e) {
        _property_valid_map[property_name] = true;
    }
    _property_update_mutex.unlock();

    if (cb_property_changed_required) {
        property_changed(property_name);
    }
}

void Interface::property_changed(std::string option_name) {}

// ----- SIGNALS -----

void Interface::signal_property_changed(Holder changed_properties, Holder invalidated_properties) {
    _property_update_mutex.lock();
    auto changed_options = changed_properties.get_dict_string();
    for (auto& [name, value] : changed_options) {
        _properties[name] = value;
        _property_valid_map[name] = true;
    }

    auto removed_options = invalidated_properties.get_array();
    for (auto& removed_option : removed_options) {
        _property_valid_map[removed_option.get_string()] = false;
    }
    _property_update_mutex.unlock();

    // Once all properties have been updated, notify the user.
    for (auto& [name, value] : changed_options) {
        property_changed(name);
    }
}

// ----- MESSAGES -----

void Interface::message_handle(Message& msg) {}
