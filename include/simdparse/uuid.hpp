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
#include <cstdio>
#include <string_view>

#if defined(__AVX2__)
#include <immintrin.h>
#endif

namespace simdparse
{
#if defined(__AVX2__)
    namespace detail
    {
        inline __m128i parse_uuid(__m256i x) {
            // Build a mask to apply a different offset to alphas and digits
            const __m256i sub = _mm256_set1_epi8(0x2f);
            const __m256i mask = _mm256_set1_epi8(0x20);
            const __m256i alpha_offset = _mm256_set1_epi8(0x28);
            const __m256i digits_offset = _mm256_set1_epi8(0x01);
            const __m256i unweave = _mm256_set_epi32(0x0f0d0b09, 0x0e0c0a08, 0x07050301, 0x06040200, 0x0f0d0b09, 0x0e0c0a08, 0x07050301, 0x06040200);
            const __m256i shift = _mm256_set_epi32(0x00000000, 0x00000004, 0x00000000, 0x00000004, 0x00000000, 0x00000004, 0x00000000, 0x00000004);

            // Translate ASCII bytes to their value
            // i.e. 0x3132333435363738 -> 0x0102030405060708
            // Shift hi-digits
            // i.e. 0x0102030405060708 -> 0x1002300450067008
            // Horizontal add
            // i.e. 0x1002300450067008 -> 0x12345678
            __m256i a = _mm256_sub_epi8(x, sub);
            __m256i alpha = _mm256_slli_epi64(_mm256_and_si256(a, mask), 2);
            __m256i sub_mask = _mm256_blendv_epi8(digits_offset, alpha_offset, alpha);
            a = _mm256_sub_epi8(a, sub_mask);
            a = _mm256_shuffle_epi8(a, unweave);
            a = _mm256_sllv_epi32(a, shift);
            a = _mm256_hadd_epi32(a, _mm256_setzero_si256());
            a = _mm256_permute4x64_epi64(a, 0b00001000);

            return _mm256_castsi256_si128(a);
        }
    }
#endif

    struct uuid
    {
        constexpr static std::string_view name = "UUID";

        uuid() = default;
        uuid(std::array<uint8_t, 16> a)
            : _id(a)
        {}

        const uint8_t* data() const
        {
            return _id.data();
        }

        constexpr std::size_t size() const
        {
            return _id.size();
        }

        bool operator==(const uuid& op) const
        {
            return compare(op) == 0;
        }

        bool operator!=(const uuid& op) const
        {
            return compare(op) != 0;
        }

        bool operator<(const uuid& op) const
        {
            return compare(op) < 0;
        }

        bool operator>(const uuid& op) const
        {
            return compare(op) > 0;
        }

        /**
         * Converts an UUIDv4 string representation to a 128-bit unsigned int.
         *
         * UUID string is expected in the 8-4-4-4-12 format, e.g. `f81d4fae-7dec-11d0-a765-00a0c91e6bf6`.
         */
        bool parse(const std::string_view& str)
        {
            if (str.size() == 36) {
                return parse_uuid_rfc_4122(str.data());
            } else if (str.size() == 32) {
                return parse_uuid_compact(str.data());
            }
            return false;
        }

    private:
        int compare(const uuid& op) const
        {
            return std::memcmp(_id.data(), op._id.data(), _id.size());
        }

#if defined(__AVX2__)
        bool parse_uuid_compact(const char* str)
        {
            const __m256i x = _mm256_loadu_si256((__m256i*)str);
            _mm_storeu_si128(reinterpret_cast<__m128i*>(_id.data()), detail::parse_uuid(x));
            return true;
        }

        /**
         * Converts an UUIDv4 string representation to a 128-bit unsigned int.
         *
         * UUID string is expected in the 8-4-4-4-12 format, e.g. `f81d4fae-7dec-11d0-a765-00a0c91e6bf6`.
         * Uses SIMD via Intel AVX2 instruction set.
         *
         * @see https://github.com/crashoz/uuid_v4
         */
        bool parse_uuid_rfc_4122(const char* str)
        {
            // Remove dashes and pack hexadecimal ASCII bytes in a 256-bit integer
            const __m256i dash_shuffle = _mm256_set_epi32(0x80808080, 0x0f0e0d0c, 0x0b0a0908, 0x06050403, 0x80800f0e, 0x0c0b0a09, 0x07060504, 0x03020100);

            __m256i x = _mm256_loadu_si256((__m256i*)str);
            x = _mm256_shuffle_epi8(x, dash_shuffle);
            x = _mm256_insert_epi16(x, *(uint16_t*)(str + 16), 7);
            x = _mm256_insert_epi32(x, *(uint32_t*)(str + 32), 7);

            _mm_storeu_si128(reinterpret_cast<__m128i*>(_id.data()), detail::parse_uuid(x));
            return true;
        }
#else
        bool parse_uuid_compact(const char* str)
        {
            int n = 0;
            sscanf(str,
                "%2hhx%2hhx%2hhx%2hhx"
                "%2hhx%2hhx"
                "%2hhx%2hhx"
                "%2hhx%2hhx"
                "%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx%n",
                &_id[0], &_id[1], &_id[2], &_id[3],
                &_id[4], &_id[5],
                &_id[6], &_id[7],
                &_id[8], &_id[9],
                &_id[10], &_id[11], &_id[12], &_id[13], &_id[14], &_id[15], &n);
            return n == 32;
        }

        bool parse_uuid_rfc_4122(const char* str)
        {
            int n = 0;
            std::sscanf(str,
                "%2hhx%2hhx%2hhx%2hhx-"
                "%2hhx%2hhx-"
                "%2hhx%2hhx-"
                "%2hhx%2hhx-"
                "%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx%n",
                &_id[0], &_id[1], &_id[2], &_id[3],
                &_id[4], &_id[5],
                &_id[6], &_id[7],
                &_id[8], &_id[9],
                &_id[10], &_id[11], &_id[12], &_id[13], &_id[14], &_id[15], &n);
            return n == 36;
        }
#endif

    private:
        std::array<uint8_t, 16> _id;
    };
}
