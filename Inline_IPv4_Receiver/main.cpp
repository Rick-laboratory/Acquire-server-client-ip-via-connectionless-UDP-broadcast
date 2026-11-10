#include "Inline_IPv4_Receiver.h"
#include "../cli.hpp"
#include <iostream>

int main(int argc, char* argv[]) {
    if (argc == 2 && (std::string_view(argv[1]) == "--help" ||
                     std::string_view(argv[1]) == "-h")) {
        std::cout << "Usage: discovery-server [port] [bind-IPv4]\n"
                     "Defaults: 9009, 0.0.0.0. Waits up to 5 seconds for one client.\n";
        return 0;
    }
    if (argc > 3) {
        std::cerr << "Usage: discovery-server [port] [bind-IPv4]\n";
        return 2;
    }
    try {
        Inline_IPv4_Receiver server(argc >= 2 ? parse_port(argv[1]) : 9009,
                                   5000, argc == 3 ? argv[2] : "0.0.0.0");
        std::cout << "Listening on port " << server.Port() << "...\n" << std::flush;
        const discovery::Peer client = server.Receive();
        std::cout << "Client: " << client.address << ':' << client.port << '\n';
        return 0;
    } catch (const std::invalid_argument& error) {
        std::cerr << error.what() << '\n';
        return 2;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
