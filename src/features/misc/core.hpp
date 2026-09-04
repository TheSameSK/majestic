#pragma once
#include "features/misc/weapon_mods.hpp"
#include "ui/menu/menu_actions.hpp"

namespace hacks {
    inline bool _SafeCheckPlayerAlive() {
        return IsValidPtr(local.player) && local.player->HP > 0;
    }

    void kill_aura() {
        for (pair<CObject*, DataPed>& entity : game::ped_list) {
            CObject* ped = entity.first;
            DataPed data = entity.second;
            if (IsValidPtr(ped)) {
                CModelInfo* _hmi = ped->ModelInfo();
                if (!IsValidPtr(_hmi)) continue;
                DWORD hash = _hmi->GetHash();
                if ((hash != game::freemode_f) && (hash != game::freemode_m)) {
                    ped->HP = 0.f;
                }
            }
        }
        return;
    }
    map<uintptr_t, float> default_recoil;
    map<uintptr_t, float> default_spread;
    static bool was_recoil_set = false;
    static bool was_spread_set = false;
    static bool was_godmode_set = false;
    inline void _update_hacks_inner();

    inline bool valid_weapon_info_ptr(uintptr_t ptr) {
        return ptr > 0x10000 && ptr < 0x7FFFFFFFFFFF && IsValidPtr(reinterpret_cast<CWeaponInfo*>(ptr));
    }

    inline void restore_recoil_entry(uintptr_t ptr, float value) {
        __try {
            if (!valid_weapon_info_ptr(ptr)) return;
            reinterpret_cast<CWeaponInfo*>(ptr)->setRecoil(value);
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
        }
    }

    inline void restore_spread_entry(uintptr_t ptr, float value) {
        __try {
            if (!valid_weapon_info_ptr(ptr)) return;
            reinterpret_cast<CWeaponInfo*>(ptr)->setSpread(value);
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
        }
    }

    inline void restore_all_recoil() {
        for (const auto& [ptr, value] : default_recoil) {
            restore_recoil_entry(ptr, value);
        }

        default_recoil.clear();
        was_recoil_set = false;
    }

    inline void restore_all_spread() {
        for (const auto& [ptr, value] : default_spread) {
            restore_spread_entry(ptr, value);
        }

        default_spread.clear();
        was_spread_set = false;
    }

    inline void disable_core_runtime_effects() {
        weapon_mods::restore_all_double_shoot();
        restore_all_recoil();
        restore_all_spread();

        if (IsValidPtr(local.player)) {
            memory::write<uint8_t>((uintptr_t)local.player + 0x0189, 0x0);
            local.player->setCollision(0.25f);
        }
        was_godmode_set = false;
    }

    void update_hacks() {
        _update_hacks_inner();
    }
    inline void _update_hacks_inner() {
                    if (IsValidPtr(local.player) && _SafeCheckPlayerAlive()) {
                const bool unsafe_mode = unsafe_mode_enabled();
                bool toggle_key = config::get("visual", "toggle_key", 0);
                if (toggle_key) {
                    runtime_debug::last_section = "gt_hacks_visual_toggle";
                    int toggle_key_num = config::get("visual", "toggle_key_num", 1);
                    const bool toggle_down = bind_state::is_key_down(toggle_key_num);
                    static bool old_state = false;
                    if (toggle_down && !old_state) {
                        bool visuals_enable = config::get("visual", "enable", 0);
                        config::update(!visuals_enable, "visual", "enable", 0);
                    }
                    old_state = toggle_down;
                }
                {
                    bool do_recoil = unsafe_mode && config::get("hacks", "remove_recoil", 0);
                    bool do_spread = unsafe_mode && config::get("hacks", "remove_spread", 0);
                    bool do_range = unsafe_mode && config::get("hacks", "extend_range", 0);
                    bool do_double_shoot = unsafe_mode && bind_state::is_active(k_double_shoot_bind);

                    if (!do_double_shoot) {
                        runtime_debug::last_section = "gt_hacks_restore_double_shoot";
                        weapon_mods::restore_all_double_shoot();
                    }

                    runtime_debug::last_section = "gt_hacks_weapon_info";
                    CWeaponManager* wep = local.player->weapon();
                    if (IsValidPtr(wep) && IsValidPtr(wep->_WeaponInfo)) {
                        auto aWep = wep->_WeaponInfo;
                        if (IsValidPtr(aWep)) {
                            uintptr_t info_ptr = reinterpret_cast<uintptr_t>(aWep);
                            DWORD whash = (info_ptr > 0x10000 && info_ptr < 0x7FFFFFFFFFFF) ? *(DWORD*)(info_ptr + 0x10) : 0;
                            const bool double_shoot_selected = do_double_shoot && weapon_mods::selected("double_shoot_weapons", whash);

                            if (!double_shoot_selected) {
                                runtime_debug::last_section = "gt_hacks_restore_weapon_double_shoot";
                                weapon_mods::restore_double_shoot(aWep);
                            }

                            if (do_range) {
                                runtime_debug::last_section = "gt_hacks_weapon_range";
                                aWep->Range(config::get("hacks", "extended_range", 0.f));
                            }

                            if (double_shoot_selected) {
                                runtime_debug::last_section = "gt_hacks_weapon_double_shoot";
                                weapon_mods::apply_double_shoot(aWep, whash);
                            }

                            if (do_recoil) {
                                runtime_debug::last_section = "gt_hacks_recoil";
                                if (info_ptr && default_recoil.find(info_ptr) == default_recoil.end()) {
                                    default_recoil[info_ptr] = aWep->getRecoil();
                                }
                                aWep->setRecoil(0.f);
                                was_recoil_set = true;
                            } else if (was_recoil_set) {
                                runtime_debug::last_section = "gt_hacks_restore_recoil";
                                restore_all_recoil();
                            }

                            if (do_spread) {
                                runtime_debug::last_section = "gt_hacks_spread";
                                if (info_ptr && default_spread.find(info_ptr) == default_spread.end()) {
                                    default_spread[info_ptr] = aWep->getSpread();
                                }
                                aWep->setSpread(0.f);
                                was_spread_set = true;
                            } else if (was_spread_set) {
                                runtime_debug::last_section = "gt_hacks_restore_spread";
                                restore_all_spread();
                            }
                        }
                    } else {
                    }
                    if (!do_recoil && was_recoil_set && !(IsValidPtr(wep) && IsValidPtr(wep->_WeaponInfo))) {
                        runtime_debug::last_section = "gt_hacks_restore_recoil_missing_weapon";
                        restore_all_recoil();
                    }
                    if (!do_spread && was_spread_set && !(IsValidPtr(wep) && IsValidPtr(wep->_WeaponInfo))) {
                        runtime_debug::last_section = "gt_hacks_restore_spread_missing_weapon";
                        restore_all_spread();
                    }
                }
                if (unsafe_mode && bind_state::is_active(k_godmode_bind)) {
                    runtime_debug::last_section = "gt_hacks_godmode";
                    memory::write<uint8_t>((uintptr_t)local.player + 0x0189, 0x1);
                    was_godmode_set = true;
                }
                else {
                    if (was_godmode_set) {
                        runtime_debug::last_section = "gt_hacks_disable_godmode";
                        was_godmode_set = false;
                        memory::write<uint8_t>((uintptr_t)local.player + 0x0189, 0x0);
                    }
                }
                if (unsafe_mode && config::get("hacks", "auto_armor", 0)) {
                    runtime_debug::last_section = "gt_hacks_auto_armor";
                    int amode = config::get("hacks", "autoarmor_mode", 0);
                    if (amode == 0) {
                        local.player->SetArmor(config::get("hacks", "auto_armor_val", 99.0f));
                    }
                    else if (amode == 1) {
                        float cur_a = config::get("hacks", "auto_armor_val", 99.0f);
                        if (local.player->GetArmor() < cur_a) {
                            local.player->SetArmor(cur_a);
                        }
                    }
                    else if (amode == 2) {
                        int key = config::get("hacks", "armor_key", 1);
                        if (bind_state::is_key_down(key)) {
                            local.player->SetArmor(config::get("hacks", "auto_armor_val", 99.0f));
                        }
                    }
                }
                if (!local.player->IsInVehicle()) {
                    static bool collision_removed = false;
                    const bool remove_collision = unsafe_mode && config::get("hacks", "no_collision_ped", 0);
                    if (remove_collision) {
                        runtime_debug::last_section = "gt_hacks_ped_collision";
                        local.player->setCollision(-1.0f);
                        collision_removed = true;
                    }
                    else if (collision_removed) {
                        runtime_debug::last_section = "gt_hacks_restore_ped_collision";
                        collision_removed = false;
                        local.player->setCollision(0.25f);
                        collision_removed = false;
                    }
                }
                if (config::get("hack", "debug", 0)) {
                    runtime_debug::last_section = "gt_hacks_debug";
                    CVehicle* veh = local.player->vehicle();
                    if (IsValidPtr(veh)) {
                        ui::DEBUG.AddLog("veh -> 0x%x\n", veh);
                    }
                    ui::DEBUG.AddLog("isInVeh -> 0x%x\n", local.player->IsInVehicle());
                }
                if (unsafe_mode && config::get("hacks", "infinite_stamina", 0)) {
                    runtime_debug::last_section = "gt_hacks_stamina";
                    native::player::restore_player_stamina(native::player::player_id(), 1.0f);
                }
                if (menu_actions::pending_set_hp.exchange(false)) {
                    runtime_debug::last_section = "gt_hacks_set_hp";
                    int hp = config::get("hacks", "sethp_value", 100);
                    local.player->HP = (float)(hp + 100);
                }
                if (menu_actions::pending_set_armor.exchange(false)) {
                    runtime_debug::last_section = "gt_hacks_set_armor";
                    float armor = (float)config::get("hacks", "setarmor_value", 100);
                    local.player->SetArmor(armor);
                }
            }
    }
    Vector3 get_cam_directions() {
        const auto ped = local_ped_handle();
        const float entity_heading = ped ? native::entity::get_entity_heading(ped) : 0.0f;
        float heading = native::cam::get_gameplay_cam_relative_heading() + entity_heading;
        float pitch = native::cam::get_gameplay_cam_relative_pitch();
        float x = -sin(heading * PI / 180.0f);
        float y = cos(heading * PI / 180.0f);
        float z = sin(pitch * PI / 180.0f);
        double len = sqrt(x * x + y * y + z * z);
        if (len != 0) {
            x = x / len;
            y = y / len;
            z = z / len;
        }
        return Vector3(x, y, z);
    }
}


// ============================================================================
//  NOCTUA LICENSED BUILD WATERMARK  (auto-generated, do not remove)
//  Inert per-file identifier required by the EULA. It uniquely tags this
//  source file so this program is distinguishable from any other build.
//  Contains no functional logic.
//  file: src/features/misc/core.hpp
// ============================================================================
#ifndef NOCTUA_LICENSE_MARK_7b94515e3fc5e66bdd3ebc1728e60cfd
#define NOCTUA_LICENSE_MARK_7b94515e3fc5e66bdd3ebc1728e60cfd
namespace noctua_license { namespace mark_7b94515e3fc5e66bdd3ebc1728e60cfd {
    inline constexpr unsigned long long kMarkId = 0x55a873df1c32abe8ull;
    inline constexpr char kMarkFile[] = "src/features/misc/core.hpp";
    inline constexpr unsigned char kMarkEntropy[] = { 0xf5, 0x5d, 0x31, 0x57, 0xef, 0x79, 0x40, 0x95, 0xd6, 0x7e, 0x72, 0xca, 0x95, 0xe5, 0xa1, 0xdb };
    inline constexpr unsigned long  kMarkSalts[]  = { 0xa4cfa449ul, 0x6ec28054ul, 0x1f7553a5ul, 0xdd04d7dcul, 0xc72fbeaaul, 0x0928a8fdul, 0x89a4e3c3ul, 0xa833c20bul };
    [[maybe_unused]] inline unsigned long long mark_digest() noexcept {
        unsigned long long h = 1469598103934665603ull ^ kMarkId;
        for (unsigned char b : kMarkEntropy) { h ^= b; h *= 1099511628211ull; }
        for (unsigned long  s : kMarkSalts)  { h ^= s; h *= 1099511628211ull; }
        return h;
    }
} } // namespace noctua_license::mark_7b94515e3fc5e66bdd3ebc1728e60cfd
#endif // NOCTUA_LICENSE_MARK_7b94515e3fc5e66bdd3ebc1728e60cfd
