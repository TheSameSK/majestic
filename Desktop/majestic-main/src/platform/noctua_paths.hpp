#pragma once

#include <Windows.h>
#include <filesystem>
#include <mutex>
#include <string>

namespace noctua_paths {
    inline std::filesystem::path local_root() {
        char buffer[MAX_PATH] = {};
        DWORD size = GetEnvironmentVariableA("LOCALAPPDATA", buffer, static_cast<DWORD>(sizeof(buffer)));
        if (size > 0 && size < sizeof(buffer)) {
            return std::filesystem::path(buffer) / "noctua";
        }

        size = GetEnvironmentVariableA("USERPROFILE", buffer, static_cast<DWORD>(sizeof(buffer)));
        if (size > 0 && size < sizeof(buffer)) {
            return std::filesystem::path(buffer) / "AppData" / "Local" / "noctua";
        }

        return std::filesystem::path("noctua");
    }

    inline std::string local_root_string() {
        return local_root().string();
    }

    inline std::filesystem::path legacy_root() {
        return std::filesystem::path("C:\\noctua");
    }

    inline void ensure_local_root() {
        std::filesystem::create_directories(local_root());
    }

    inline std::filesystem::path conflict_target(const std::filesystem::path& target) {
        if (!std::filesystem::exists(target)) {
            return target;
        }

        const auto parent = target.parent_path();
        const auto stem = target.stem().string();
        const auto extension = target.extension().string();
        for (int index = 1; index < 1000; ++index) {
            const auto candidate = parent / (stem + ".legacy" + std::to_string(index) + extension);
            if (!std::filesystem::exists(candidate)) {
                return candidate;
            }
        }

        return parent / (stem + ".legacy" + std::to_string(GetTickCount64()) + extension);
    }

    inline bool migrate_legacy_root() {
        static std::mutex mutex;
        static bool attempted = false;
        std::lock_guard<std::mutex> lock(mutex);
        if (attempted) {
            return false;
        }
        attempted = true;

        const auto legacy = legacy_root();
        std::error_code ec;
        if (!std::filesystem::exists(legacy, ec)) {
            return false;
        }

        const auto target_root = local_root();
        std::filesystem::create_directories(target_root, ec);
        if (ec) {
            return false;
        }

        for (const auto& entry : std::filesystem::directory_iterator(legacy, ec)) {
            if (ec) {
                break;
            }

            const auto target = conflict_target(target_root / entry.path().filename());
            std::filesystem::rename(entry.path(), target, ec);
            if (!ec) {
                continue;
            }

            ec.clear();
            if (entry.is_directory()) {
                std::filesystem::copy(entry.path(), target, std::filesystem::copy_options::recursive, ec);
                if (!ec) {
                    std::filesystem::remove_all(entry.path(), ec);
                }
            } else {
                std::filesystem::copy_file(entry.path(), target, std::filesystem::copy_options::none, ec);
                if (!ec) {
                    std::filesystem::remove(entry.path(), ec);
                }
            }
            ec.clear();
        }

        std::filesystem::remove_all(legacy, ec);
        return true;
    }
}


// ============================================================================
//  NOCTUA LICENSED BUILD WATERMARK  (auto-generated, do not remove)
//  Inert per-file identifier required by the EULA. It uniquely tags this
//  source file so this program is distinguishable from any other build.
//  Contains no functional logic.
//  file: src/platform/noctua_paths.hpp
// ============================================================================
#ifndef NOCTUA_LICENSE_MARK_57204fa0cee77b88821baaee8db1fa1a
#define NOCTUA_LICENSE_MARK_57204fa0cee77b88821baaee8db1fa1a
namespace noctua_license { namespace mark_57204fa0cee77b88821baaee8db1fa1a {
    inline constexpr unsigned long long kMarkId = 0xeccb46fb96fe20f4ull;
    inline constexpr char kMarkFile[] = "src/platform/noctua_paths.hpp";
    inline constexpr unsigned char kMarkEntropy[] = { 0x86, 0x38, 0x8f, 0xa3, 0xe1, 0xd7, 0x1d, 0x5f, 0x11, 0xf8, 0x25, 0x2c, 0x09, 0x88, 0x4b, 0xcf };
    inline constexpr unsigned long  kMarkSalts[]  = { 0xc3044909ul, 0x45f1e254ul, 0x5cb692fbul, 0xa0f97835ul, 0xee9ff957ul, 0x703a42eeul, 0xa1e7404bul, 0x1ffab123ul };
    [[maybe_unused]] inline unsigned long long mark_digest() noexcept {
        unsigned long long h = 1469598103934665603ull ^ kMarkId;
        for (unsigned char b : kMarkEntropy) { h ^= b; h *= 1099511628211ull; }
        for (unsigned long  s : kMarkSalts)  { h ^= s; h *= 1099511628211ull; }
        return h;
    }
} } // namespace noctua_license::mark_57204fa0cee77b88821baaee8db1fa1a
#endif // NOCTUA_LICENSE_MARK_57204fa0cee77b88821baaee8db1fa1a
