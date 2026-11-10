#include "Inline_IPv4_Broadcast_UDP/Inline_IPv4_Broadcast_UDP.h"
#include "Inline_IPv4_Receiver/Inline_IPv4_Receiver.h"

#include <future>
#include <iostream>

namespace {
void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
} // namespace

int main() {
    try {
        Inline_IPv4_Receiver server(0, 2000, "127.0.0.1");
        discovery::Socket noise;
        noise.bind_to("127.0.0.1", 0);
        const sockaddr_in target = discovery::address("127.0.0.1", server.Port());

        // Reject empty, short, wrong and oversized packets, even a valid-token prefix.
        const std::string valid(discovery::token.data(), discovery::token.size());
        for (const std::string& message :
             {std::string(), std::string("x"), std::string("junk"), valid + "extra"}) {
            require(sendto(noise.get(), message.data(), static_cast<int>(message.size()), 0,
                           reinterpret_cast<const sockaddr*>(&target), sizeof(target)) ==
                    static_cast<int>(message.size()), "Noise send failed.");
        }

        auto response = std::async(std::launch::async, [&server] { return server.Receive(); });
        Inline_IPv4_Broadcast_UDP client(server.Port(), 2000, "127.0.0.1", "127.0.0.1");
        const discovery::Peer discovered = client.Broadcast();
        const discovery::Peer requester = response.get();
        require(discovered.address == "127.0.0.1" && discovered.port == server.Port(),
                "The client received the wrong endpoint.");
        require(requester.address == "127.0.0.1" && requester.port != 0,
                "The server did not return its client's endpoint.");

        Inline_IPv4_Receiver quiet(0, 100, "127.0.0.1");
        bool timed_out = false;
        try { quiet.Receive(); }
        catch (const std::runtime_error&) { timed_out = true; }
        require(timed_out, "The receive deadline did not terminate the operation.");

        discovery::Socket silent;
        const auto silent_port = silent.bind_to("127.0.0.1", 0);
        Inline_IPv4_Broadcast_UDP unanswered(silent_port, 100, "127.0.0.1", "127.0.0.1");
        bool client_timed_out = false;
        try { unanswered.Broadcast(); }
        catch (const std::runtime_error&) { client_timed_out = true; }
        require(client_timed_out, "The client deadline did not terminate discovery.");

        bool rejected = false;
        try { Inline_IPv4_Broadcast_UDP invalid(0); }
        catch (const std::invalid_argument&) { rejected = true; }
        require(rejected, "Zero destination port accepted.");
        std::cout << "Loopback discovery, malformed datagrams, endpoint and timeout checks passed.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
