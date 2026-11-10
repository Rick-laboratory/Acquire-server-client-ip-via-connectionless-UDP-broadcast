#pragma once
#include "../discovery.hpp"

#include <algorithm>

class Inline_IPv4_Broadcast_UDP {
public:
    explicit Inline_IPv4_Broadcast_UDP(std::uint16_t port = 9009, DWORD timeout_ms = 5000,
                                       const std::string& destination = "255.255.255.255",
                                       const std::string& bind_address = "0.0.0.0")
        : destination_(discovery::address(destination, port)), timeout_ms_(timeout_ms) {
        if (port == 0 || timeout_ms == 0) {
            throw std::invalid_argument("Destination port and timeout must be positive.");
        }
        const BOOL enabled = TRUE;
        if (setsockopt(socket_.get(), SOL_SOCKET, SO_BROADCAST,
                       reinterpret_cast<const char*>(&enabled), sizeof(enabled)) == SOCKET_ERROR) {
            throw discovery::socket_error("setsockopt(SO_BROADCAST)");
        }
        socket_.bind_to(bind_address, 0);
    }

    discovery::Peer Broadcast() {
        const auto deadline = discovery::deadline_after(timeout_ms_);
        while (discovery::Clock::now() < deadline) {
            discovery::send_token(socket_.get(), destination_);
            const auto retry_at = std::min(deadline, discovery::Clock::now() +
                                                     std::chrono::milliseconds(500));
            while (discovery::wait_readable(socket_.get(), retry_at)) {
                sockaddr_in peer{};
                if (discovery::receive_token(socket_.get(), peer) &&
                    peer.sin_port == destination_.sin_port &&
                    (destination_.sin_addr.s_addr == INADDR_BROADCAST ||
                     destination_.sin_addr.s_addr == peer.sin_addr.s_addr)) {
                    return discovery::describe(peer);
                }
            }
        }
        throw std::runtime_error("Server discovery timed out.");
    }

private:
    discovery::Socket socket_;
    sockaddr_in destination_{};
    DWORD timeout_ms_;
};
