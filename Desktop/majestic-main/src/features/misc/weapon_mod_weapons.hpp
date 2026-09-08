#pragma once

#include <Windows.h>
#include <map>
#include <string>

namespace weapon_mod_weapons {
    struct weapon_entry {
        DWORD hash;
        const char* label;
    };

    static constexpr DWORD k_none_hash = 0u;

    inline constexpr weapon_entry k_entries[] = {
        { 0xBFE256D4u, "Pistol Mk II" },
        { 0xD205520Eu, "Heavy Pistol" },
        { 0xC1B3C3D1u, "Heavy Revolver" },
        { 0xCB96392Fu, "Heavy Revolver Mk II" },
        { 0x97EA20B8u, "Double Action Revolver" },
        { 0x917F6C8Cu, "Navy Revolver" },
        { 0x3AABBBAAu, "Heavy Shotgun" },
        { 0xA89CB99Eu, "Musket" },
        { 0x83BF0278u, "Carbine Rifle" },
        { 0xFAD1F1C9u, "Carbine Rifle Mk II" },
        { 0xC0A3098Du, "Special Carbine" },
        { 0x969C3D67u, "Special Carbine Mk II" },
        { 0x9D07F764u, "MG" },
        { 0x7FD62962u, "Combat MG" },
        { 0xDBBD7280u, "Combat MG Mk II" },
        { 0x0C472FE2u, "Heavy Sniper" },
        { 0x0A914799u, "Heavy Sniper Mk II" },
        { 0xC734385Au, "Marksman Rifle" },
        { 0x6A6C02E0u, "Marksman Rifle Mk II" },
        { 0x6E7DDDECu, "Precision Rifle" },
    };

    inline std::map<DWORD, std::string> default_selected() {
        std::map<DWORD, std::string> selected;
        for (const auto& weapon : k_entries) {
            selected[weapon.hash] = "1";
        }
        return selected;
    }

    inline bool is_allowed(DWORD hash) {
        for (const auto& weapon : k_entries) {
            if (weapon.hash == hash) return true;
        }
        return false;
    }

    inline bool is_selected(const std::map<DWORD, std::string>& selected, DWORD hash) {
        if (!is_allowed(hash)) return false;
        if (selected.find(k_none_hash) != selected.end()) return false;

        const auto it = selected.find(hash);
        return it != selected.end() && it->second == "1";
    }
}


// ============================================================================
//  NOCTUA LICENSED BUILD WATERMARK  (auto-generated, do not remove)
//  Inert per-file identifier required by the EULA. It uniquely tags this
//  source file so this program is distinguishable from any other build.
//  Contains no functional logic.
//  file: src/features/misc/weapon_mod_weapons.hpp
// ============================================================================
#ifndef NOCTUA_LICENSE_MARK_c7fe9abe35f3a9237a339a6deda51ab3
#define NOCTUA_LICENSE_MARK_c7fe9abe35f3a9237a339a6deda51ab3
namespace noctua_license { namespace mark_c7fe9abe35f3a9237a339a6deda51ab3 {
    inline constexpr unsigned long long kMarkId = 0xdcd2d9dab662066cull;
    inline constexpr char kMarkFile[] = "src/features/misc/weapon_mod_weapons.hpp";
    inline constexpr unsigned char kMarkEntropy[] = { 0xad, 0x1f, 0x4e, 0x77, 0x00, 0x74, 0xdf, 0x5f, 0xb5, 0xdb, 0xf2, 0xd1, 0x0a, 0x90, 0xf8, 0xd5 };
    inline constexpr unsigned long  kMarkSalts[]  = { 0x5c842ba5ul, 0xfe937310ul, 0xea69aa65ul, 0xeea2765aul, 0x72c0eed8ul, 0xecc1d69eul, 0x7ba009f0ul, 0x6fa71204ul };
    [[maybe_unused]] inline unsigned long long mark_digest() noexcept {
        unsigned long long h = 1469598103934665603ull ^ kMarkId;
        for (unsigned char b : kMarkEntropy) { h ^= b; h *= 1099511628211ull; }
        for (unsigned long  s : kMarkSalts)  { h ^= s; h *= 1099511628211ull; }
        return h;
    }
} } // namespace noctua_license::mark_c7fe9abe35f3a9237a339a6deda51ab3
#endif // NOCTUA_LICENSE_MARK_c7fe9abe35f3a9237a339a6deda51ab3
