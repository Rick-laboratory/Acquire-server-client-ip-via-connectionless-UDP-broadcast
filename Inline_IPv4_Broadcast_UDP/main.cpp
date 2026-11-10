#include "Inline_IPv4_Broadcast_UDP.h"
#include "../cli.hpp"
#include <iostream>

int main(int argc, char* argv[]) {
    if (argc == 2 && (std::string_view(argv[1]) == "--help" ||
                     std::string_view(argv[1]) == "-h")) {
        std::cout << "Usage: discovery-client [server-IPv4] [port]\n"
                     "Defaults: 255.255.255.255:9009. Retries every 500 ms for 5 seconds.\n";
        return 0;
    }
    if (argc > 3) {
        std::cerr << "Usage: discovery-client [server-IPv4] [port]\n";
        return 2;
    }
    try {
        Inline_IPv4_Broadcast_UDP client(argc == 3 ? parse_port(argv[2]) : 9009,
                                        5000, argc >= 2 ? argv[1] : "255.255.255.255");
        const discovery::Peer server = client.Broadcast();
        std::cout << "Server: " << server.address << ':' << server.port << '\n';
        return 0;
    } catch (const std::invalid_argument& error) {
        std::cerr << error.what() << '\n';
        return 2;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}

