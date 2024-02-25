/**
 * simdparse: High-speed parser with vector instructions
 * @see https://github.com/hunyadi/simdparse
 *
 * Copyright (c) 2024 Levente Hunyadi
 *
 * This work is licensed under the terms of the MIT license.
 * For a copy, see <https://opensource.org/licenses/MIT>.
 */

#pragma once
#include <array>
#include <string_view>

#if defined(_WIN32) || defined(_WIN64)
#define WIN32_LEAN_AND_MEAN
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <arpa/inet.h>
#endif

namespace simdparse
{
    struct ipv4_addr
    {
        constexpr static std::string_view name = "IPv4 address";
        using ipv4_t = std::array<uint8_t, sizeof(struct in_addr)>;

        constexpr ipv4_addr()
        {}

        constexpr ipv4_addr(const ipv4_t& addr)
            : _addr(addr)
        {}

        constexpr std::size_t max_size() const
        {
            return _addr.max_size();
        }

        const uint8_t* data() const
        {
            return _addr.data();
        }

        /** Parses an IPv4 address string into an IPv4 address object. */
        bool parse(const std::string_view& str)
        {
            if (str.size() >= INET_ADDRSTRLEN) {
                return false;
            }

            char addr_str[INET_ADDRSTRLEN];
            std::memcpy(addr_str, str.data(), str.size());
            addr_str[str.size()] = 0;

            int status = inet_pton(AF_INET, addr_str, _addr.data());

            if (status <= 0) {
                return false;
            }

            return true;
        }

    private:
        ipv4_t _addr = { 0 };
    };

    struct ipv6_addr
    {
        constexpr static std::string_view name = "IPv6 address";
        using ipv6_t = std::array<uint8_t, sizeof(struct in6_addr)>;

        constexpr ipv6_addr()
        {}

        constexpr ipv6_addr(const ipv6_t& addr)
            : _addr(addr)
        {}

        constexpr std::size_t max_size() const
        {
            return _addr.max_size();
        }

        const uint8_t* data() const
        {
            return _addr.data();
        }

        /** Parses an IPv6 address string into an IPv6 address object. */
        bool parse(const std::string_view& str)
        {
            if (str.size() >= INET6_ADDRSTRLEN) {
                return false;
            }

            char addr_str[INET6_ADDRSTRLEN];
            std::memcpy(addr_str, str.data(), str.size());
            addr_str[str.size()] = 0;

            int status = inet_pton(AF_INET6, addr_str, _addr.data());
            if (status <= 0) {
                return false;
            }

            return true;
        }

    private:
        ipv6_t _addr = { 0 };
    };
}
