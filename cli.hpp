#pragma once
#include <charconv>
#include <cstdint>
#include <stdexcept>
#include <string_view>
#include <system_error>

inline std::uint16_t parse_port(std::string_view text) {
    unsigned port = 0;
    const auto result = std::from_chars(text.data(), text.data() + text.size(), port);
    if (result.ec != std::errc{} || result.ptr != text.data() + text.size() || port > 65535) {
        throw std::invalid_argument("Port must be an integer from 0 to 65535.");
    }
    return static_cast<std::uint16_t>(port);
}
