#pragma once

#include <simpledbus/base/Connection.h>

#include <atomic>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include "kvn/kvn_safe_callback.hpp"

namespace SimpleDBus {

class Proxy;

class Interface;
class PropertyBase;
template <typename T>
class Property;
template <typename T>
class CustomProperty;

class Interface {
  public:
    class PropertyBase {
      public:
        PropertyBase(Interface& interface, const std::string& name) : _interface(interface), _name(name), _valid(false) {}

        virtual ~PropertyBase() = default;

        // Delete copy, allow move
        PropertyBase(const PropertyBase&) = delete;
        PropertyBase& operator=(const PropertyBase&) = delete;
        PropertyBase(PropertyBase&&) noexcept = delete;
        PropertyBase& operator=(PropertyBase&&) noexcept = delete;

        PropertyBase& refresh() {
            _interface.property_refresh(_name);
            return *this;
        }

        void emit() {
            _interface.property_emit(_name, get());
        }

        Holder get() const {
            std::scoped_lock lock(_mutex);
            return _value;
        }

        PropertyBase& set(Holder value) {
            std::scoped_lock lock(_mutex);
            _value = value;
            _valid = true;
            notify_changed();
            return *this;
        }

        bool valid() {
            std::scoped_lock lock(_mutex);
            return _valid;
        }

        void invalidate() {
            std::scoped_lock lock(_mutex);
            _valid = false;
        }

        bool operator==(const Holder& other) const {
            std::scoped_lock lock(_mutex);
            return _value == other;
        }

        bool operator!=(const Holder& other) const { return !(*this == other); }

        virtual void notify_changed() {}

      protected:
        Interface& _interface;
        const std::string _name;
        mutable std::recursive_mutex _mutex;
        Holder _value;
        bool _valid;
    };

    template <typename T>
    class Property : public PropertyBase {
      public:
        Property(Interface& interface, const std::string& name) : PropertyBase(interface, name) {}

        virtual ~Property() { on_changed.unload(); }

        T operator()() const { return get(); }
        operator T() const { return get(); }
        void operator()(const T& value) { set(value); }

        using PropertyBase::set;

        Property& refresh() {
            PropertyBase::refresh();
            return *this;
        }

        T get() const {
            std::scoped_lock lock(_mutex);
            return _value.get<T>();
        }

        Property& set(T value) {
            std::scoped_lock lock(_mutex);
            _value = Holder::create<T>(value);
            _valid = true;
            return *this;
        }

        kvn::safe_callback<void(T)> on_changed;

        void notify_changed() override { on_changed(get()); }
    };

    template <typename T>
    class CustomProperty : public PropertyBase {
      public:
        CustomProperty(Interface& interface, const std::string& name, std::function<Holder(T)> to_holder,
                       std::function<T(Holder)> from_holder)
            : PropertyBase(interface, name), _to_holder(to_holder), _from_holder(from_holder) {}

        virtual ~CustomProperty() { on_changed.unload(); }

        T operator()() const { return get(); }
        operator T() const { return get(); }
        void operator()(const T& value) { set(value); }

        using PropertyBase::set;

        CustomProperty& refresh() {
            PropertyBase::refresh();
            return *this;
        }

        T get() const {
            std::scoped_lock lock(_mutex);
            return _from_holder(_value);
        }

        CustomProperty& set(T value) {
            std::scoped_lock lock(_mutex);
            _value = _to_holder(value);
            _valid = true;
            return *this;
        }

        kvn::safe_callback<void(T)> on_changed;

        void notify_changed() override { on_changed(get()); }

      private:
        std::function<Holder(T)> _to_holder;
        std::function<T(Holder)> _from_holder;
    };

    Interface(std::shared_ptr<Connection> conn, std::shared_ptr<Proxy> proxy, const std::string& interface_name);

    virtual ~Interface() = default;

    // ----- LIFE CYCLE -----
    void load(Holder options);
    void unload();
    bool is_loaded() const;

    // ----- METHODS -----
    Message create_method_call(const std::string& method_name);

    // ----- PROPERTIES -----
    bool property_exists(const std::string& property_name);
    void property_refresh(const std::string& property_name);
    void property_emit(const std::string& property_name, Holder value);

    // ----- MESSAGES -----
    virtual void message_handle(Message& msg) {}

    // ----- HANDLES -----
    void handle_properties_changed(Holder changed_properties, Holder invalidated_properties);
    void handle_property_set(std::string property_name, Holder value);
    Holder handle_property_get(std::string property_name);
    Holder handle_property_get_all();

    template <typename T>
    Property<T>& property(const std::string& name) {
        std::unique_ptr<PropertyBase> property_ptr = std::make_unique<Property<T>>(*this, name);
        Property<T>& property = dynamic_cast<Property<T>&>(*property_ptr);
        _properties.emplace(name, std::move(property_ptr));
        return property;
    }

    template <typename T>
    CustomProperty<T>& property(const std::string& name, std::function<Holder(T)> to_holder, std::function<T(Holder)> from_holder) {
        std::unique_ptr<PropertyBase> property_ptr = std::make_unique<CustomProperty<T>>(*this, name, to_holder, from_holder);
        CustomProperty<T>& property = dynamic_cast<CustomProperty<T>&>(*property_ptr);
        _properties.emplace(name, std::move(property_ptr));
        return property;
    }

  protected:
    std::atomic_bool _loaded{true};

    std::string _path;
    std::string _bus_name;
    std::string _interface_name;
    std::shared_ptr<Connection> _conn;
    std::weak_ptr<Proxy> _proxy;

    std::shared_ptr<Proxy> proxy() const;

    // IMPORTANT: Never erase from _property_bases during lifetime of Interface.
    // "Removal" of a property means: invalidate it via invalidate() or set _valid = false.
    // The entry must remain so that permanent references in derived classes stay valid.
    std::map<std::string, std::unique_ptr<PropertyBase>> _properties;
};

}  // namespace SimpleDBus
