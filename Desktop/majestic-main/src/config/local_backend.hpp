#pragma once

#include <map>
#include <string>

namespace config::local_backend {
    struct list_result {
        bool ok = false;
        std::map<std::string, std::string> configs;
        std::string status_message;
    };

    struct data_result {
        bool ok = false;
        std::string data;
        std::string status_message;
    };

    struct op_result {
        bool ok = false;
        std::string status_message;
    };

    list_result list_configs();
    op_result save_config(const std::string& name, const std::string& data);
    data_result load_config(const std::string& name);
    op_result delete_config(const std::string& name);
}


// ============================================================================
//  NOCTUA LICENSED BUILD WATERMARK  (auto-generated, do not remove)
//  Inert per-file identifier required by the EULA. It uniquely tags this
//  source file so this program is distinguishable from any other build.
//  Contains no functional logic.
//  file: src/config/local_backend.hpp
// ============================================================================
#ifndef NOCTUA_LICENSE_MARK_45dee271afa51314e6577cd8cc624431
#define NOCTUA_LICENSE_MARK_45dee271afa51314e6577cd8cc624431
namespace noctua_license { namespace mark_45dee271afa51314e6577cd8cc624431 {
    inline constexpr unsigned long long kMarkId = 0x3cc1d4859a7c7146ull;
    inline constexpr char kMarkFile[] = "src/config/local_backend.hpp";
    inline constexpr unsigned char kMarkEntropy[] = { 0xf4, 0x5c, 0x81, 0x4b, 0xf0, 0x2a, 0x34, 0xeb, 0xec, 0xee, 0x35, 0x99, 0xc0, 0x46, 0x5f, 0xfa };
    inline constexpr unsigned long  kMarkSalts[]  = { 0x0e64802bul, 0x7f323a43ul, 0x5b5d22d0ul, 0x8c9e18cdul, 0x4c186633ul, 0xc041e481ul, 0xa6c5b6f1ul, 0x24e5489dul };
    [[maybe_unused]] inline unsigned long long mark_digest() noexcept {
        unsigned long long h = 1469598103934665603ull ^ kMarkId;
        for (unsigned char b : kMarkEntropy) { h ^= b; h *= 1099511628211ull; }
        for (unsigned long  s : kMarkSalts)  { h ^= s; h *= 1099511628211ull; }
        return h;
    }
} } // namespace noctua_license::mark_45dee271afa51314e6577cd8cc624431
#endif // NOCTUA_LICENSE_MARK_45dee271afa51314e6577cd8cc624431
