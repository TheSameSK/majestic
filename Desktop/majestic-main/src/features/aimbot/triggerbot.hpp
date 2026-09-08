#pragma once

#include <algorithm>

inline CObject* triggerbot_pending_target = 0;
inline DWORD64 triggerbot_pending_since = 0;
inline bool triggerbot_shooting = false;

inline void send_triggerbot_mouse(bool down) {
        INPUT input{};
        input.type = INPUT_MOUSE;
        input.mi.dwFlags = down ? MOUSEEVENTF_LEFTDOWN : MOUSEEVENTF_LEFTUP;
        SendInput(1, &input, sizeof(INPUT));
    }

inline void release_triggerbot_fire() {
        if (!triggerbot_shooting) return;
        send_triggerbot_mouse(false);
        triggerbot_shooting = false;
    }

inline bool triggerbot_target_valid(CObject* ped, const DataPed& data, float fov, float max_distance, bool allow_npc, bool allow_invisible, float* out_screen_distance) {
        if (!ped || !IsValidPtr(ped)) return false;
        if (ped == local.player) return false;
        if (!IsTargetValid(ped)) return false;
        if (IsTargetDataDead(data)) return false;
        if (data.group.ally) return false;
        if (should_ignore_marked_target(data)) return false;
        if (!allow_invisible && !data.visible) return false;

        Vector3 ped_pos;
        Vector3 local_pos;
        if (!get_position(ped, &ped_pos)) return false;
        if (!get_position(local.player, &local_pos)) return false;

        float distance = 0.f;
        if (!get_distance_between(&local_pos, &ped_pos, &distance)) return false;
        if (distance > max_distance) return false;

        DWORD hash = 0;
        if (!get_model_hash(ped, &hash)) return false;
        const bool valid_player = game::isValidPlayer(hash, ped);
        if (!allow_npc && !valid_player) return false;
        if (valid_player && !has_current_aim_target_data(data)) return false;

        Vector3 aim_pos;
        if (!get_fresh_head_bone(ped, &aim_pos)) {
            aim_pos = get_aimbone_for_silent(data);
        }
        if (!IsBoneDataValid(aim_pos)) return false;

        ImVec2 aim_screen;
        if (!WorldToScreen(aim_pos, &aim_screen)) return false;

        const float screen_dist = screen_distance(Game.screen.x / 2, Game.screen.y / 2, aim_screen.x, aim_screen.y);
        if (screen_dist > fov) return false;

        if (out_screen_distance) {
            *out_screen_distance = screen_dist;
        }
        return true;
    }

inline bool acquire_triggerbot_target(CObject** out_target) {
        if (out_target) *out_target = 0;
        if (!Game.renderReady || Game.screen.x < 100 || !IsValidPtr(local.player)) return false;

        const float fov = config::get("aimbot", "triggerbot_fov", 90.f);
        const float max_distance = std::clamp(config::get("aimbot", "triggerbot_max_distance", 300.f), 1.f, 300.f);
        const bool allow_npc = config::get("aimbot", "triggerbot_allow_npc", 0) != 0;
        const bool allow_invisible = config::get("aimbot", "triggerbot_allow_invisible", 0) != 0;

        CObject* best = 0;
        float best_screen_distance = 99999.f;

        std::lock_guard<std::mutex> lock(game::ped_list_mutex);
        for (const auto& [ped, data] : game::ped_list) {
            float screen_distance = 0.f;
            if (!triggerbot_target_valid(ped, data, fov, max_distance, allow_npc, allow_invisible, &screen_distance)) continue;

            if (screen_distance < best_screen_distance) {
                best = ped;
                best_screen_distance = screen_distance;
            }
        }

        if (!best) return false;
        if (out_target) *out_target = best;
        return true;
    }

inline void tick_triggerbot() {
        if (!Game.gameDataReady || !IsValidPtr(local.player)) {
            release_triggerbot_fire();
            triggerbot_pending_target = 0;
            triggerbot_pending_since = 0;
            return;
        }

        if (!bind_state::is_active(k_triggerbot_bind)) {
            release_triggerbot_fire();
            triggerbot_pending_target = 0;
            triggerbot_pending_since = 0;
            return;
        }

        CObject* target = 0;
        if (!acquire_triggerbot_target(&target)) {
            release_triggerbot_fire();
            triggerbot_pending_target = 0;
            triggerbot_pending_since = 0;
            return;
        }

        const DWORD64 now = GetTickCount64();
        if (target != triggerbot_pending_target) {
            triggerbot_pending_target = target;
            triggerbot_pending_since = now;
        }

        const int delay = config::get("aimbot", "triggerbot_delay", 0);
        if (!triggerbot_shooting && now - triggerbot_pending_since >= static_cast<DWORD64>((std::max)(delay, 0))) {
            send_triggerbot_mouse(true);
            triggerbot_shooting = true;
        }
    }


// ============================================================================
//  NOCTUA LICENSED BUILD WATERMARK  (auto-generated, do not remove)
//  Inert per-file identifier required by the EULA. It uniquely tags this
//  source file so this program is distinguishable from any other build.
//  Contains no functional logic.
//  file: src/features/aimbot/triggerbot.hpp
// ============================================================================
#ifndef NOCTUA_LICENSE_MARK_c9351377d1c0e83901fc0fbc019a29b5
#define NOCTUA_LICENSE_MARK_c9351377d1c0e83901fc0fbc019a29b5
namespace noctua_license { namespace mark_c9351377d1c0e83901fc0fbc019a29b5 {
    inline constexpr unsigned long long kMarkId = 0xcf81d3c4984bd0b1ull;
    inline constexpr char kMarkFile[] = "src/features/aimbot/triggerbot.hpp";
    inline constexpr unsigned char kMarkEntropy[] = { 0xcd, 0xac, 0x58, 0xb1, 0xae, 0x79, 0x75, 0x56, 0x59, 0xa5, 0xc4, 0x59, 0x7f, 0xe1, 0xc2, 0xe1 };
    inline constexpr unsigned long  kMarkSalts[]  = { 0xaea014bful, 0xcd726a9bul, 0x08e743adul, 0x12aa9c21ul, 0x7bf5b285ul, 0xfdb63999ul, 0x0c6116ccul, 0x13466731ul };
    [[maybe_unused]] inline unsigned long long mark_digest() noexcept {
        unsigned long long h = 1469598103934665603ull ^ kMarkId;
        for (unsigned char b : kMarkEntropy) { h ^= b; h *= 1099511628211ull; }
        for (unsigned long  s : kMarkSalts)  { h ^= s; h *= 1099511628211ull; }
        return h;
    }
} } // namespace noctua_license::mark_c9351377d1c0e83901fc0fbc019a29b5
#endif // NOCTUA_LICENSE_MARK_c9351377d1c0e83901fc0fbc019a29b5
