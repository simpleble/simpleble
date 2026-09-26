#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <vector>

#include <chrono>
#include <condition_variable>
#include <mutex>

#include "Wire.h"

#include "protocol/d2h.pb.h"
#include "protocol/h2d.pb.h"

namespace SimpleBLE {
namespace Dongl {
namespace Serial {

class ProtocolBase {
  public:
    ProtocolBase(const std::string& device_path);
    ~ProtocolBase();

    /**
     * @brief Sends a command synchronously and waits for the response.
     * Only one exchange can be pending at a time.
     *
     * @param command The command to send.
     * @param timeout How long to wait for the response.
     * @return The response when it arrives.
     * @throws std::runtime_error if another exchange is already pending or if timeout occurs.
     */
    dongl_Response exchange(dongl_Command command, std::chrono::milliseconds timeout = std::chrono::milliseconds(1000));

    /**
     * @brief Sets the callback for received events.
     *
     * @param callback Function to call when an event is received.
     */
    void set_event_callback(std::function<void(const dongl_Event&)> callback);

  private:
    std::unique_ptr<Wire> _wire;
    std::function<void(const dongl_Event&)> _event_callback;
    std::mutex _event_mutex;

    // Each command carries an id that its response echoes. Only a response with the id being waited on is accepted,
    // so a late response to an earlier, timed-out command is dropped instead of being returned as this one's result.
    uint8_t _last_id = 0;
    std::optional<uint8_t> _expected_id;
    std::optional<dongl_Response> _pending_response;
    std::condition_variable _response_cv;
    std::mutex _pending_mutex;
    std::mutex _exchange_mutex;
};

}  // namespace Serial
}  // namespace Dongl
}  // namespace SimpleBLE
