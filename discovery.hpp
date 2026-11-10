#pragma once
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <winsock2.h>
#include <ws2tcpip.h>

#include <array>
#include <chrono>
#include <cstdint>
#include <stdexcept>
#include <string>

#ifdef _MSC_VER
#pragma comment(lib, "Ws2_32.lib")
#endif

namespace discovery {
using Clock = std::chrono::steady_clock;
// Preserve the four bytes sent by the original Windows int value 0x000B00B5.
inline constexpr std::array<char, 4> token{static_cast<char>(0xB5), 0x00, 0x0B, 0x00};

struct Peer {
    std::string address;
    std::uint16_t port;
};

inline std::runtime_error socket_error(const char* operation) {
    return std::runtime_error(std::string(operation) + " failed: " +
                              std::to_string(WSAGetLastError()));
}

class Winsock {
public:
    Winsock() {
        WSADATA data{};
        const int error = WSAStartup(MAKEWORD(2, 2), &data);
        if (error != 0) throw std::runtime_error("WSAStartup failed: " + std::to_string(error));
    }
    ~Winsock() { WSACleanup(); }
    Winsock(const Winsock&) = delete;
    Winsock& operator=(const Winsock&) = delete;
};

inline sockaddr_in address(const std::string& ip, std::uint16_t port) {
    sockaddr_in result{};
    result.sin_family = AF_INET;
    result.sin_port = htons(port);
    if (inet_pton(AF_INET, ip.c_str(), &result.sin_addr) != 1) {
        throw std::invalid_argument("Expected a numeric IPv4 address.");
    }
    return result;
}

class Socket {
public:
    Socket() : handle_(socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) {
        if (handle_ == INVALID_SOCKET) throw socket_error("socket");
    }
    ~Socket() { closesocket(handle_); }
    Socket(const Socket&) = delete;
    Socket& operator=(const Socket&) = delete;

    SOCKET get() const noexcept { return handle_; }

    std::uint16_t bind_to(const std::string& ip, std::uint16_t port) {
        sockaddr_in local = address(ip, port);
        if (bind(handle_, reinterpret_cast<const sockaddr*>(&local), sizeof(local)) == SOCKET_ERROR) {
            throw socket_error("bind");
        }
        int length = sizeof(local);
        if (getsockname(handle_, reinterpret_cast<sockaddr*>(&local), &length) == SOCKET_ERROR) {
            throw socket_error("getsockname");
        }
        return ntohs(local.sin_port);
    }

private:
    Winsock winsock_; // Constructed before the socket, destroyed after it is closed.
    SOCKET handle_;
};

inline bool wait_readable(SOCKET socket, Clock::time_point deadline) {
    const auto remaining = std::chrono::duration_cast<std::chrono::microseconds>(deadline - Clock::now());
    if (remaining.count() <= 0) return false;
    timeval timeout{};
    timeout.tv_sec = static_cast<long>(remaining.count() / 1000000);
    timeout.tv_usec = static_cast<long>(remaining.count() % 1000000);
    fd_set readable;
    FD_ZERO(&readable);
    FD_SET(socket, &readable);
    const int result = select(0, &readable, nullptr, nullptr, &timeout);
    if (result == SOCKET_ERROR) throw socket_error("select");
    return result > 0;
}

inline void send_token(SOCKET socket, const sockaddr_in& peer) {
    const int sent = sendto(socket, token.data(), static_cast<int>(token.size()), 0,
                            reinterpret_cast<const sockaddr*>(&peer), sizeof(peer));
    if (sent == SOCKET_ERROR) throw socket_error("sendto");
    if (sent != static_cast<int>(token.size())) throw std::runtime_error("Incomplete datagram.");
}

inline bool receive_token(SOCKET socket, sockaddr_in& peer) {
    std::array<char, token.size()> bytes{};
    int peer_size = sizeof(peer);
    const int received = recvfrom(socket, bytes.data(), static_cast<int>(bytes.size()), 0,
                                  reinterpret_cast<sockaddr*>(&peer), &peer_size);
    if (received == SOCKET_ERROR) {
        // Windows reports an oversized datagram after discarding its excess bytes.
        if (WSAGetLastError() == WSAEMSGSIZE) return false;
        throw socket_error("recvfrom");
    }
    return received == static_cast<int>(token.size()) && bytes == token &&
           peer_size == sizeof(sockaddr_in) && peer.sin_family == AF_INET;
}

inline Peer describe(const sockaddr_in& peer) {
    std::array<char, INET_ADDRSTRLEN> text{};
    if (inet_ntop(AF_INET, &peer.sin_addr, text.data(), text.size()) == nullptr) {
        throw socket_error("inet_ntop");
    }
    return {text.data(), ntohs(peer.sin_port)};
}

inline Clock::time_point deadline_after(DWORD timeout_ms) {
    if (timeout_ms == 0) throw std::invalid_argument("The discovery timeout must be positive.");
    return Clock::now() + std::chrono::milliseconds(timeout_ms);
}
} // namespace discovery
