#ifndef RYKLYS_BACKEND_WEBSOCKETFRAME_H
#define RYKLYS_BACKEND_WEBSOCKETFRAME_H

#include <cstdint>
#include <cstring>
#include <optional>
#include <vector>

// WebSocket opcodes (RFC 6455 §5.2)
enum class WsOpcode : uint8_t {
    CONTINUATION = 0x0,
    TEXT         = 0x1,
    BINARY       = 0x2,
    // 0x3-0x7 reserved for non-control frames
    CLOSE        = 0x8,
    PING         = 0x9,
    PONG         = 0xA,
    // 0xB-0xF reserved for control frames
};

struct WebSocketFrame {
    bool     fin = true;
    WsOpcode opcode = WsOpcode::TEXT;
    bool     masked = false;
    uint8_t  mask_key[4] = {0};
    std::vector<uint8_t> payload;

    // =========================================================================
    // Parse a single frame from a byte buffer.
    // Returns std::nullopt if the buffer doesn't contain a complete frame yet.
    // On success, returns the frame AND advances `consumed` to the number of
    // bytes consumed so you can remove them from your read buffer.
    // =========================================================================
    static std::optional<WebSocketFrame> Parse(const uint8_t* data, size_t len, size_t& consumed) {
        consumed = 0;
        if (len < 2) return std::nullopt;

        WebSocketFrame frame;
        size_t pos = 0;

        // Byte 0: FIN + opcode
        frame.fin    = (data[0] & 0x80) != 0;
        frame.opcode = static_cast<WsOpcode>(data[0] & 0x0F);

        // Byte 1: MASK + payload length
        frame.masked = (data[1] & 0x80) != 0;
        uint64_t payload_len = data[1] & 0x7F;
        pos = 2;

        if (payload_len == 126) {
            if (len < pos + 2) return std::nullopt;
            payload_len = (static_cast<uint64_t>(data[pos]) << 8) |
                           static_cast<uint64_t>(data[pos + 1]);
            pos += 2;
        } else if (payload_len == 127) {
            if (len < pos + 8) return std::nullopt;
            payload_len = 0;
            for (int i = 0; i < 8; i++) {
                payload_len = (payload_len << 8) | static_cast<uint64_t>(data[pos + i]);
            }
            pos += 8;
        }

        // Read masking key if present
        if (frame.masked) {
            if (len < pos + 4) return std::nullopt;
            std::memcpy(frame.mask_key, data + pos, 4);
            pos += 4;
        }

        // Check we have enough data for the payload
        if (len < pos + payload_len) return std::nullopt;

        // Extract and unmask payload
        frame.payload.resize(payload_len);
        std::memcpy(frame.payload.data(), data + pos, payload_len);

        if (frame.masked) {
            for (uint64_t i = 0; i < payload_len; i++) {
                frame.payload[i] ^= frame.mask_key[i % 4];
            }
        }

        consumed = pos + payload_len;
        return frame;
    }

    // =========================================================================
    // Serialize a frame for sending (server→client, so NOT masked)
    // =========================================================================
    std::vector<uint8_t> Serialize() const {
        std::vector<uint8_t> out;
        out.reserve(2 + 8 + payload.size()); // worst case header + payload

        // Byte 0: FIN + opcode
        out.push_back((fin ? 0x80 : 0x00) | static_cast<uint8_t>(opcode));

        // Byte 1: payload length (server frames are NOT masked)
        if (payload.size() <= 125) {
            out.push_back(static_cast<uint8_t>(payload.size()));
        } else if (payload.size() <= 65535) {
            out.push_back(126);
            out.push_back(static_cast<uint8_t>((payload.size() >> 8) & 0xFF));
            out.push_back(static_cast<uint8_t>(payload.size() & 0xFF));
        } else {
            out.push_back(127);
            for (int i = 56; i >= 0; i -= 8) {
                out.push_back(static_cast<uint8_t>((payload.size() >> i) & 0xFF));
            }
        }

        // Payload
        out.insert(out.end(), payload.begin(), payload.end());
        return out;
    }
};

#endif // RYKLYS_BACKEND_WEBSOCKETFRAME_H