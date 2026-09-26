#include "fixture.h"
#include <simpleble/Backend.h>
#include <algorithm>
#include <cctype>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <thread>

namespace hitl {
Options Options::parse(int& argc, char** argv) {
    Options options;
    int kept = 1;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        auto pos = arg.find('=');
        auto key = arg.substr(0, pos);
        auto value = pos == std::string::npos ? "" : arg.substr(pos + 1);
        if ((key == "--control" || key == "--board" || key == "--probe" || key == "--adapter") && value.empty())
            throw std::runtime_error(key + " requires a value: " + key + "=<value>");
        if (key == "--control")
            options.control = value;
        else if (key == "--board")
            options.board = value;
        else if (key == "--probe" || key == "--adapter") {
            if (value.empty() || value.find_first_not_of("0123456789") != std::string::npos)
                throw std::runtime_error(key + " requires an unsigned integer");
            auto number = std::stoull(value);
            if (number > UINT32_MAX) throw std::runtime_error(key + " is too large");
            (key == "--probe" ? options.probe : options.adapter) = static_cast<unsigned>(number);
        } else
            argv[kept++] = argv[i];
    }
    argc = kept;
    argv[kept] = nullptr;
    if (options.control != "ble" && options.control != "rtt")
        throw std::runtime_error("Use --control=ble or --control=rtt");
    if (!options.board.empty()) {
        if (options.board.size() != 16 ||
            options.board.find_first_not_of("0123456789abcdefABCDEF") != std::string::npos)
            throw std::runtime_error("--board requires a 16-digit hexadecimal ID");
        std::transform(options.board.begin(), options.board.end(), options.board.begin(),
                       [](unsigned char c) { return std::tolower(c); });
    }
    if (options.probe && options.control != "rtt") throw std::runtime_error("--probe requires --control=rtt");
    return options;
}
std::string uuid(unsigned suffix) {
    std::ostringstream out;
    out << "7e57" << std::hex << std::setw(4) << std::setfill('0') << suffix << "-2e7a-4b6f-a1b0-9c8d6e5f4301";
    return out.str();
}
std::string hex(const Bytes& data) { return data.empty() ? "-" : data.toHex(); }
Bytes unhex(const std::string& text) {
    if (text == "-") return {};
    if (text.size() % 2 || text.find_first_not_of("0123456789abcdef") != std::string::npos)
        throw std::runtime_error("Invalid firmware hex payload");
    Bytes out;
    for (size_t i = 0; i < text.size(); i += 2)
        out.push_back(static_cast<uint8_t>(std::stoul(text.substr(i, 2), nullptr, 16)));
    return out;
}
Fields fields(const std::string& line) {
    Fields result;
    std::istringstream input(line);
    for (std::string word; input >> word;) {
        auto pos = word.find('=');
        if (pos != std::string::npos) result.emplace(word.substr(0, pos), word.substr(pos + 1));
    }
    return result;
}
void wait_until(const std::function<bool()>& predicate, std::chrono::milliseconds timeout) {
    auto deadline = std::chrono::steady_clock::now() + timeout;
    while (!predicate()) {
        if (std::chrono::steady_clock::now() >= deadline)
            throw std::runtime_error("Timed out waiting for fixture state");
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
}
Fixture::Fixture(const Options& options) : board(options.board) {
    // Temporarily hardcoded: the benches test SimpleBLE through a Dongl. --adapter indexes Dongl adapters only.
    // Enumerating only the Dongl backend keeps CoreBluetooth, and its permission prompt, out of the run.
    SimpleBLE::Config::Dongl::use_dongl_backend = true;
    std::vector<SimpleBLE::Adapter> adapters;
    for (auto& backend : SimpleBLE::Backend::get_backends()) {
        if (backend.identifier() != "Dongl") continue;
        if (!backend.bluetooth_enabled()) throw std::runtime_error("Bluetooth is disabled");
        adapters = backend.adapters();
    }
    if (options.adapter >= adapters.size()) throw std::runtime_error("Dongl adapter not found");
    adapter = adapters[options.adapter];
    if (options.control == "rtt") {
        rtt = std::make_unique<Rtt>(options.probe);
        if (!board.empty() && board != rtt->board) throw std::runtime_error("Probe is attached to a different board");
        board = rtt->board;
    }
}
Fixture::~Fixture() {
    try {
        if (peer.initialized()) {
            peer.set_callback_on_connected(nullptr);
            peer.set_callback_on_disconnected(nullptr);
            if (peer.is_connected()) peer.disconnect();
        }
    } catch (const std::exception& e) {
        std::cerr << "Fixture cleanup: " << e.what() << '\n';
    }
}
void Fixture::discover() {
    adapter.scan_for(2500);
    std::vector<SimpleBLE::Peripheral> matches;
    for (auto candidate : adapter.scan_get_results()) {
        auto name = candidate.identifier();
        if (name.size() != 12 || name.substr(0, 4) != "SBH-" || !candidate.is_connectable()) continue;
        std::string suffix = name.substr(4);
        std::transform(suffix.begin(), suffix.end(), suffix.begin(), [](unsigned char c) { return std::tolower(c); });
        if (!board.empty() && suffix != board.substr(8)) continue;
        auto services = candidate.services();
        if (std::none_of(services.begin(), services.end(), [](auto service) { return service.uuid() == uuid(1); }))
            continue;
        matches.push_back(candidate);
    }
    if (matches.size() != 1)
        throw std::runtime_error("Expected one fixture, found " + std::to_string(matches.size()) +
                                 "; use --board=<id> to select one");
    peer = matches.front();
}
void Fixture::connect() {
    peer.connect();
    // RTT scenarios can connect before the backend implements GATT reads.
    if (!rtt) {
        auto identity = control("HELLO");
        if (!board.empty() && board != identity.at("board")) throw std::runtime_error("BLE board identity mismatch");
        board = identity.at("board");
    }
}
void Fixture::disconnect() {
    peer.disconnect();
    wait_until([&] { return !peer.is_connected(); });
}
std::string exchange_ble(const std::string& request, const std::function<void(const Bytes&)>& write,
                         const std::function<Bytes()>& read) {
    if (request.empty() || request.size() > 1536 || request.back() != '\n')
        throw std::runtime_error("Invalid BLE control request");
    std::string response;
    // Fixed 20-byte writes work at the minimum ATT MTU and need no mtu() implementation.
    for (size_t offset = 0; offset < request.size(); offset += 19) {
        Bytes fragment{0};
        for (size_t i = offset; i < std::min(offset + 19, request.size()); ++i)
            fragment.push_back(static_cast<uint8_t>(request[i]));
        write(fragment);
    }
    unsigned length = 0;
    do {
        unsigned offset = static_cast<unsigned>(response.size());
        write(Bytes{1, static_cast<uint8_t>(offset), static_cast<uint8_t>(offset >> 8)});
        auto page = read();
        if (page.size() < 4 || page.size() > 20) throw std::runtime_error("Invalid control response page");
        unsigned total = page[0] | (unsigned(page[1]) << 8);
        unsigned position = page[2] | (unsigned(page[3]) << 8);
        if (!total || total > 1536 || offset >= total || position != offset || (offset && total != length) ||
            page.size() != 4 + std::min(16u, total - offset))
            throw std::runtime_error("Invalid control response length/offset");
        length = total;
        for (size_t i = 4; i < page.size(); ++i) response += static_cast<char>(page[i]);
    } while (response.size() < length);
    return response;
}
Fields Fixture::control(const std::string& command) {
    if (command.empty() || command.find_first_of("\r\n") != std::string::npos)
        throw std::runtime_error("Invalid control command");
    auto id = std::to_string(++request_id);
    auto request = id + " " + command + "\n";
    if (request.size() > 1536) throw std::runtime_error("Control command is too long");
    std::string response;
    if (rtt)
        response = rtt->exchange(request);
    else {
        response = exchange_ble(
            request, [&](const Bytes& data) { peer.write_request(uuid(0x11), uuid(0x12), data); },
            [&] { return peer.read(uuid(0x11), uuid(0x13)); });
    }
    std::cout << "> " << command << "\n< " << response;
    std::istringstream input(response);
    std::string received_id, status;
    input >> received_id >> status;
    if (received_id != id || status != "OK") throw std::runtime_error("Firmware rejected command: " + response);
    auto result = fields(response);
    if (command == "HELLO") {
        if (result.at("version") != "1") throw std::runtime_error("Unsupported fixture protocol");
        const auto& id = result.at("board");
        if (id.size() != 16 || id.find_first_not_of("0123456789abcdef") != std::string::npos)
            throw std::runtime_error("Invalid firmware board ID");
        if (!board.empty() && board != id) throw std::runtime_error("Firmware board identity mismatch");
    }
    return result;
}
void Fixture::reset(unsigned test_id, unsigned ceiling) {
    if (!peer.initialized()) discover();
    if (!peer.is_connected()) connect();
    control("RESET " + std::to_string(test_id));
    wait_until([&] { return !peer.is_connected(); });
    connect();
    control("MTU " + std::to_string(ceiling));
    disconnect();
    connect();
}
}  // namespace hitl
