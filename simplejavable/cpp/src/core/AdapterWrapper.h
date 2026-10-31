#pragma once
#include <simpleble/SimpleBLE.h>

#include <cstdint>
#include <memory>

#include "simplejni/Common.hpp"
#include "simplejni/Registry.hpp"
#include "org/simplejavable/AdapterCallback.h"

class AdapterWrapper {
  public:
    explicit AdapterWrapper(const SimpleBLE::Adapter& adapter);
    ~AdapterWrapper();

    // Accessor to the underlying adapter
    SimpleBLE::Adapter& get();
    const SimpleBLE::Adapter& get() const;

    void setCallback(jobject callback);

#if defined(__linux__) && !defined(__ANDROID__)
    // Registers or removes the Linux pairing callbacks, which forward to the Java callback.
    void setPasskeyRequestCallback(bool enabled);
    void setPasskeyDisplayCallback(bool enabled);
    void setNumericComparisonCallback(bool enabled);
    void setPairingAuthorizationCallback(bool enabled);
#endif

    // NOTE: All Dongls share the same identifier, and WinRT creates new adapter objects on every enumeration,
    // so we use the identifier and the address together to recognize the same adapter.
    static int64_t getHash(SimpleBLE::Adapter& adapter);

  private:
    SimpleBLE::Adapter _adapter;
    std::shared_ptr<Org::SimpleJavaBLE::AdapterCallback> _callback;
};
