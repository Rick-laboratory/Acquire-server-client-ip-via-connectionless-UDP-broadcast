#pragma once
#include "../discovery.hpp"

class Inline_IPv4_Receiver {
public:
    explicit Inline_IPv4_Receiver(std::uint16_t port = 9009, DWORD timeout_ms = 5000,
                                 const std::string& bind_address = "0.0.0.0")
        : timeout_ms_(timeout_ms) {
        if (timeout_ms == 0) throw std::invalid_argument("The timeout must be positive.");
        port_ = socket_.bind_to(bind_address, port);
    }

    discovery::Peer Receive() {
        const auto deadline = discovery::deadline_after(timeout_ms_);
        while (discovery::wait_readable(socket_.get(), deadline)) {
            sockaddr_in peer{};
            if (discovery::receive_token(socket_.get(), peer)) {
                discovery::send_token(socket_.get(), peer);
                return discovery::describe(peer);
            }
        }
        throw std::runtime_error("Client discovery timed out.");
    }

    std::uint16_t Port() const noexcept { return port_; }

private:
    discovery::Socket socket_;
    DWORD timeout_ms_;
    std::uint16_t port_ = 0;
};
