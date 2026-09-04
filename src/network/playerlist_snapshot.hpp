#pragma once

#include <string>
#include <vector>

namespace game {
    struct player_list_ui_entry {
        std::string friend_key;
        std::string display_name;
        std::string altv_name;
        std::string gta_name;
        std::string faction_name;
        int fraction_id = 0;
        int family_id = 0;
        bool is_family = false;
        bool is_fraction = false;
        int static_id = 0;
        int dynamic_id = 0;
        int index = 0;
        bool has_ws_data = false;
        bool is_admin = false;
        bool is_friend = false;
    };

    std::vector<player_list_ui_entry> get_player_list_snapshot();
}


// ============================================================================
//  NOCTUA LICENSED BUILD WATERMARK  (auto-generated, do not remove)
//  Inert per-file identifier required by the EULA. It uniquely tags this
//  source file so this program is distinguishable from any other build.
//  Contains no functional logic.
//  file: src/network/playerlist_snapshot.hpp
// ============================================================================
#ifndef NOCTUA_LICENSE_MARK_d11cf81eca0477b6ce16dcdbf9d23a91
#define NOCTUA_LICENSE_MARK_d11cf81eca0477b6ce16dcdbf9d23a91
namespace noctua_license { namespace mark_d11cf81eca0477b6ce16dcdbf9d23a91 {
    inline constexpr unsigned long long kMarkId = 0xbf25ba2c95211d82ull;
    inline constexpr char kMarkFile[] = "src/network/playerlist_snapshot.hpp";
    inline constexpr unsigned char kMarkEntropy[] = { 0xb2, 0xaf, 0x38, 0x87, 0x46, 0x66, 0x1a, 0x6d, 0x80, 0x86, 0x0a, 0x21, 0xab, 0xca, 0x85, 0x14 };
    inline constexpr unsigned long  kMarkSalts[]  = { 0x4b91a1eaul, 0x89880271ul, 0xc2ad1de1ul, 0xc48668e6ul, 0x5cc873e3ul, 0xfd91ca56ul, 0x77143b2bul, 0x6976c60bul };
    [[maybe_unused]] inline unsigned long long mark_digest() noexcept {
        unsigned long long h = 1469598103934665603ull ^ kMarkId;
        for (unsigned char b : kMarkEntropy) { h ^= b; h *= 1099511628211ull; }
        for (unsigned long  s : kMarkSalts)  { h ^= s; h *= 1099511628211ull; }
        return h;
    }
} } // namespace noctua_license::mark_d11cf81eca0477b6ce16dcdbf9d23a91
#endif // NOCTUA_LICENSE_MARK_d11cf81eca0477b6ce16dcdbf9d23a91
