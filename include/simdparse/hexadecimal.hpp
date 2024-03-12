#pragma once
#include <string_view>
#include <charconv>

#if defined(__AVX2__)
#include <immintrin.h>
#endif

namespace simdparse
{
    struct hexadecimal_integer
    {
        constexpr static std::string_view name = "hexadecimal integer";

        constexpr hexadecimal_integer()
        {}

        constexpr hexadecimal_integer(uint64_t value)
            : value(value)
        {}

        constexpr bool operator==(const hexadecimal_integer& op) const
        {
            return value == op.value;
        }

        constexpr bool operator!=(const hexadecimal_integer& op) const
        {
            return value != op.value;
        }

        constexpr bool operator<(const hexadecimal_integer& op) const
        {
            return value < op.value;
        }

        constexpr bool operator>(const hexadecimal_integer& op) const
        {
            return value > op.value;
        }

        bool parse(const std::string_view& str)
        {
            if (str.size() > 2) {
                if (str[0] == '0' && str[1] == 'x') {
                    return parse_string(str.substr(2));
                }
            }
            return parse_string(str);
        }

    private:
        bool parse_string(const std::string_view& str)
        {
            if (str.size() > 16) {
                return false;
            }
            return parse_hexadecimal(str);
        }

#if defined(__AVX2__)
        /** Parses the string representation of an integer with SIMD instructions. */
        bool parse_hexadecimal(const std::string_view& str)
        {
            alignas(__m128i) std::array<char, 16> buf = {
                '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0'
            };
            std::memcpy(buf.data() + 16 - str.size(), str.data(), str.size());
            const __m128i characters = _mm_load_si128(reinterpret_cast<const __m128i*>(buf.data()));

            // translate ASCII bytes to their value
            // i.e. 0x3132333435363738 -> 0x0102030405060708
            // i.e. 0x3030616263646566 -> 0x00000a0b0c0d0e0f
            const __m128i sub = _mm_set1_epi8(0x2f);
            __m128i a = _mm_sub_epi8(characters, sub);
            const __m128i mask = _mm_set1_epi8(0x20);
            __m128i alpha = _mm_slli_epi64(_mm_and_si128(a, mask), 2);
            const __m128i alpha_offset = _mm_set1_epi8(0x28);
            const __m128i digits_offset = _mm_set1_epi8(0x01);
            __m128i sub_mask = _mm_blendv_epi8(digits_offset, alpha_offset, alpha);
            a = _mm_sub_epi8(a, sub_mask);

            // group in 32-bit integer slots, and byte-swap to LSB first
            const __m128i unweave = _mm_set_epi32(0x00020406, 0x01030507, 0x080a0c0e, 0x090b0d0f);
            a = _mm_shuffle_epi8(a, unweave);

            // shift 32-bit slots with high digits
            const __m128i shift = _mm_set_epi32(0x00000004, 0x00000000, 0x00000004, 0x00000000);
            a = _mm_sllv_epi32(a, shift);

            // horizontally add consecutive 32-bit integers to re-combine 64-bit number
            a = _mm_hadd_epi32(a, _mm_setzero_si128());

            _mm_storel_epi64(reinterpret_cast<__m128i*>(&value), a);
            return true;
        }
#else
        bool parse_hexadecimal(const std::string_view& str)
        {
            std::from_chars_result result = std::from_chars(str.data(), str.data() + str.size(), value, 16);
            return result.ec == std::errc{} && result.ptr == str.data() + str.size();
        }
#endif

    public:
        uint64_t value = 0;
    };
}
