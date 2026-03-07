#ifndef RYKLYS_BACKEND_WEBSOCKETUTILS_H
#define RYKLYS_BACKEND_WEBSOCKETUTILS_H

#include <array>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

// =============================================================================
// Minimal SHA-1 implementation (RFC 3174)
// =============================================================================
namespace ws_crypto {
    inline std::array<uint8_t, 20> SHA1(const std::string &input) {
        // SHA-1 constants
        uint32_t h0 = 0x67452301;
        uint32_t h1 = 0xEFCDAB89;
        uint32_t h2 = 0x98BADCFE;
        uint32_t h3 = 0x10325476;
        uint32_t h4 = 0xC3D2E1F0;

        // Pre-processing: adding padding bits
        std::vector<uint8_t> msg(input.begin(), input.end());
        uint64_t original_bit_len = msg.size() * 8;
        msg.push_back(0x80);
        while (msg.size() % 64 != 56) {
            msg.push_back(0x00);
        }
        // Append original length in bits as 64-bit big-endian
        for (int i = 56; i >= 0; i -= 8) {
            msg.push_back(static_cast<uint8_t>((original_bit_len >> i) & 0xFF));
        }

        auto left_rotate = [](uint32_t value, uint32_t count) -> uint32_t {
            return (value << count) | (value >> (32 - count));
        };

        // Process each 512-bit chunk
        for (size_t offset = 0; offset < msg.size(); offset += 64) {
            uint32_t w[80];
            for (int i = 0; i < 16; i++) {
                w[i] = (msg[offset + i * 4] << 24) |
                       (msg[offset + i * 4 + 1] << 16) |
                       (msg[offset + i * 4 + 2] << 8) |
                       (msg[offset + i * 4 + 3]);
            }
            for (int i = 16; i < 80; i++) {
                w[i] = left_rotate(w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16], 1);
            }

            uint32_t a = h0, b = h1, c = h2, d = h3, e = h4;
            for (int i = 0; i < 80; i++) {
                uint32_t f, k;
                if (i < 20) {
                    f = (b & c) | ((~b) & d);
                    k = 0x5A827999;
                } else if (i < 40) {
                    f = b ^ c ^ d;
                    k = 0x6ED9EBA1;
                } else if (i < 60) {
                    f = (b & c) | (b & d) | (c & d);
                    k = 0x8F1BBCDC;
                } else {
                    f = b ^ c ^ d;
                    k = 0xCA62C1D6;
                }

                uint32_t temp = left_rotate(a, 5) + f + e + k + w[i];
                e = d;
                d = c;
                c = left_rotate(b, 30);
                b = a;
                a = temp;
            }
            h0 += a;
            h1 += b;
            h2 += c;
            h3 += d;
            h4 += e;
        }

        std::array<uint8_t, 20> hash;
        for (int i = 0; i < 4; i++) {
            hash[i] = (h0 >> (24 - i * 8)) & 0xFF;
            hash[i + 4] = (h1 >> (24 - i * 8)) & 0xFF;
            hash[i + 8] = (h2 >> (24 - i * 8)) & 0xFF;
            hash[i + 12] = (h3 >> (24 - i * 8)) & 0xFF;
            hash[i + 16] = (h4 >> (24 - i * 8)) & 0xFF;
        }
        return hash;
    }

    // =============================================================================
    // Base64 encode
    // =============================================================================
    inline std::string Base64Encode(const uint8_t *data, size_t len) {
        static const char table[] =
                "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
        std::string result;
        result.reserve(4 * ((len + 2) / 3));

        for (size_t i = 0; i < len; i += 3) {
            uint32_t n = (static_cast<uint32_t>(data[i]) << 16);
            if (i + 1 < len) n |= (static_cast<uint32_t>(data[i + 1]) << 8);
            if (i + 2 < len) n |= static_cast<uint32_t>(data[i + 2]);

            result.push_back(table[(n >> 18) & 0x3F]);
            result.push_back(table[(n >> 12) & 0x3F]);
            result.push_back((i + 1 < len) ? table[(n >> 6) & 0x3F] : '=');
            result.push_back((i + 2 < len) ? table[n & 0x3F] : '=');
        }
        return result;
    }

    // =============================================================================
    // Compute Sec-WebSocket-Accept from Sec-WebSocket-Key (RFC 6455 §4.2.2)
    // =============================================================================
    inline std::string ComputeAcceptKey(const std::string &client_key) {
        const std::string MAGIC_GUID = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
        std::string concatenated = client_key + MAGIC_GUID;
        auto hash = SHA1(concatenated);
        return Base64Encode(hash.data(), hash.size());
    }
} // namespace ws_crypto

#endif // RYKLYS_BACKEND_WEBSOCKETUTILS_H
