#pragma once

#include <simplecble/error.h>

#include <simpleble/Exceptions.h>

#include <cstddef>
#include <exception>
#include <vector>

// NOTE: A list handle points to a std::vector holding a collection as it was read, so counting and indexing it
// doesn't read the collection again.
namespace SimpleCBLE::List {

// Stores the collection that read() returns in a new list and returns its size.
template <typename Read>
size_t read(void** out_list, simpleble_error_t** out_error, Read read) {
    simpleble_error_release(out_error);

    if (out_list == nullptr) {
        *out_error = new SimpleBLE::Error(SimpleBLE::ErrorCode::INVALID_ARGUMENT, "out_list is NULL");
        return 0;
    }
    *out_list = nullptr;

    try {
        auto* list = new auto(read());
        *out_list = list;
        return list->size();
    } catch (const SimpleBLE::Exception::BaseException& e) {
        *out_error = e.make_error().release();
    } catch (const std::exception& e) {
        *out_error = new SimpleBLE::Error(SimpleBLE::ErrorCode::UNCLASSIFIED_EXCEPTION, e.what());
    } catch (...) {
        *out_error = new SimpleBLE::Error(SimpleBLE::ErrorCode::UNCLASSIFIED_EXCEPTION, "Unknown exception");
    }

    return 0;
}

// Stores the collection that read() returns for the object behind a handle in a new list and returns its size.
template <typename Object, typename Read>
size_t read(void* handle, void** out_list, simpleble_error_t** out_error, Read read) {
    if (handle == nullptr) {
        simpleble_error_release(out_error);
        if (out_list != nullptr) *out_list = nullptr;
        *out_error = new SimpleBLE::Error(SimpleBLE::ErrorCode::INVALID_ARGUMENT, "handle is NULL");
        return 0;
    }

    return List::read(out_list, out_error, [&]() { return read(*static_cast<Object*>(handle)); });
}

template <typename T>
size_t count(void* list) {
    if (list == nullptr) return 0;
    return static_cast<std::vector<T>*>(list)->size();
}

// Returns the entry at index, or reports an INVALID_ARGUMENT error and returns NULL.
template <typename T>
T* at(void* list, size_t index, simpleble_error_t** out_error) {
    if (list == nullptr) {
        *out_error = new SimpleBLE::Error(SimpleBLE::ErrorCode::INVALID_ARGUMENT, "list is NULL");
        return nullptr;
    }

    auto& entries = *static_cast<std::vector<T>*>(list);
    if (index >= entries.size()) {
        *out_error = new SimpleBLE::Error(SimpleBLE::ErrorCode::INVALID_ARGUMENT, "index is out of range");
        return nullptr;
    }
    return &entries[index];
}

// Returns a new copy of the entry at index for the caller to own, or NULL on failure.
template <typename T>
T* get(void* list, size_t index, simpleble_error_t** out_error) {
    simpleble_error_release(out_error);

    T* entry = at<T>(list, index, out_error);
    if (entry == nullptr) return nullptr;

    try {
        return new T(*entry);
    } catch (const std::exception& e) {
        *out_error = new SimpleBLE::Error(SimpleBLE::ErrorCode::OUT_OF_MEMORY, e.what());
    }

    return nullptr;
}

template <typename T>
void release(void* list) {
    delete static_cast<std::vector<T>*>(list);
}

}  // namespace SimpleCBLE::List
