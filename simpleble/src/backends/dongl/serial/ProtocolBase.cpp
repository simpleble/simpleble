#include "ProtocolBase.h"

using namespace SimpleBLE::Dongl::Serial;

#include "nanopb/pb_decode.h"
#include "nanopb/pb_encode.h"

#include <fmt/core.h>

#include "LoggingInternal.h"

ProtocolBase::ProtocolBase(const std::string& device_path) : _wire(std::make_unique<Wire>(device_path)) {
    // Set up the Wire packet callback to handle incoming packets
    _wire->set_packet_callback([this](const std::vector<uint8_t>& packet) {
        dongl_D2H d2h = dongl_D2H_init_zero;
        pb_istream_t stream = pb_istream_from_buffer(packet.data(), packet.size());
        if (!pb_decode(&stream, dongl_D2H_fields, &d2h)) {
            SIMPLEBLE_LOG_ERROR(fmt::format("Failed to decode Dongl packet: {}", PB_GET_ERROR(&stream)));
            return;
        }

        if (d2h.which_type == dongl_D2H_rsp_tag) {
            std::lock_guard<std::mutex> lock(_pending_mutex);
            if (_expected_id != d2h.type.rsp.id || _pending_response.has_value()) {
                SIMPLEBLE_LOG_WARN("Discarding a Dongl response to an earlier command");
                return;
            }
            _pending_response = d2h.type.rsp;
            _response_cv.notify_one();

        } else if (d2h.which_type == dongl_D2H_evt_tag) {
            std::lock_guard<std::mutex> lock(_event_mutex);
            if (_event_callback) {
                try {
                    _event_callback(d2h.type.evt);
                } catch (const std::exception& e) {
                    SIMPLEBLE_LOG_ERROR(fmt::format("Dongl event handler failed: {}", e.what()));
                } catch (...) {
                    SIMPLEBLE_LOG_ERROR("Dongl event handler failed with an unknown exception");
                }
            }
        }
    });

    _wire->set_error_callback([this](const Wire::Error& error) {
        SIMPLEBLE_LOG_WARN(fmt::format("Dongl wire error: {}", static_cast<int>(error)));
    });
}

ProtocolBase::~ProtocolBase() {
    // The reader callback captures this object, so stop and join its thread
    // before the callback state and mutexes are destroyed.
    _wire.reset();
}

dongl_Response ProtocolBase::exchange(dongl_Command command, std::chrono::milliseconds timeout) {
    // Serialize exchanges. Responses to earlier, timed-out commands carry a different id and are discarded.
    std::lock_guard<std::mutex> exchange_lock(_exchange_mutex);
    command.id = ++_last_id;

    // Clear any previous response and send the command
    {
        std::unique_lock<std::mutex> lock(_pending_mutex);
        _pending_response.reset();  // Ensure it's empty to indicate we're waiting
        _expected_id = command.id;
    }

    try {
        size_t command_size = 0;
        pb_get_encoded_size(&command_size, dongl_Command_fields, &command);

        std::vector<uint8_t> tx_buffer_raw(command_size);
        pb_ostream_t stream = pb_ostream_from_buffer(tx_buffer_raw.data(), tx_buffer_raw.size());
        bool status = pb_encode(&stream, dongl_Command_fields, &command);
        if (!status) {
            // TODO: Handle encoding failure
            throw std::runtime_error("Failed to encode command");
        }

        _wire->send_packet(tx_buffer_raw);
    } catch (const std::runtime_error& e) {
        // If sending fails, no cleanup needed since we didn't set pending_response
        throw;
    }

    // Wait for the response
    {
        std::unique_lock<std::mutex> lock(_pending_mutex);
        const bool received = _response_cv.wait_for(lock, timeout, [this]() { return _pending_response.has_value(); });
        _expected_id.reset();
        if (received) {
            dongl_Response response = *_pending_response;
            _pending_response.reset();
            return response;
        } else {
            // Timeout occurred, reset pending state
            _pending_response.reset();
            throw std::runtime_error("Timeout waiting for response");
        }
    }
}

void ProtocolBase::set_event_callback(std::function<void(const dongl_Event&)> callback) {
    std::lock_guard<std::mutex> lock(_event_mutex);
    _event_callback = std::move(callback);
}
