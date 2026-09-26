#include <iostream>
#include "fixture.h"

int main(int argc, char** argv) {
    try {
        auto options = hitl::Options::parse(argc, argv);
        if (argc != 1)
            throw std::runtime_error("Options: --control=ble|rtt --board=<id> --probe=<serial> --adapter=<index>");
        hitl::Fixture fixture(options);
        // Edit this sequence for the backend operation under development.
        // With RTT, control() also works before discovery or connection.
        fixture.discover();
        fixture.connect();
        fixture.control("SET READ_VALUE 0001ff");
        auto value = fixture.peer.read(hitl::uuid(1), hitl::uuid(3));
        std::cout << "Read: " << hitl::hex(value) << '\n';
        fixture.peer.write_request(hitl::uuid(1), hitl::uuid(4), hitl::Bytes{0, 0xff, 42});
        fixture.control("GET_WRITE WRITE_REQUEST");
        fixture.disconnect();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
