#pragma once

#include <memory>
#include <stdexcept>
#include <string>

#include <simpleble/export.h>

#include "Error.h"
#include "Types.h"

namespace SimpleBLE {

namespace Exception {

class SIMPLEBLE_EXPORT BaseException : public std::runtime_error {
  public:
    BaseException(const std::string& __arg) : std::runtime_error(__arg) {}

    /**
     * @brief Creates an owned error containing this exception's diagnostic details.
     * @return An error that remains valid independently of the exception.
     * @throws std::bad_alloc If the error cannot be allocated.
     */
    virtual std::unique_ptr<Error> make_error() const;
};

class SIMPLEBLE_EXPORT NotInitialized : public BaseException {
  public:
    NotInitialized();
    std::unique_ptr<Error> make_error() const override;
};

class SIMPLEBLE_EXPORT NotConnected : public BaseException {
  public:
    NotConnected();
    std::unique_ptr<Error> make_error() const override;
};

class SIMPLEBLE_EXPORT InvalidReference : public BaseException {
  public:
    InvalidReference();
    std::unique_ptr<Error> make_error() const override;
};

class SIMPLEBLE_EXPORT ServiceNotFound : public BaseException {
  public:
    ServiceNotFound(BluetoothUUID uuid);
    std::unique_ptr<Error> make_error() const override;

    /**
     * @brief Retrieves the missing service UUID.
     * @return A reference to the UUID owned by this exception.
     */
    const BluetoothUUID& uuid() const noexcept;

  private:
    BluetoothUUID uuid_;
};

class SIMPLEBLE_EXPORT CharacteristicNotFound : public BaseException {
  public:
    CharacteristicNotFound(BluetoothUUID uuid);
    std::unique_ptr<Error> make_error() const override;

    /**
     * @brief Retrieves the missing characteristic UUID.
     * @return A reference to the UUID owned by this exception.
     */
    const BluetoothUUID& uuid() const noexcept;

  private:
    BluetoothUUID uuid_;
};

class SIMPLEBLE_EXPORT DescriptorNotFound : public BaseException {
  public:
    DescriptorNotFound(BluetoothUUID uuid);
    std::unique_ptr<Error> make_error() const override;

    /**
     * @brief Retrieves the missing descriptor UUID.
     * @return A reference to the UUID owned by this exception.
     */
    const BluetoothUUID& uuid() const noexcept;

  private:
    BluetoothUUID uuid_;
};

class SIMPLEBLE_EXPORT OperationNotSupported : public BaseException {
  public:
    OperationNotSupported();
    OperationNotSupported(const std::string& operation, const BluetoothUUID& characteristic_uuid);
    std::unique_ptr<Error> make_error() const override;
};

class SIMPLEBLE_EXPORT OperationFailed : public BaseException {
  public:
    OperationFailed();
    OperationFailed(const std::string& err_msg);
    std::unique_ptr<Error> make_error() const override;
};

class SIMPLEBLE_EXPORT WinRTException : public BaseException {
  public:
    WinRTException(int32_t err_code, const std::string& err_msg);
    std::unique_ptr<Error> make_error() const override;

    /**
     * @brief Retrieves the native WinRT error code.
     * @return The original signed 32-bit HRESULT value.
     */
    int32_t code() const noexcept;

  private:
    int32_t code_;
};

class SIMPLEBLE_EXPORT WinRTAccessDenied : public WinRTException {
  public:
    WinRTAccessDenied(int32_t err_code, const std::string& err_msg);
    std::unique_ptr<Error> make_error() const override;
};

class SIMPLEBLE_EXPORT CoreBluetoothException : public BaseException {
  public:
    CoreBluetoothException(const std::string& err_msg);
    std::unique_ptr<Error> make_error() const override;
};

}  // namespace Exception

}  // namespace SimpleBLE
