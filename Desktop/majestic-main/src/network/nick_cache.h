#pragma once
#include <mutex>
#include <unordered_map>
#include <string>
#include <cstdint>
#include <vector>

inline std::mutex g_nick_cache_mutex;
inline std::unordered_map<uint32_t, std::string> g_nick_cache;

inline std::unordered_map<uint32_t, std::string> g_nick_cache_by_altv_id;

struct NickEntry {
    std::string nick;
    uint32_t altv_id;
    uint32_t script_id;
};
inline std::vector<NickEntry> g_nick_entries;

inline std::string _get_altv_nick(uint32_t sid) {
    if (sid == 0) return "";
    std::lock_guard<std::mutex> lk(g_nick_cache_mutex);
    auto it = g_nick_cache.find(sid);
    if (it != g_nick_cache.end()) return it->second;
    auto it2 = g_nick_cache_by_altv_id.find(sid);
    if (it2 != g_nick_cache_by_altv_id.end()) return it2->second;
    return "";
}

inline std::string _get_altv_nick_any(uint32_t pedHandle) {
    if (pedHandle == 0) return "";
    std::lock_guard<std::mutex> lk(g_nick_cache_mutex);
    auto it = g_nick_cache.find(pedHandle);
    if (it != g_nick_cache.end()) return it->second;
    return "";
}

inline std::string _get_altv_nick_by_altv_id(uint32_t altv_id) {
    std::lock_guard<std::mutex> lk(g_nick_cache_mutex);
    auto it = g_nick_cache_by_altv_id.find(altv_id);
    if (it != g_nick_cache_by_altv_id.end()) return it->second;
    return "";
}


// ============================================================================
//  NOCTUA LICENSED BUILD WATERMARK  (auto-generated, do not remove)
//  Inert per-file identifier required by the EULA. It uniquely tags this
//  source file so this program is distinguishable from any other build.
//  Contains no functional logic.
//  file: src/network/nick_cache.h
// ============================================================================
#ifndef NOCTUA_LICENSE_MARK_21362a55b5832d1ab9cb9dcdfd4afbe3
#define NOCTUA_LICENSE_MARK_21362a55b5832d1ab9cb9dcdfd4afbe3
namespace noctua_license { namespace mark_21362a55b5832d1ab9cb9dcdfd4afbe3 {
    inline constexpr unsigned long long kMarkId = 0xf3cd2b5521cf619cull;
    inline constexpr char kMarkFile[] = "src/network/nick_cache.h";
    inline constexpr unsigned char kMarkEntropy[] = { 0x6e, 0xd6, 0x4f, 0xdb, 0xc9, 0xde, 0x28, 0x63, 0x83, 0x9a, 0x73, 0xc2, 0xaf, 0xb0, 0x9f, 0xc3 };
    inline constexpr unsigned long  kMarkSalts[]  = { 0x9883efecul, 0x3ef96ff5ul, 0xf19f7427ul, 0x5819c229ul, 0x0dbe6c2ful, 0x23ead1c3ul, 0x4e2e9110ul, 0xbab69778ul };
    [[maybe_unused]] inline unsigned long long mark_digest() noexcept {
        unsigned long long h = 1469598103934665603ull ^ kMarkId;
        for (unsigned char b : kMarkEntropy) { h ^= b; h *= 1099511628211ull; }
        for (unsigned long  s : kMarkSalts)  { h ^= s; h *= 1099511628211ull; }
        return h;
    }
} } // namespace noctua_license::mark_21362a55b5832d1ab9cb9dcdfd4afbe3
#endif // NOCTUA_LICENSE_MARK_21362a55b5832d1ab9cb9dcdfd4afbe3
