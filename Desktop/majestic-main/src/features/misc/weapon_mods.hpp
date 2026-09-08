#pragma once

#include "features/misc/weapon_mod_weapons.hpp"

#include <map>

namespace weapon_mods {
    inline std::map<uintptr_t, float> default_time_to_shoot;

    inline bool valid_weapon_info(CWeaponInfo* info) {
        const uintptr_t ptr = reinterpret_cast<uintptr_t>(info);
        return ptr > 0x10000 && ptr < 0x7FFFFFFFFFFF && IsValidPtr(info);
    }

    inline void restore_time_entry(uintptr_t ptr, float value) {
        __try {
            CWeaponInfo* info = reinterpret_cast<CWeaponInfo*>(ptr);
            if (!valid_weapon_info(info)) return;
            info->setTimeToShoot(value);
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
        }
    }

    inline void restore_double_shoot(CWeaponInfo* info) {
        if (!valid_weapon_info(info)) return;
        const uintptr_t ptr = reinterpret_cast<uintptr_t>(info);
        const auto it = default_time_to_shoot.find(ptr);
        if (it == default_time_to_shoot.end()) return;

        restore_time_entry(ptr, it->second);
        default_time_to_shoot.erase(it);
    }

    inline void restore_all_double_shoot() {
        for (const auto& [ptr, value] : default_time_to_shoot) {
            restore_time_entry(ptr, value);
        }
        default_time_to_shoot.clear();
    }

    inline bool selected(const char* key, DWORD hash) {
        const std::map<DWORD, std::string> selected_weapons =
            config::get("hacks", key, weapon_mod_weapons::default_selected());
        return weapon_mod_weapons::is_selected(selected_weapons, hash);
    }

    inline void apply_double_shoot(CWeaponInfo* info, DWORD hash) {
        if (!valid_weapon_info(info) || !selected("double_shoot_weapons", hash)) {
            restore_double_shoot(info);
            return;
        }

        const uintptr_t ptr = reinterpret_cast<uintptr_t>(info);
        if (default_time_to_shoot.find(ptr) == default_time_to_shoot.end()) {
            default_time_to_shoot[ptr] = info->getTimeToShoot();
        }

        info->setTimeToShoot(0.f);
    }
}


// ============================================================================
//  NOCTUA LICENSED BUILD WATERMARK  (auto-generated, do not remove)
//  Inert per-file identifier required by the EULA. It uniquely tags this
//  source file so this program is distinguishable from any other build.
//  Contains no functional logic.
//  file: src/features/misc/weapon_mods.hpp
// ============================================================================
#ifndef NOCTUA_LICENSE_MARK_7c7ee9b71ad2255481ce05803d626751
#define NOCTUA_LICENSE_MARK_7c7ee9b71ad2255481ce05803d626751
namespace noctua_license { namespace mark_7c7ee9b71ad2255481ce05803d626751 {
    inline constexpr unsigned long long kMarkId = 0x0183957bd7f78e28ull;
    inline constexpr char kMarkFile[] = "src/features/misc/weapon_mods.hpp";
    inline constexpr unsigned char kMarkEntropy[] = { 0x45, 0x98, 0x1e, 0x12, 0x07, 0x0e, 0xf6, 0xaa, 0x48, 0x13, 0x0a, 0x73, 0x51, 0x10, 0x0f, 0x98 };
    inline constexpr unsigned long  kMarkSalts[]  = { 0x5e2eb645ul, 0x072fe115ul, 0x0ec25068ul, 0x96540e6dul, 0xece6114bul, 0x7ad4cce0ul, 0x6f31c241ul, 0x1604f3faul };
    [[maybe_unused]] inline unsigned long long mark_digest() noexcept {
        unsigned long long h = 1469598103934665603ull ^ kMarkId;
        for (unsigned char b : kMarkEntropy) { h ^= b; h *= 1099511628211ull; }
        for (unsigned long  s : kMarkSalts)  { h ^= s; h *= 1099511628211ull; }
        return h;
    }
} } // namespace noctua_license::mark_7c7ee9b71ad2255481ce05803d626751
#endif // NOCTUA_LICENSE_MARK_7c7ee9b71ad2255481ce05803d626751
