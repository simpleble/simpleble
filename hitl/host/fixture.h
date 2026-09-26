#pragma once
#include <simpleble/SimpleBLE.h>
#include <chrono>
#include <functional>
#include <map>
#include <memory>
#include <string>

namespace hitl {
using Bytes = SimpleBLE::ByteArray;
using Fields = std::map<std::string, std::string>;
struct Options {
    std::string control = "ble", board;
    unsigned probe = 0, adapter = 0;
    // Consumes HITL arguments, leaving GoogleTest arguments in argv.
    static Options parse(int& argc, char** argv);
};
std::string uuid(unsigned suffix);
std::string hex(const Bytes& data);
Bytes unhex(const std::string& text);
Fields fields(const std::string& line);
std::string exchange_ble(const std::string& request, const std::function<void(const Bytes&)>& write,
                         const std::function<Bytes()>& read);
void wait_until(const std::function<bool()>& predicate, std::chrono::milliseconds timeout = std::chrono::seconds(5));

class Rtt {
  public:
    explicit Rtt(unsigned probe);
    ~Rtt();
    Rtt(const Rtt&) = delete;
    Rtt& operator=(const Rtt&) = delete;
    std::string board;
    unsigned probe = 0;
    std::string exchange(const std::string& request);

  private:
    struct Impl;
    std::unique_ptr<Impl> impl;
};

// Operations are explicit: constructing this object does not scan, connect, or reset.
class Fixture {
  public:
    explicit Fixture(const Options& options);
    ~Fixture();
    void discover();
    void connect();
    void disconnect();
    Fields control(const std::string& command);
    // Regression setup: reset, reconnect, select the next MTU ceiling, reconnect.
    void reset(unsigned test_id, unsigned ceiling);
    SimpleBLE::Adapter adapter;
    SimpleBLE::Peripheral peer;
    std::string board;
    bool uses_rtt() const { return rtt != nullptr; }

  private:
    std::unique_ptr<Rtt> rtt;
    unsigned request_id = 0;
};
}  // namespace hitl
