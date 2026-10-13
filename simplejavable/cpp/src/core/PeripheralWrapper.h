#pragma once
#include <simpleble/SimpleBLE.h>

#include <cstdint>
#include <memory>

#include "org/simplejavable/PeripheralCallback.h"

class PeripheralWrapper {
  public:
    explicit PeripheralWrapper(const SimpleBLE::Peripheral& peripheral);

    SimpleBLE::Peripheral& get();
    const SimpleBLE::Peripheral& get() const;

    void setCallback(jobject callback);

    // Registers or removes the Dongl pairing callbacks, which forward to the Java callback.
    void setPasskeyRequestCallback(bool enabled);
    void setPasskeyDisplayCallback(bool enabled);
    void setNumericComparisonCallback(bool enabled);

    static int64_t getHash(SimpleBLE::Peripheral& peripheral);

  private:
    SimpleBLE::Peripheral _peripheral;
    std::shared_ptr<Org::SimpleJavaBLE::PeripheralCallback> _callback;
};
