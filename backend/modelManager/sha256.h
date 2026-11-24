#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

// SHA-256 (FIPS 180-4), fed in pieces so a large GGUF file is never held in memory at once.
class Sha256 {
public:
    Sha256();
    void update(const void* data, std::size_t size);
    void update(std::string_view bytes) { update(bytes.data(), bytes.size()); }
    // The digest as 64 lowercase hex digits. The object is spent afterwards.
    std::string finishHex();

    static std::string hex(std::string_view bytes);

private:
    void block(const std::uint8_t* p);

    std::array<std::uint32_t, 8> state_;
    std::array<std::uint8_t, 64> buffer_{};
    std::size_t buffered_ = 0;
    std::uint64_t length_ = 0;   // bytes so far
};
