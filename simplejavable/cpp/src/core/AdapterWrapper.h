#pragma once
#include <simpleble/SimpleBLE.h>

#include <cstdint>

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

    // NOTE: All Dongls share the same identifier, and WinRT creates new adapter objects on every enumeration,
    // so we use the identifier and the address together to recognize the same adapter.
    static int64_t getHash(SimpleBLE::Adapter& adapter);

  private:
    SimpleBLE::Adapter _adapter;
};
