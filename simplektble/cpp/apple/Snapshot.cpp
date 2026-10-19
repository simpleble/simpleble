#include <simplektble/snapshot.h>

#include <simplecble/error.h>
#include <simpleble/Advanced.h>
#include <simpleble/SimpleBLE.h>

#include <cstdlib>
#include <memory>
#include <string>
#include <vector>

// NOTE: A SimpleCBLE handle points to the public SimpleBLE object it wraps.

namespace {

template <typename List>
simpleble_peripheral_t* snapshot(simpleble_adapter_t handle, size_t* out_count, simpleble_error_t** out_error,
                                 List list) {
    simpleble_error_release(out_error);
    *out_count = 0;

    if (handle == nullptr) {
        *out_error = new SimpleBLE::Error(SimpleBLE::ErrorCode::INVALID_ARGUMENT, "handle is NULL");
        return nullptr;
    }

    try {
        std::vector<std::unique_ptr<SimpleBLE::Peripheral>> peripherals;
        for (auto& peripheral : list(*static_cast<SimpleBLE::Adapter*>(handle))) {
            peripherals.push_back(std::make_unique<SimpleBLE::Peripheral>(peripheral));
        }
        if (peripherals.empty()) return nullptr;

        auto handles =
            static_cast<simpleble_peripheral_t*>(std::malloc(peripherals.size() * sizeof(simpleble_peripheral_t)));
        if (handles == nullptr) {
            *out_error = new SimpleBLE::Error(SimpleBLE::ErrorCode::OUT_OF_MEMORY, "Failed to allocate the handles");
            return nullptr;
        }
        for (size_t index = 0; index < peripherals.size(); index++) {
            handles[index] = peripherals[index].release();
        }
        *out_count = peripherals.size();
        return handles;
    } catch (const SimpleBLE::Exception::BaseException& e) {
        *out_error = e.make_error().release();
    } catch (const std::exception& e) {
        *out_error = new SimpleBLE::Error(SimpleBLE::ErrorCode::UNCLASSIFIED_EXCEPTION, e.what());
    } catch (...) {
        *out_error = new SimpleBLE::Error(SimpleBLE::ErrorCode::UNCLASSIFIED_EXCEPTION, "Unknown exception");
    }
    return nullptr;
}

}  // namespace

simpleble_peripheral_t* simplektble_adapter_retrieve_cached_peripherals(simpleble_adapter_t handle,
                                                                        const char* const* identifiers,
                                                                        size_t identifier_count, size_t* out_count,
                                                                        simpleble_error_t** out_error) {
    auto retrieve = [identifiers, identifier_count](SimpleBLE::Adapter& adapter) -> std::vector<SimpleBLE::Peripheral> {
        std::vector<SimpleBLE::BluetoothAddress> addresses;
        for (size_t index = 0; index < identifier_count; index++) {
            if (identifiers[index] != nullptr) addresses.emplace_back(identifiers[index]);
        }
#if SIMPLEBLE_BACKEND_MACOS
        return SimpleBLE::Advanced::MacOS::retrieve_cached_peripherals(adapter, addresses);
#elif SIMPLEBLE_BACKEND_IOS
        return SimpleBLE::Advanced::iOS::retrieve_cached_peripherals(adapter, addresses);
#else
        (void)adapter;
        throw SimpleBLE::Exception::OperationNotSupported();
#endif
    };
    return snapshot(handle, out_count, out_error, retrieve);
}
