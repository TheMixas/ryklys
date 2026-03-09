#pragma once
#include <array>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

namespace crypto {

    inline std::array<uint8_t, 32> SHA256(const uint8_t *data, size_t len) {
        // Initial hash values (first 32 bits of fractional parts of square roots of first 8 primes)
        uint32_t h[8] = {
            0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
            0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
        };

        // Round constants (first 32 bits of fractional parts of cube roots of first 64 primes)
        static const uint32_t k[64] = {
            0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,
            0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
            0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
            0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
            0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
            0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
            0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
            0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
            0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
            0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
            0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,
            0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
            0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5,
            0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
            0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
            0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
        };

        auto right_rotate = [](uint32_t val, uint32_t count) -> uint32_t {
            return (val >> count) | (val << (32 - count));
        };

        // Pre-processing: padding
        std::vector<uint8_t> msg(data, data + len);
        uint64_t original_bit_len = len * 8;
        msg.push_back(0x80);
        while (msg.size() % 64 != 56) {
            msg.push_back(0x00);
        }
        for (int i = 56; i >= 0; i -= 8) {
            msg.push_back(static_cast<uint8_t>((original_bit_len >> i) & 0xFF));
        }

        // Process each 512-bit chunk
        for (size_t offset = 0; offset < msg.size(); offset += 64) {
            uint32_t w[64];

            // First 16 words from the chunk
            for (int i = 0; i < 16; i++) {
                w[i] = (msg[offset + i * 4]     << 24) |
                       (msg[offset + i * 4 + 1] << 16) |
                       (msg[offset + i * 4 + 2] << 8)  |
                       (msg[offset + i * 4 + 3]);
            }

            // Extend to 64 words
            for (int i = 16; i < 64; i++) {
                uint32_t s0 = right_rotate(w[i-15], 7) ^ right_rotate(w[i-15], 18) ^ (w[i-15] >> 3);
                uint32_t s1 = right_rotate(w[i-2], 17) ^ right_rotate(w[i-2], 19)  ^ (w[i-2] >> 10);
                w[i] = w[i-16] + s0 + w[i-7] + s1;
            }

            // Initialize working variables
            uint32_t a = h[0], b = h[1], c = h[2], d = h[3];
            uint32_t e = h[4], f = h[5], g = h[6], hh = h[7];

            // 64 rounds
            for (int i = 0; i < 64; i++) {
                uint32_t S1  = right_rotate(e, 6) ^ right_rotate(e, 11) ^ right_rotate(e, 25);
                uint32_t ch  = (e & f) ^ (~e & g);
                uint32_t tmp1 = hh + S1 + ch + k[i] + w[i];
                uint32_t S0  = right_rotate(a, 2) ^ right_rotate(a, 13) ^ right_rotate(a, 22);
                uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
                uint32_t tmp2 = S0 + maj;

                hh = g;
                g  = f;
                f  = e;
                e  = d + tmp1;
                d  = c;
                c  = b;
                b  = a;
                a  = tmp1 + tmp2;
            }

            h[0] += a; h[1] += b; h[2] += c; h[3] += d;
            h[4] += e; h[5] += f; h[6] += g; h[7] += hh;
        }

        // Produce the final hash
        std::array<uint8_t, 32> hash;
        for (int i = 0; i < 8; i++) {
            hash[i * 4]     = (h[i] >> 24) & 0xFF;
            hash[i * 4 + 1] = (h[i] >> 16) & 0xFF;
            hash[i * 4 + 2] = (h[i] >> 8)  & 0xFF;
            hash[i * 4 + 3] =  h[i]        & 0xFF;
        }
        return hash;
    }

    // String overload for convenience
    inline std::array<uint8_t, 32> SHA256(const std::string &input) {
        return SHA256(reinterpret_cast<const uint8_t*>(input.data()), input.size());
    }

    inline std::array<uint8_t, 32> HMAC_SHA256(const std::string &key, const std::string &message) {
        const size_t BLOCK_SIZE = 64;

        // If key is longer than block size, hash it first
        std::vector<uint8_t> k(BLOCK_SIZE, 0);
        if (key.size() > BLOCK_SIZE) {
            auto hashed = SHA256(key);
            std::memcpy(k.data(), hashed.data(), 32);
        } else {
            std::memcpy(k.data(), key.data(), key.size());
        }

        // Inner and outer padded keys
        std::vector<uint8_t> i_key_pad(BLOCK_SIZE);
        std::vector<uint8_t> o_key_pad(BLOCK_SIZE);
        for (size_t i = 0; i < BLOCK_SIZE; i++) {
            i_key_pad[i] = k[i] ^ 0x36;
            o_key_pad[i] = k[i] ^ 0x5C;
        }

        // Inner hash: SHA256(i_key_pad || message)
        std::vector<uint8_t> inner_data(i_key_pad.begin(), i_key_pad.end());
        inner_data.insert(inner_data.end(), message.begin(), message.end());
        auto inner_hash = SHA256(inner_data.data(), inner_data.size());

        // Outer hash: SHA256(o_key_pad || inner_hash)
        std::vector<uint8_t> outer_data(o_key_pad.begin(), o_key_pad.end());
        outer_data.insert(outer_data.end(), inner_hash.begin(), inner_hash.end());
        return SHA256(outer_data.data(), outer_data.size());
    }

}