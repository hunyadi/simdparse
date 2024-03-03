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

        constexpr ipv4_addr(uint8_t a, uint8_t b, uint8_t c, uint8_t d)
            : _addr{ a, b, c, d }
        {}

        bool operator==(const ipv4_addr& op) const
        {
            return _addr == op._addr;
        }

        bool operator!=(const ipv4_addr& op) const
        {
            return _addr != op._addr;
        }

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

        constexpr ipv6_addr(uint64_t a, uint64_t b)
            : _addr{
                static_cast<uint8_t>((a >> 56) & 0xff), static_cast<uint8_t>((a >> 48) & 0xff), static_cast<uint8_t>((a >> 40) & 0xff), static_cast<uint8_t>((a >> 32) & 0xff),
                static_cast<uint8_t>((a >> 24) & 0xff), static_cast<uint8_t>((a >> 16) & 0xff), static_cast<uint8_t>((a >> 8) & 0xff), static_cast<uint8_t>(a & 0xff),
                static_cast<uint8_t>((b >> 56) & 0xff), static_cast<uint8_t>((b >> 48) & 0xff), static_cast<uint8_t>((b >> 40) & 0xff), static_cast<uint8_t>((b >> 32) & 0xff),
                static_cast<uint8_t>((b >> 24) & 0xff), static_cast<uint8_t>((b >> 16) & 0xff), static_cast<uint8_t>((b >> 8) & 0xff), static_cast<uint8_t>(b & 0xff) }
        {}

        constexpr ipv6_addr(uint32_t a, uint32_t b, uint32_t c, uint32_t d)
            : _addr{
                static_cast<uint8_t>((a >> 24) & 0xff), static_cast<uint8_t>((a >> 16) & 0xff), static_cast<uint8_t>((a >> 8) & 0xff), static_cast<uint8_t>(a & 0xff),
                static_cast<uint8_t>((b >> 24) & 0xff), static_cast<uint8_t>((b >> 16) & 0xff), static_cast<uint8_t>((b >> 8) & 0xff), static_cast<uint8_t>(b & 0xff),
                static_cast<uint8_t>((c >> 24) & 0xff), static_cast<uint8_t>((c >> 16) & 0xff), static_cast<uint8_t>((c >> 8) & 0xff), static_cast<uint8_t>(c & 0xff),
                static_cast<uint8_t>((d >> 24) & 0xff), static_cast<uint8_t>((d >> 16) & 0xff), static_cast<uint8_t>((d >> 8) & 0xff), static_cast<uint8_t>(d & 0xff) }
        {}

        constexpr ipv6_addr(uint16_t a, uint16_t b, uint16_t c, uint16_t d, uint16_t e, uint16_t f, uint16_t g, uint16_t h)
            : _addr{
                static_cast<uint8_t>((a >> 8) & 0xff), static_cast<uint8_t>(a & 0xff),
                static_cast<uint8_t>((b >> 8) & 0xff), static_cast<uint8_t>(b & 0xff),
                static_cast<uint8_t>((c >> 8) & 0xff), static_cast<uint8_t>(c & 0xff),
                static_cast<uint8_t>((d >> 8) & 0xff), static_cast<uint8_t>(d & 0xff),
                static_cast<uint8_t>((e >> 8) & 0xff), static_cast<uint8_t>(e & 0xff),
                static_cast<uint8_t>((f >> 8) & 0xff), static_cast<uint8_t>(f & 0xff),
                static_cast<uint8_t>((g >> 8) & 0xff), static_cast<uint8_t>(g & 0xff),
                static_cast<uint8_t>((h >> 8) & 0xff), static_cast<uint8_t>(h & 0xff) }
        {}

        bool operator==(const ipv6_addr& op) const
        {
            return _addr == op._addr;
        }

        bool operator!=(const ipv6_addr& op) const
        {
            return _addr != op._addr;
        }

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
