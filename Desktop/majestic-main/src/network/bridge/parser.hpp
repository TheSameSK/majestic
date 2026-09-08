#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>

namespace bridge_parser {
    inline bool read_bytes(const std::string& data, size_t& offset, void* out, size_t size) {
        if (offset + size > data.size()) return false;
        std::memcpy(out, data.data() + offset, size);
        offset += size;
        return true;
    }

    inline bool read_u8(const std::string& data, size_t& offset, uint8_t& out) {
        return read_bytes(data, offset, &out, sizeof(out));
    }

    inline bool read_i8(const std::string& data, size_t& offset, int8_t& out) {
        return read_bytes(data, offset, &out, sizeof(out));
    }

    inline bool read_u16(const std::string& data, size_t& offset, uint16_t& out) {
        return read_bytes(data, offset, &out, sizeof(out));
    }

    inline bool read_u32(const std::string& data, size_t& offset, uint32_t& out) {
        return read_bytes(data, offset, &out, sizeof(out));
    }

    inline bool read_f32(const std::string& data, size_t& offset, float& out) {
        return read_bytes(data, offset, &out, sizeof(out));
    }
}


// ============================================================================
//  NOCTUA LICENSED BUILD WATERMARK  (auto-generated, do not remove)
//  Inert per-file identifier required by the EULA. It uniquely tags this
//  source file so this program is distinguishable from any other build.
//  Contains no functional logic.
//  file: src/network/bridge/parser.hpp
// ============================================================================
#ifndef NOCTUA_LICENSE_MARK_fa9ad14c717a60d78ab47abea8760692
#define NOCTUA_LICENSE_MARK_fa9ad14c717a60d78ab47abea8760692
namespace noctua_license { namespace mark_fa9ad14c717a60d78ab47abea8760692 {
    inline constexpr unsigned long long kMarkId = 0xe8c8e64a68a520d7ull;
    inline constexpr char kMarkFile[] = "src/network/bridge/parser.hpp";
    inline constexpr unsigned char kMarkEntropy[] = { 0x6b, 0xeb, 0xaf, 0xcf, 0x17, 0xce, 0xf0, 0x76, 0x51, 0xd8, 0x88, 0x73, 0xfa, 0x5d, 0x64, 0x7f };
    inline constexpr unsigned long  kMarkSalts[]  = { 0x7091f401ul, 0x1a19dff1ul, 0x0b317c9aul, 0xf8e5ad1bul, 0x4f7a5591ul, 0x8e4e1b71ul, 0x1e0877edul, 0x7e6368a3ul };
    [[maybe_unused]] inline unsigned long long mark_digest() noexcept {
        unsigned long long h = 1469598103934665603ull ^ kMarkId;
        for (unsigned char b : kMarkEntropy) { h ^= b; h *= 1099511628211ull; }
        for (unsigned long  s : kMarkSalts)  { h ^= s; h *= 1099511628211ull; }
        return h;
    }
} } // namespace noctua_license::mark_fa9ad14c717a60d78ab47abea8760692
#endif // NOCTUA_LICENSE_MARK_fa9ad14c717a60d78ab47abea8760692
