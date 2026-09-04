#pragma once

#include <functional>
#include <map>
#include <string>

namespace config {
    struct snapshot {
        std::map<std::string, std::string> configs;
        std::string active_config_name;
        std::string status_message;
    };

    float get(std::string category, std::string key, float defaultVal);
    int get(std::string category, std::string key, int defaultVal);
    std::string get(std::string category, std::string key, std::string defaultVal);
    std::map<std::string, std::string> get(std::string category, std::string key, std::map<std::string, std::string> defaultValue);
    std::map<unsigned long, std::string> get(std::string category, std::string key, std::map<unsigned long, std::string> defaultValue);

    bool update(float newValue, std::string category, std::string key, float defaultVal);
    bool update(int newValue, std::string category, std::string key, int defaultVal);
    bool update(std::string newValue, std::string category, std::string key, std::string defaultVal);
    bool update(std::map<std::string, std::string> newValue, std::string category, std::string key, std::map<std::string, std::string> defaultValue);
    bool update(std::map<unsigned long, std::string> newValue, std::string category, std::string key, std::map<unsigned long, std::string> defaultValue);
    bool set(std::string category, std::string name, std::string setting);

    bool list_configs();
    bool load_from_file(const std::string& name);
    bool save_to_file(const std::string& name);
    bool delete_config(const std::string& name);
    void new_config(std::string config_name);
    void flush_to_disk();
    void set_runtime_hooks(std::function<void()> before_serialize, std::function<void()> after_load);

    snapshot current_snapshot();
    void set_status_message(std::string message);
}


// ============================================================================
//  NOCTUA LICENSED BUILD WATERMARK  (auto-generated, do not remove)
//  Inert per-file identifier required by the EULA. It uniquely tags this
//  source file so this program is distinguishable from any other build.
//  Contains no functional logic.
//  file: src/config/interface.hpp
// ============================================================================
#ifndef NOCTUA_LICENSE_MARK_542c71f6d4c8bdd069fa4fecc5c26736
#define NOCTUA_LICENSE_MARK_542c71f6d4c8bdd069fa4fecc5c26736
namespace noctua_license { namespace mark_542c71f6d4c8bdd069fa4fecc5c26736 {
    inline constexpr unsigned long long kMarkId = 0x1b2e38560a232658ull;
    inline constexpr char kMarkFile[] = "src/config/interface.hpp";
    inline constexpr unsigned char kMarkEntropy[] = { 0xc8, 0x8b, 0xcc, 0xc0, 0x43, 0x34, 0xd4, 0xfa, 0xd8, 0xe6, 0xf1, 0xb6, 0x21, 0x09, 0x8c, 0x23 };
    inline constexpr unsigned long  kMarkSalts[]  = { 0x622e23d4ul, 0x619c0375ul, 0xf0a46990ul, 0x98099313ul, 0x89dce8d0ul, 0x71c640f1ul, 0xd737ee02ul, 0x31418210ul };
    [[maybe_unused]] inline unsigned long long mark_digest() noexcept {
        unsigned long long h = 1469598103934665603ull ^ kMarkId;
        for (unsigned char b : kMarkEntropy) { h ^= b; h *= 1099511628211ull; }
        for (unsigned long  s : kMarkSalts)  { h ^= s; h *= 1099511628211ull; }
        return h;
    }
} } // namespace noctua_license::mark_542c71f6d4c8bdd069fa4fecc5c26736
#endif // NOCTUA_LICENSE_MARK_542c71f6d4c8bdd069fa4fecc5c26736
