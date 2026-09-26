#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <thread>
#include "fixture.h"
#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

namespace hitl {
// SEGGER's C API is loaded only for --control=rtt; BLE runs need no J-Link installation.
struct Rtt::Impl {
#ifdef _WIN32
    HMODULE library = nullptr;
#else
    void* library = nullptr;
#endif
    bool opened = false, started = false;
    std::string pending;
    template <typename T>
    T function(const char* name) {
#ifdef _WIN32
        auto symbol = GetProcAddress(library, name);
#else
        auto symbol = dlsym(library, name);
#endif
        if (!symbol) throw std::runtime_error(std::string("Missing J-Link function: ") + name);
        return reinterpret_cast<T>(symbol);
    }
    Impl() {
        auto path = std::getenv("JLINK_LIBRARY");
#ifdef _WIN32
        library = LoadLibraryA(path ? path : "JLink_x64.dll");
#elif defined(__APPLE__)
        library = dlopen(path ? path : "/Applications/SEGGER/JLink/libjlinkarm.dylib", RTLD_NOW | RTLD_LOCAL);
#else
        library = dlopen(path ? path : "libjlinkarm.so", RTLD_NOW | RTLD_LOCAL);
#endif
        if (!library) throw std::runtime_error("Cannot load J-Link library; set JLINK_LIBRARY to its full path");
    }
    ~Impl() {
        try {
            if (started) function<int (*)(unsigned, void*)>("JLINK_RTTERMINAL_Control")(1, nullptr);
            if (opened) function<void (*)()>("JLINKARM_Close")();
        } catch (...) {
        }
#ifdef _WIN32
        if (library) FreeLibrary(library);
#else
        if (library) dlclose(library);
#endif
    }
    void command(const char* text) {
        char error[256] = {};
        if (function<int (*)(const char*, char*, int)>("JLINKARM_ExecCommand")(text, error, sizeof(error)) < 0)
            throw std::runtime_error(std::string("J-Link: ") + error);
    }
    void start() {
        if (function<int (*)(unsigned, void*)>("JLINK_RTTERMINAL_Control")(0, nullptr) < 0)
            throw std::runtime_error("Cannot start RTT");
        started = true;
        wait_until([&] {
            // NumBytesTransferred, NumBytesRead, HostOverflowCount, IsRunning, NumUpBuffers, NumDownBuffers,
            // reserved[2].
            uint32_t status[8] = {};
            if (function<int (*)(unsigned, void*)>("JLINK_RTTERMINAL_Control")(4, status) < 0)
                throw std::runtime_error("Cannot query RTT status");
            return status[4] != 0;
        });
    }
};
Rtt::Rtt(unsigned selected) : impl(std::make_unique<Impl>()) {
    if (!selected) {
        int count = impl->function<int (*)()>("JLINKARM_EMU_GetNumDevices")();
        if (count < 1) throw std::runtime_error("No USB J-Link probe found");
        if (count > 1) throw std::runtime_error("Multiple J-Link probes found; select one with --probe=<serial>");
    } else if (impl->function<int (*)(unsigned)>("JLINKARM_EMU_SelectByUSBSN")(selected) < 0) {
        throw std::runtime_error("Selected J-Link probe not found");
    }
    auto error = impl->function<const char* (*)()>("JLINKARM_Open")();
    if (error) throw std::runtime_error(std::string("J-Link open: ") + error);
    impl->opened = true;
    probe = impl->function<unsigned (*)()>("JLINKARM_GetSN")();
    if (selected && selected != probe) throw std::runtime_error("J-Link serial mismatch");
    if (impl->function<int (*)(int)>("JLINKARM_TIF_Select")(1) < 0) throw std::runtime_error("Cannot select SWD");
    impl->function<void (*)(unsigned)>("JLINKARM_SetSpeed")(4000);
    impl->command("Device = NRF52840_XXAA");
    if (impl->function<int (*)()>("JLINKARM_Connect")() < 0) throw std::runtime_error("Cannot connect to nRF52840");
    if (impl->function<int (*)()>("JLINKARM_IsHalted")() != 0)
        throw std::runtime_error("Target is halted; resume it before RTT attachment");
    auto read = impl->function<int (*)(unsigned, unsigned, uint32_t*, uint8_t*)>("JLINKARM_ReadMemU32");
    uint32_t part = 0, id[2] = {};
    if (read(0x10000100, 1, &part, nullptr) != 1 || part != 0x52840 || read(0x10000060, 2, id, nullptr) != 2)
        throw std::runtime_error("Cannot identify an nRF52840 target");
    std::ostringstream out;
    out << std::hex << std::setfill('0') << std::setw(8) << id[1] << std::setw(8) << id[0];
    board = out.str();
    std::cout << "J-Link " << probe << ", board " << board << '\n';
    impl->command("SetRTTSearchRanges 0x20000000 0x40000");
    impl->start();
}
Rtt::~Rtt() = default;
std::string Rtt::exchange(const std::string& request) {
    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    size_t sent = 0;
    std::string response;
    auto expected = request.substr(0, request.find(' ')) + " ";
    while (response.empty()) {
        if (sent < request.size()) {
            int count = impl->function<int (*)(unsigned, const void*, unsigned)>("JLINK_RTTERMINAL_Write")(
                0, request.data() + sent, static_cast<unsigned>(request.size() - sent));
            if (count < 0) throw std::runtime_error("RTT write failed");
            sent += count;
        }
        char data[2048];
        int count = impl->function<int (*)(unsigned, void*, unsigned)>("JLINK_RTTERMINAL_Read")(0, data, sizeof(data));
        if (count < 0) throw std::runtime_error("RTT read failed");
        impl->pending.append(data, count);
        size_t newline;
        while ((newline = impl->pending.find('\n')) != std::string::npos) {
            auto line = impl->pending.substr(0, newline + 1);
            impl->pending.erase(0, newline + 1);
            if (line.size() > 1536) throw std::runtime_error("Oversized RTT record");
            if (line.rfind("EV ", 0) == 0) {
                std::cout << line;
                if (fields(line)["type"] == "fault") throw std::runtime_error("Firmware fault: " + line);
            } else if (line.rfind(expected, 0) == 0 && response.empty())
                response = line;
            else
                throw std::runtime_error("Unexpected RTT record: " + line);
        }
        if (impl->pending.size() > 1536) throw std::runtime_error("Unterminated RTT record");
        if (std::chrono::steady_clock::now() >= deadline) throw std::runtime_error("RTT command timed out");
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    // Reattach after reset so the J-Link DLL discards the previous ring offsets.
    auto reboot = request.find(" REBOOT ");
    if (reboot != std::string::npos && response.rfind(expected + "OK", 0) == 0) {
        impl->function<int (*)(unsigned, void*)>("JLINK_RTTERMINAL_Control")(1, nullptr);
        impl->started = false;
        std::this_thread::sleep_for(std::chrono::milliseconds(std::stoul(request.substr(reboot + 8)) + 200));
        impl->pending.clear();
        impl->start();
    }
    return response;
}
}  // namespace hitl
