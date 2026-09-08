#pragma once

#include <stdexcept>
#include <string>

#include <simpleble/export.h>

#include "Types.h"

namespace SimpleBLE {

namespace Exception {

class SIMPLEBLE_EXPORT BaseException : public std::runtime_error {
  public:
    BaseException(const std::string& __arg) : std::runtime_error(__arg) {}
};

class SIMPLEBLE_EXPORT NotInitialized : public BaseException {
  public:
    NotInitialized();
};

class SIMPLEBLE_EXPORT NotConnected : public BaseException {
  public:
    NotConnected();
};

class SIMPLEBLE_EXPORT InvalidReference : public BaseException {
  public:
    InvalidReference();
};

class SIMPLEBLE_EXPORT ServiceNotFound : public BaseException {
  public:
    ServiceNotFound(BluetoothUUID uuid);

    /**
     * @brief Retrieves the missing service UUID.
     * @return A reference to the UUID owned by this exception.
     */
    const BluetoothUUID& uuid() const noexcept { return uuid_; }

  private:
    BluetoothUUID uuid_;
};

class SIMPLEBLE_EXPORT CharacteristicNotFound : public BaseException {
  public:
    CharacteristicNotFound(BluetoothUUID uuid);

    /**
     * @brief Retrieves the missing characteristic UUID.
     * @return A reference to the UUID owned by this exception.
     */
    const BluetoothUUID& uuid() const noexcept { return uuid_; }

  private:
    BluetoothUUID uuid_;
};

class SIMPLEBLE_EXPORT DescriptorNotFound : public BaseException {
  public:
    DescriptorNotFound(BluetoothUUID uuid);

    /**
     * @brief Retrieves the missing descriptor UUID.
     * @return A reference to the UUID owned by this exception.
     */
    const BluetoothUUID& uuid() const noexcept { return uuid_; }

  private:
    BluetoothUUID uuid_;
};

class SIMPLEBLE_EXPORT OperationNotSupported : public BaseException {
  public:
    OperationNotSupported();
    OperationNotSupported(const std::string& operation, const BluetoothUUID& characteristic_uuid);
};

class SIMPLEBLE_EXPORT OperationFailed : public BaseException {
  public:
    OperationFailed();
    OperationFailed(const std::string& err_msg);
};

class SIMPLEBLE_EXPORT WinRTException : public BaseException {
  public:
    WinRTException(int32_t err_code, const std::string& err_msg);

    /**
     * @brief Retrieves the native WinRT error code.
     * @return The original signed 32-bit HRESULT value.
     */
    int32_t code() const noexcept { return code_; }

  private:
    int32_t code_;
};

class SIMPLEBLE_EXPORT WinRTAccessDenied : public WinRTException {
  public:
    WinRTAccessDenied(int32_t err_code, const std::string& err_msg);
};

class SIMPLEBLE_EXPORT CoreBluetoothException : public BaseException {
  public:
    CoreBluetoothException(const std::string& err_msg);
};

}  // namespace Exception

}  // namespace SimpleBLE
