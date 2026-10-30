/*
 * Thread-safe map of callbacks with custom comparator support
 */
#ifndef KVN_SAFE_CALLBACK_MAP_HPP
#define KVN_SAFE_CALLBACK_MAP_HPP

#include <functional>
#include <map>
#include <mutex>

namespace kvn {

template <typename _Key, typename _Signature, typename _Compare = std::less<_Key>>
class safe_callback_map;

// Callbacks run under the map's lock, so once load(), unload() or clear() returns on another thread, a replaced or
// removed callback is no longer running. A callback can change its own entry, as it runs from a copy.
template <typename _Key, typename _Res, typename... _ArgTypes, typename _Compare>
class safe_callback_map<_Key, _Res(_ArgTypes...), _Compare> {
  public:
    safe_callback_map() = default;
    explicit safe_callback_map(const _Compare& comp) : _map(comp) {}

    // Remove copy constructor and copy assignment
    safe_callback_map(const safe_callback_map&) = delete;
    safe_callback_map& operator=(const safe_callback_map&) = delete;

    void load(const _Key& key, std::function<_Res(_ArgTypes...)> callback) {
        std::scoped_lock lock(_mutex);
        if (callback) {
            _map.insert_or_assign(key, std::move(callback));
        } else {
            _map.erase(key);
        }
    }

    void unload(const _Key& key) {
        std::scoped_lock lock(_mutex);
        _map.erase(key);
    }

    void clear() {
        std::scoped_lock lock(_mutex);
        _map.clear();
    }

    // Exceptions from the callback propagate to the caller.
    _Res call(const _Key& key, _ArgTypes... arguments) {
        std::scoped_lock lock(_mutex);
        auto it = _map.find(key);
        if (it == _map.end()) {
            return _Res();
        }

        std::function<_Res(_ArgTypes...)> callback = it->second;
        return callback(std::forward<_ArgTypes>(arguments)...);
    }

  protected:
    std::recursive_mutex _mutex;
    std::map<_Key, std::function<_Res(_ArgTypes...)>, _Compare> _map;
};

}  // namespace kvn

#endif  // KVN_SAFE_CALLBACK_MAP_HPP
