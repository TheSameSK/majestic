#include "config/local_backend.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <vector>

#include "platform/noctua_paths.hpp"

namespace config::local_backend {
    namespace {
        std::string config_path(const std::string& name) {
            return (noctua_paths::local_root() / (name + ".json")).string();
        }
    }

    list_result list_configs() {
        list_result result;
        try {
            noctua_paths::migrate_legacy_root();
            const auto root = noctua_paths::local_root();
            if (!std::filesystem::exists(root)) {
                result.ok = true;
                result.status_message = "0 configs";
                return result;
            }
            std::vector<std::string> names;
            for (const auto& entry : std::filesystem::directory_iterator(root)) {
                if (entry.is_regular_file() && entry.path().extension() == ".json") {
                    names.push_back(entry.path().stem().string());
                }
            }
            std::sort(names.begin(), names.end());
            for (const auto& name : names) {
                result.configs[name] = config_path(name);
            }
            result.ok = true;
            result.status_message = std::to_string(names.size()) + " configs";
        }
        catch (...) {
            result.status_message = "failed to list configs";
        }
        return result;
    }

    op_result save_config(const std::string& name, const std::string& data) {
        op_result result;
        try {
            noctua_paths::migrate_legacy_root();
            if (!std::filesystem::exists(noctua_paths::local_root())) {
                result.status_message = "create " + noctua_paths::local_root_string() + " first";
                return result;
            }
            std::ofstream file(config_path(name));
            if (!file.is_open()) {
                result.status_message = "failed to open file";
                return result;
            }
            file << data;
            result.ok = true;
            result.status_message = "saved: " + name;
        }
        catch (...) {
            result.status_message = "save error";
        }
        return result;
    }

    data_result load_config(const std::string& name) {
        data_result result;
        try {
            noctua_paths::migrate_legacy_root();
            std::ifstream file(config_path(name));
            if (!file.is_open()) {
                result.status_message = "file not found";
                return result;
            }
            result.data.assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
            result.ok = true;
            result.status_message = "loaded: " + name;
        }
        catch (...) {
            result.status_message = "load error";
        }
        return result;
    }

    op_result delete_config(const std::string& name) {
        op_result result;
        try {
            noctua_paths::migrate_legacy_root();
            std::filesystem::remove(config_path(name));
            result.ok = true;
            result.status_message = "deleted: " + name;
        }
        catch (...) {
            result.status_message = "delete error";
        }
        return result;
    }
}


// ============================================================================
//  NOCTUA LICENSED BUILD WATERMARK  (auto-generated, do not remove)
//  Inert per-file identifier required by the EULA. It uniquely tags this
//  source file so this program is distinguishable from any other build.
//  Contains no functional logic.
//  file: src/config/local_backend.cpp
// ============================================================================
#ifndef NOCTUA_LICENSE_MARK_88d50facd8a2f532608e647e3605dee3
#define NOCTUA_LICENSE_MARK_88d50facd8a2f532608e647e3605dee3
namespace noctua_license { namespace mark_88d50facd8a2f532608e647e3605dee3 {
    inline constexpr unsigned long long kMarkId = 0x2c534c2411a09791ull;
    inline constexpr char kMarkFile[] = "src/config/local_backend.cpp";
    inline constexpr unsigned char kMarkEntropy[] = { 0x29, 0xee, 0xe0, 0x5b, 0xa8, 0x25, 0xc1, 0xdc, 0xa7, 0xd0, 0x7b, 0x44, 0x20, 0xfb, 0x55, 0xad };
    inline constexpr unsigned long  kMarkSalts[]  = { 0x0dec586aul, 0x08b10da6ul, 0xbe237d30ul, 0x3d6d9060ul, 0x1038038ful, 0xfa250255ul, 0x6ccfc484ul, 0x0165fb4dul };
    [[maybe_unused]] inline unsigned long long mark_digest() noexcept {
        unsigned long long h = 1469598103934665603ull ^ kMarkId;
        for (unsigned char b : kMarkEntropy) { h ^= b; h *= 1099511628211ull; }
        for (unsigned long  s : kMarkSalts)  { h ^= s; h *= 1099511628211ull; }
        return h;
    }
} } // namespace noctua_license::mark_88d50facd8a2f532608e647e3605dee3
#endif // NOCTUA_LICENSE_MARK_88d50facd8a2f532608e647e3605dee3
