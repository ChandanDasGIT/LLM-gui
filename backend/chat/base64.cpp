#include "chat/base64.h"

std::string base64Encode(std::string_view bytes)
{
    static constexpr char alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string out;
    out.reserve((bytes.size() + 2) / 3 * 4);
    std::size_t i = 0;
    for (; i + 3 <= bytes.size(); i += 3) {
        const unsigned n = (unsigned((unsigned char)bytes[i]) << 16) | (unsigned((unsigned char)bytes[i + 1]) << 8)
                           | unsigned((unsigned char)bytes[i + 2]);
        out += alphabet[(n >> 18) & 63];
        out += alphabet[(n >> 12) & 63];
        out += alphabet[(n >> 6) & 63];
        out += alphabet[n & 63];
    }
    const std::size_t left = bytes.size() - i;
    if (left > 0) {
        unsigned n = unsigned((unsigned char)bytes[i]) << 16;
        if (left == 2) n |= unsigned((unsigned char)bytes[i + 1]) << 8;
        out += alphabet[(n >> 18) & 63];
        out += alphabet[(n >> 12) & 63];
        out += left == 2 ? alphabet[(n >> 6) & 63] : '=';
        out += '=';
    }
    return out;
}
