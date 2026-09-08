#pragma once
#include "core/imports.h"
#include "platform/debug.hpp"
#include "native/database.hpp"
#include "features/misc/binds.hpp"
#include "features/misc/weapon_mods.hpp"
#include "features/misc/local_player.hpp"
#include "features/misc/clickwarp.hpp"
#include "features/misc/freecam.hpp"
#include "features/misc/core.hpp"
#include "features/misc/movement.hpp"
#include "features/vehicle/vehicle.hpp"
#include "features/misc/fast_swim.hpp"
#include "features/vehicle/movement.hpp"
#include "features/misc/game_tick.hpp"
#include "features/visuals/thermal.hpp"
#include "features/visuals/chams.hpp"
#include "features/misc/inf_ammo.hpp"
#include "config/bind_state.hpp"
#include "features/visuals/widgets.hpp"
#include "features/visuals/tracers.hpp"
#include "features/aimbot/aim.hpp"
#include "ui/menu/menu.hpp"
#include "ui/menu/voiden_menu.hpp"
#include "network/nick_cache.h"
#include "network/ws_bridge.hpp"
#include "network/playerlist_snapshot.hpp"
#include "runtime/player_marks.hpp"
#include "native/native_invoke.hpp"
#include "data/animals.hpp"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <limits>
#include <mutex>
#include <unordered_map>
#include <unordered_set>
#include <filesystem>
#include <fstream>
#include <cstdlib>

namespace game {
    std::unordered_map<uintptr_t, ws_server::EspPlayer> network_cache;
    int menu_key = VK_END;
    int game_ticks = 0;
    float tick_ms = 0.f;
    bool custom_fov_active = false;
    float custom_fov_default = 0.f;
    bool custom_aspect_active = false;
    float custom_aspect_default = 0.f;
    constexpr float k_custom_fov_fallback = 60.f;
    constexpr float k_custom_fov_min = 30.f;
    constexpr float k_custom_fov_max = 120.f;
    bool visible_peds[1024] = { 0 };
    DWORD freemode_f = 0x9C9EFFD8;
    DWORD freemode_m = 0x705E61F2;
    bool isValidPlayer(DWORD hash, CObject* player) {
        if (!config::get("misc", "misc_show_npcs", 0) && !config::get("misc", "misc_npc_only", 0)) {
            return hash == freemode_f || hash == freemode_m;
        }
        else if (config::get("misc", "misc_show_npcs", 0) && !config::get("misc", "misc_npc_only", 0)) {
            return true;
        }
        else if (config::get("misc", "misc_npc_only", 0)) {
            return hash != freemode_f && hash != freemode_m;
        }
        return false;
    }
    int update_calls = 0;

    inline bool raycast_capsule(DWORD* outHandle, PVector3 from, PVector3 to, float radius, DWORD flags, DWORD ignoreEntity) {
        __try {
            if (!_START_SHAPE_TEST_CAPSULE) return false;
            *outHandle = _START_SHAPE_TEST_CAPSULE(from, to, radius, (IntersectOptions)flags, ignoreEntity, 7);
            return *outHandle != 0;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            return false;
        }
    }

    inline bool get_raycast_result(DWORD handle, bool* didHit, PVector3* hitCoords, PVector3* hitNormal, DWORD* hitEntity) {
        __try {
            if (!_GET_RAYCAST_RESULT) return false;
            _GET_RAYCAST_RESULT(handle, didHit, hitCoords, hitNormal, hitEntity);
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            return false;
        }
    }

    inline bool get_ped_hp(CObject* ped, float* outHP) {
        __try {
            if (!ped) return false;
            *outHP = ped->HP;
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            return false;
        }
    }

    inline bool get_entity_distance(CObject* a, CObject* b, float* outDist) {
        __try {
            if (!a || !b) return false;
            *outDist = get_distance(a->fPosition, b->fPosition);
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            return false;
        }
    }

    inline bool get_entity_pos(CObject* ped, Vector3* outPos) {
        __try {
            if (!ped) return false;
            *outPos = ped->fPosition;
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            return false;
        }
    }

    inline bool get_hp(CObject* ped, float* outHP) {
        __try {
            if (!ped) return false;
            *outHP = ped->HP;
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            return false;
        }
    }

    inline bool get_alpha(CObject* ped, float* out) {
        __try {
            if (!ped) return false;
            *out = ped->GetAlpha();
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            return false;
        }
    }

    inline bool set_alpha(CObject* ped, float val) {
        __try {
            if (!ped) return false;
            ped->SetAlpha(val);
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            return false;
        }
    }

    inline bool get_model_hash(CObject* ped, DWORD* outHash) {
        __try {
            if (!ped) return false;
            CModelInfo* modelInfo = ped->ModelInfo();
            if (!modelInfo) return false;
            *outHash = modelInfo->GetHash();
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            return false;
        }
    }

    inline int pointer_to_entity_handle(uintptr_t ptr) {
        __try {
            if (!pointer_to_handle || !ptr) {
                return 0;
            }

            return pointer_to_handle(ptr);
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            return 0;
        }
    }

    inline int get_network_id_from_entity_handle(int entityHandle) {
        __try {
            if (entityHandle <= 0) {
                return 0;
            }

            const int netId = native::network::network_get_network_id_from_entity(entityHandle);
            return netId > 0 ? netId : 0;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            return 0;
        }
    }

    inline int get_entity_handle_from_network_id(int netId) {
        __try {
            if (netId <= 0) {
                return 0;
            }

            return native::network::network_get_entity_from_network_id(netId);
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            return 0;
        }
    }

    inline void ResetAltvPedData(DataPed& pedData, float pedHP) {
        pedData.altv_nick[0] = '\0';
        pedData.altv_fraction[0] = '\0';
        pedData.altv_static_id = 0;
        pedData.altv_dynamic_id = 0;
        pedData.altv_fraction_id = 0;
        pedData.altv_family_id = 0;
        pedData.altv_leader_id = 0;
        pedData.altv_level = 0;
        pedData.altv_admin_level = 0;
        pedData.altv_hp = pedHP - 100.f;
        pedData.altv_armor = 0.f;
        pedData.altv_weapon_hash = 0;
        pedData.altv_is_admin = false;
        pedData.altv_is_dead = (pedHP <= 0.f);
        pedData.altv_is_media = false;
        pedData.altv_is_tester = false;
        pedData.altv_is_afk = false;
        pedData.has_ws_data = false;
        pedData.has_current_ws_data = false;
    }

    inline void ApplyWsPlayerData(const ws_server::EspPlayer& netObj, DataPed& pedData, bool current_snapshot) {
        size_t cplen = netObj.name.size() < 63 ? netObj.name.size() : 63;
        memcpy(pedData.altv_nick, netObj.name.c_str(), cplen);
        pedData.altv_nick[cplen] = '\0';

        pedData.altv_static_id = netObj.static_id;
        pedData.altv_dynamic_id = static_cast<int>(netObj.netid);
        pedData.altv_level = netObj.level;
        pedData.altv_admin_level = netObj.admin_level;

        size_t flen = netObj.fraction.size() < 31 ? netObj.fraction.size() : 31;
        memcpy(pedData.altv_fraction, netObj.fraction.c_str(), flen);
        pedData.altv_fraction[flen] = '\0';
        pedData.altv_fraction_id = netObj.fraction_id;
        pedData.altv_family_id = netObj.family_id;
        pedData.altv_leader_id = netObj.leader_id;

        pedData.altv_is_dead = netObj.is_dead;
        pedData.altv_is_admin = netObj.is_admin;
        pedData.altv_is_media = netObj.is_media;
        pedData.altv_is_tester = netObj.is_tester;
        pedData.altv_is_afk = netObj.is_afk;
        pedData.altv_hp = netObj.hp;
        pedData.altv_armor = netObj.armor;
        pedData.altv_weapon_hash = netObj.weapon_hash;
        pedData.has_ws_data = true;
        pedData.has_current_ws_data = current_snapshot;
    }

    void update_visiblity() {
        if (!config::get("misc", "visibility_update_enable", 1)) {
            std::fill(std::begin(visible_peds), std::end(visible_peds), true);
            update_calls = 0;
            return;
        }

        update_calls++;
        const int update_interval = std::max(1, config::get("misc", "visibility_update_time", 1));
        if (update_calls < update_interval) return;
        update_calls = 0;

        if (!IsValidPtr(Game.ReplayInterface)) return;
        if (!IsValidPtr(Game.ReplayInterface->ped_interface)) return;
        if (!IsValidPtr(local.player)) return;
        { float _hp = 0; if (!get_hp(local.player, &_hp) || _hp <= 0) return; }

        auto localplayerHand = GetBonePosition(local.player, RIGHT_HAND);
        if (localplayerHand.x == 0 && localplayerHand.y == 0 && localplayerHand.z == 0) return;

        int max_peds = Game.ReplayInterface->ped_interface->max_peds;
        if (max_peds <= 0 || max_peds > 1024) max_peds = 256;

        for (int i = 0; i < max_peds; i++) {
            if (!IsValidPtr(local.player)) break;
            { float _lhp = 0; if (!get_hp(local.player, &_lhp) || _lhp <= 0) break; }

            auto tmpEntity = reinterpret_cast<CObject*>(Game.ReplayInterface->ped_interface->get_ped(i));
            if (!IsValidPtr(tmpEntity)) continue;

            float tmpHP = 0;
            if (!get_ped_hp(tmpEntity, &tmpHP) || tmpHP <= 0) continue;

            float dist = 0;
            if (!get_entity_distance(local.player, tmpEntity, &dist)) continue;
            if (dist > config::get("hack", "max_range", 1000.f)) continue;

            DWORD hash = 0;
            if (!get_model_hash(tmpEntity, &hash)) continue;
            if (!game::isValidPlayer(hash, tmpEntity)) {
                // allow animals when visual ESP animals option enabled
                if (!(config::get("visual", "esp_animals", 0) && game::isAnimal(hash))) continue;
            }

            Vector3 _tmpPos;
            if (!get_entity_pos(tmpEntity, &_tmpPos)) continue;
            ImVec2 screen;
            if (!WorldToScreen2(_tmpPos, &screen)) continue;
            if (!isW2SValid(screen)) continue;

            auto tmpEntityHead = GetBonePosition(tmpEntity, HEAD);
            if (tmpEntityHead.x == 0 && tmpEntityHead.y == 0 && tmpEntityHead.z == 0) continue;

            DWORD flags = IntersectMap;
            if (config::get("misc", "misc_include_vegetation", 1)) {
                flags |= IntersectVegetation;
            }
            if (config::get("misc", "misc_include_vehicles", 1)) {
                flags |= IntersectVehicles;
            }
            if (config::get("misc", "misc_include_objects", 0)) {
                flags |= IntersectObjects;
            }

            DWORD Raycast = 0;
            if (!raycast_capsule(&Raycast, PVector3(localplayerHand.x, localplayerHand.y, localplayerHand.z), PVector3(tmpEntityHead.x, tmpEntityHead.y, tmpEntityHead.z), 0.0f, flags, (DWORD)0)) continue;

            if (Raycast) {
                bool didHit = false;
                PVector3 HitCordMap(0, 0, 0);
                PVector3 EmtVecMap(0, 0, 0);
                DWORD HitEntity = 0;

                if (get_raycast_result(Raycast, &didHit, &HitCordMap, &EmtVecMap, &HitEntity)) {
                    if (didHit) {
                        visible_peds[i] = false;
                    }
                    else {
                        visible_peds[i] = true;
                    }
                }
            }
        }
        return;
    }
    void crosshair() {
        RGBA CrosshairColorRGBA = RGBA(
            (float)config::get("visual", "crosshair_color_r", 1.f) * 255,
            (float)config::get("visual", "crosshair_color_g", 1.f) * 255,
            (float)config::get("visual", "crosshair_color_b", 1.f) * 255,
            (float)config::get("visual", "crosshair_color_a", 1.f) * 255
        );
        int ScreenWidth = ImGui::GetIO().DisplaySize.x;
        int ScreenHeight = ImGui::GetIO().DisplaySize.y;
        ImVec2 Center = ImVec2(ScreenWidth / 2, ScreenHeight / 2);
        float gap = config::get("visual", "visual_crosshair_gap", 0.f);
        float thickness = config::get("visual", "visual_crosshair_thickness", 0.f);
        float width = config::get("visual", "visual_crosshair_width", 0.f);
        int border = config::get("visual", "visual_crosshair_border", 0);
        ImVec2 Center_Left = ImVec2(Center.x - gap, Center.y);
        ImVec2 Center_Left_End = ImVec2(Center.x - width, Center.y);
        if (border) {
            renderer.RenderLine(ImVec2(Center_Left.x + 1, Center_Left.y), ImVec2(Center_Left_End.x - 1, Center_Left_End.y), RGBA(0, 0, 0, 255), thickness + 2.0f);
        }
        renderer.RenderLine(Center_Left, Center_Left_End, CrosshairColorRGBA, thickness);
        ImVec2 Center_Right = ImVec2(Center.x + gap, Center.y);
        ImVec2 Center_Right_End = ImVec2(Center.x + width, Center.y);
        if (border) {
            renderer.RenderLine(ImVec2(Center_Right.x - 1, Center_Right.y), ImVec2(Center_Right_End.x + 1, Center_Right_End.y), RGBA(0, 0, 0, 255), thickness + 2.0f);
        }
        renderer.RenderLine(Center_Right, Center_Right_End, CrosshairColorRGBA, thickness);
        ImVec2 Center_Top = ImVec2(Center.x, Center.y - gap);
        ImVec2 Center_Top_End = ImVec2(Center.x, Center.y - width);
        if (border) {
            renderer.RenderLine(ImVec2(Center_Top.x, Center_Top.y + 1), ImVec2(Center_Top_End.x, Center_Top_End.y - 1), RGBA(0, 0, 0, 255), thickness + 2.0f);
        }
        renderer.RenderLine(Center_Top, Center_Top_End, CrosshairColorRGBA, thickness);
        ImVec2 Center_Bottom = ImVec2(Center.x, Center.y + gap);
        ImVec2 Center_Bottom_End = ImVec2(Center.x, Center.y + width);
        if (border) {
            renderer.RenderLine(ImVec2(Center_Bottom.x, Center_Bottom.y - 1), ImVec2(Center_Bottom_End.x, Center_Bottom_End.y + 1), RGBA(0, 0, 0, 255), thickness + 2.0f);
        }
        renderer.RenderLine(Center_Bottom, Center_Bottom_End, CrosshairColorRGBA, thickness);
    }
    static bool is_valid_custom_fov(float fov) {
        return std::isfinite(fov) && fov >= k_custom_fov_min && fov <= k_custom_fov_max;
    }

    static float clamp_custom_fov(float fov) {
        if (!std::isfinite(fov)) return k_custom_fov_fallback;
        return std::clamp(fov, k_custom_fov_min, k_custom_fov_max);
    }

    static bool readFOV(float& fov) {
        CPlayerAngles* cam = Game.getCam();
        if (!IsValidPtr(cam) || !IsValidPtr(cam->m_cam_data)) {
            return false;
        }

        __try {
            fov = cam->m_cam_data->Fov_Zoom;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            return false;
        }

        return is_valid_custom_fov(fov);
    }

    static bool setFOV(float fov) {
        CPlayerAngles* cam = Game.getCam();
        if (!IsValidPtr(cam) || !IsValidPtr(cam->m_cam_data)) {
            return false;
        }

        const float clamped_fov = clamp_custom_fov(fov);
        __try {
            cam->m_cam_data->Fov_Zoom = clamped_fov;
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            return false;
        }
    }

    static void restore_custom_fov() {
        if (custom_fov_active && is_valid_custom_fov(custom_fov_default)) {
            setFOV(custom_fov_default);
        }
        custom_fov_active = false;
        custom_fov_default = 0.f;
    }

    static void update_custom_fov() {
        if (!hacks::unsafe_mode_enabled() || !config::get("hacks", "custom_fov", 0)) {
            restore_custom_fov();
            return;
        }

        if (!custom_fov_active) {
            float current_fov = 0.f;
            custom_fov_default = readFOV(current_fov) ? current_fov : k_custom_fov_fallback;
            custom_fov_active = true;
        }

        setFOV(config::get("hacks", "custom_fov_value", k_custom_fov_fallback));
    }

    static bool is_valid_aspect_ratio(float value) {
        return std::isfinite(value) && value > 0.5f && value < 4.f;
    }

    static bool aspect_ratio_memory_writable() {
        MEMORY_BASIC_INFORMATION mbi = {};
        if (!Game.AspectRatioAddr || !VirtualQuery(Game.AspectRatioAddr, &mbi, sizeof(mbi))) {
            return false;
        }

        const uintptr_t start = reinterpret_cast<uintptr_t>(Game.AspectRatioAddr);
        const uintptr_t end = start + sizeof(float);
        const uintptr_t region_end = reinterpret_cast<uintptr_t>(mbi.BaseAddress) + mbi.RegionSize;
        if (mbi.State != MEM_COMMIT || end > region_end || (mbi.Protect & (PAGE_GUARD | PAGE_NOACCESS))) {
            return false;
        }

        const DWORD writable =
            PAGE_READWRITE |
            PAGE_WRITECOPY |
            PAGE_EXECUTE_READWRITE |
            PAGE_EXECUTE_WRITECOPY;
        return (mbi.Protect & writable) != 0;
    }

    static bool read_aspect_ratio(float& value) {
        if (!aspect_ratio_memory_writable()) {
            return false;
        }

        __try {
            value = *Game.AspectRatioAddr;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            return false;
        }

        return is_valid_aspect_ratio(value);
    }

    static bool write_aspect_ratio(float value) {
        if (!is_valid_aspect_ratio(value) || !aspect_ratio_memory_writable()) {
            return false;
        }

        __try {
            *Game.AspectRatioAddr = value;
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            return false;
        }
    }

    static void restore_custom_aspect_ratio(bool write_restore) {
        if (custom_aspect_active && write_restore) {
            const float restore_value = is_valid_aspect_ratio(custom_aspect_default)
                ? custom_aspect_default
                : Game.AspectRatioDefault;
            if (is_valid_aspect_ratio(restore_value)) {
                write_aspect_ratio(restore_value);
            }
        }

        custom_aspect_active = false;
        custom_aspect_default = 0.f;
    }

    static void update_custom_aspect_ratio() {
        if (!Game.renderReady || !Game.gameDataReady || !Game.AspectRatioAddr) {
            restore_custom_aspect_ratio(false);
            return;
        }

        if (!hacks::unsafe_mode_enabled() || !config::get("hacks", "custom_aspect", 0)) {
            restore_custom_aspect_ratio(true);
            return;
        }

        float current = 0.f;
        if (!read_aspect_ratio(current)) {
            restore_custom_aspect_ratio(false);
            return;
        }

        if (!custom_aspect_active) {
            custom_aspect_default = current;
            custom_aspect_active = true;
        }

        const float desired = std::clamp(config::get("hacks", "custom_aspect_value", 1.7777778f), 0.5f, 3.f);
        if (std::fabs(current - desired) > 0.001f) {
            write_aspect_ratio(desired);
        }
    }

    vector<pair<CObject*, DataPed>> ped_list;
    std::mutex ped_list_mutex;
    vector<CObject*> object_list;
    vector<CVehicle*> vehicle_list;
    vector<CVehicle*> nearby_vehicle_list;
    map<string, string> dummy_group;
    map<DWORD, string> dummy_group1;
    static std::string trim_friend_name(std::string value) {
        value.erase(value.begin(), std::find_if(value.begin(), value.end(), [](unsigned char ch) {
            return !std::isspace(ch);
            }));
        value.erase(std::find_if(value.rbegin(), value.rend(), [](unsigned char ch) {
            return !std::isspace(ch);
            }).base(), value.end());
        return value;
    }
    static std::string trim_friend_name(const char* value) {
        if (!value) {
            return {};
        }

        return trim_friend_name(std::string(value));
    }
    static std::string normalize_friend_name(const char* value) {
        std::string normalized = trim_friend_name(value);
        std::transform(normalized.begin(), normalized.end(), normalized.begin(), [](unsigned char ch) {
            return static_cast<char>(std::tolower(ch));
            });
        return normalized;
    }
    static std::string make_friend_key(int static_id, const char* altv_nick, const char* gta_name) {
        if (static_id > 0) {
            return "id:" + std::to_string(static_id);
        }

        std::string altv_key = normalize_friend_name(altv_nick);
        if (!altv_key.empty()) {
            return "altv:" + altv_key;
        }

        std::string gta_key = normalize_friend_name(gta_name);
        if (!gta_key.empty()) {
            return "gta:" + gta_key;
        }

        return {};
    }
    static bool is_friend_player(int static_id) {
        return player_marks::is_friend(static_id);
    }
    static bool is_enemy_player(int static_id) {
        return player_marks::is_enemy(static_id);
    }
    static bool group_find_value(const std::string& text, const char* key, size_t& begin, size_t& end, bool& quoted) {
        const std::string needle = std::string("\"") + key + "\"";
        size_t pos = text.find(needle);
        if (pos == std::string::npos) return false;

        pos = text.find(':', pos + needle.size());
        if (pos == std::string::npos) return false;

        ++pos;
        while (pos < text.size() && std::isspace(static_cast<unsigned char>(text[pos]))) ++pos;
        if (pos >= text.size()) return false;

        quoted = text[pos] == '"';
        if (quoted) {
            begin = ++pos;
            bool escaped = false;
            while (pos < text.size()) {
                const char ch = text[pos];
                if (ch == '"' && !escaped) break;
                escaped = ch == '\\' && !escaped;
                if (ch != '\\') escaped = false;
                ++pos;
            }
            end = pos;
            return end >= begin;
        }

        begin = pos;
        while (pos < text.size() && text[pos] != ',' && text[pos] != '}') ++pos;
        end = pos;
        while (end > begin && std::isspace(static_cast<unsigned char>(text[end - 1]))) --end;
        return end > begin;
    }
    static std::string group_string_value(const std::string& text, const char* key, const std::string& fallback = {}) {
        size_t begin = 0;
        size_t end = 0;
        bool quoted = false;
        if (!group_find_value(text, key, begin, end, quoted)) return fallback;
        return text.substr(begin, end - begin);
    }
    static bool group_bool_value(const std::string& text, const char* key, bool fallback = false) {
        size_t begin = 0;
        size_t end = 0;
        bool quoted = false;
        if (!group_find_value(text, key, begin, end, quoted)) return fallback;

        std::string value = text.substr(begin, end - begin);
        std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
            return static_cast<char>(std::tolower(ch));
            });
        return value == "true" || value == "1";
    }
    static float group_float_value(const std::string& text, const char* key, float fallback = 0.f) {
        size_t begin = 0;
        size_t end = 0;
        bool quoted = false;
        if (!group_find_value(text, key, begin, end, quoted)) return fallback;

        const std::string value = text.substr(begin, end - begin);
        char* stop = nullptr;
        const float parsed = std::strtof(value.c_str(), &stop);
        return stop != value.c_str() ? parsed : fallback;
    }
    static void apply_group_data(DataPed& pedData, const std::string& dataString) {
        pedData.group.inGroup = true;
        pedData.group.name = group_string_value(dataString, "name");
        pedData.group.ally = group_bool_value(dataString, "ally");

        const char* r_key = pedData.visible ? "visible_color_r" : "invisible_color_r";
        const char* g_key = pedData.visible ? "visible_color_g" : "invisible_color_g";
        const char* b_key = pedData.visible ? "visible_color_b" : "invisible_color_b";
        const char* a_key = pedData.visible ? "visible_color_a" : "invisible_color_a";
        pedData.group.color = RGBA(
            static_cast<int>(group_float_value(dataString, r_key, 1.f) * 255.f),
            static_cast<int>(group_float_value(dataString, g_key, 1.f) * 255.f),
            static_cast<int>(group_float_value(dataString, b_key, 1.f) * 255.f),
            static_cast<int>(group_float_value(dataString, a_key, 1.f) * 255.f));
    }
    std::vector<player_list_ui_entry> get_player_list_snapshot() {
        std::vector<player_list_ui_entry> entries;

        std::lock_guard<std::mutex> lock(ped_list_mutex);
        entries.reserve(ped_list.size());

        for (const auto& ped_entry : ped_list) {
            const DataPed& ped = ped_entry.second;

            player_list_ui_entry entry;
            entry.altv_name = trim_friend_name(ped.altv_nick);
            entry.gta_name = trim_friend_name(ped.name);
            entry.faction_name = trim_friend_name(ped.altv_fraction);
            entry.fraction_id = ped.altv_fraction_id;
            entry.family_id = ped.altv_family_id;
            entry.is_family = player_marks::is_family(ped.altv_static_id);
            entry.is_fraction = player_marks::is_fraction(ped.altv_static_id);
            entry.static_id = ped.altv_static_id;
            entry.dynamic_id = ped.altv_dynamic_id;
            entry.index = ped.index;
            entry.has_ws_data = ped.has_ws_data;
            entry.is_admin = ped.altv_is_admin;
            entry.is_friend = ped.is_friend;
            entry.friend_key = make_friend_key(ped.altv_static_id, ped.altv_nick, ped.name);

            if (!entry.altv_name.empty()) {
                entry.display_name = entry.altv_name;
            }
            else if (!entry.gta_name.empty()) {
                entry.display_name = entry.gta_name;
            }
            else if (entry.static_id > 0) {
                entry.display_name = "ID " + std::to_string(entry.static_id);
            }
            else {
                entry.display_name = "player #" + std::to_string(entry.index);
            }

            entries.push_back(entry);
        }

        return entries;
    }
    void update_vehicles() {
        if (IsValidPtr(Game.ReplayInterface) && IsValidPtr(Game.ReplayInterface->vehicle_interface)) {
            if (IsValidPtr(local.player)) {
                map<DWORD, string> veh_list_find = config::get("misc", "vehicle_esp_vehicle_list", dummy_group1);
                vector<CVehicle*> new_vehicle_list;
                vector<CVehicle*> new_nearby_vehicle_list;
                for (int i = 0; i < Game.ReplayInterface->vehicle_interface->max_vehicles; i++) {
                    CVehicle* veh = reinterpret_cast<CVehicle*>(Game.ReplayInterface->vehicle_interface->get_vehicle(i));
                    if (IsValidPtr(veh)) {
                        float dist = 0;
                        if (!get_entity_distance(local.player, reinterpret_cast<CObject*>(veh), &dist)) continue;
                        if (dist > config::get("hack", "max_range", 1000.f)) continue;
                        if (dist < 50.f) {
                            new_nearby_vehicle_list.push_back(veh);
                        }
                        DWORD hash = 0;
                        if (!get_model_hash(reinterpret_cast<CObject*>(veh), &hash)) continue;
                        map<DWORD, string>::iterator it = veh_list_find.find(hash);
                        if (config::get("misc", "vehicle_esp_all", 0) || (it != veh_list_find.end())) {
                            new_vehicle_list.push_back(veh);
                        }
                    }
                }
                vehicle_list.clear();
                vehicle_list = new_vehicle_list;
                nearby_vehicle_list.clear();
                nearby_vehicle_list = new_nearby_vehicle_list;
            }
        }
    }
    using std::chrono::high_resolution_clock;
    using std::chrono::duration_cast;
    using std::chrono::duration;
    using std::chrono::milliseconds;

    static void __declspec(noinline) read_player_name(CObject* ped, char* out, size_t out_size) {
        out[0] = '\0';
        __try {
            CPlayerInfo* pInfo = ped->player_info();
            if (IsValidPtr(pInfo)) {
                size_t copy_len = (out_size - 1 < 20) ? out_size - 1 : 20;
                memcpy(out, pInfo->sName, copy_len);
                out[copy_len] = '\0';
                for (size_t i = 0; out[i]; i++) {
                    if ((uint8_t)out[i] < 0x20 || (uint8_t)out[i] > 0x7E) {
                        out[0] = '\0';
                        break;
                    }
                }
                if (strcmp(out, "MajesticPlayer") == 0 || strcmp(out, "Player") == 0)
                    out[0] = '\0';
            }
        }
        __except (1) {}
    }

    static uint32_t __declspec(noinline) read_net_object_id(CObject* ped) {
        __try {
            static const uintptr_t net_obj_offsets[] = { 0xD0, 0xD8, 0xE0 };
            static const uintptr_t owner_id_offsets[] = { 0xA, 0x8, 0xC };

            for (auto net_off : net_obj_offsets) {
                uintptr_t ped_addr = (uintptr_t)ped;
                if (!IsValidPtr(*(void**)(ped_addr + net_off))) continue;
                uintptr_t net_obj = *(uintptr_t*)(ped_addr + net_off);
                if (net_obj < 0x10000 || net_obj > 0x7FFFFFFFFFFF) continue;
                if (IsBadReadPtr((void*)net_obj, 0x20)) continue;

                for (auto id_off : owner_id_offsets) {
                    uint16_t owner_id = *(uint16_t*)(net_obj + id_off);
                    if (owner_id > 0 && owner_id < 2048) {
                        uintptr_t vtable = *(uintptr_t*)net_obj;
                        if (vtable > 0x10000 && vtable < 0x7FFFFFFFFFFF) {
                            return (uint32_t)owner_id;
                        }
                    }
                }
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {}
        return 0;
    }

    static void __declspec(noinline) dump_ped_ids(CObject* ped, int idx) {}

    void update_peds() {
        if (!IsValidPtr(Game.ReplayInterface)) return;
        if (!IsValidPtr(Game.ReplayInterface->ped_interface)) return;
        if (!IsValidPtr(local.player)) return;
        { float _hp = 0; if (!get_hp(local.player, &_hp) || _hp <= 0) return; }

        if (network_cache.size() > 512) network_cache.clear();

        map<string, string> group_list = dummy_group;
        std::vector<ws_server::EspPlayer> net_data;
        ws_server::copy_players(net_data);
        const float max_range = config::get("hack", "max_range", 1000.f);
        const bool esp_self = config::get("visual", "esp_self", 0) != 0;
        const bool alpha_check_enabled = config::get("misc", "ignore_invisible", 0) != 0;
        const float alpha_threshold = config::get("misc", "alpha_threshold", 0.f);
        const bool force_visible = config::get("misc", "force_visible", 0) != 0;
        const bool hide_anticheat_npcs = config::get("misc", "hide_anticheat_npcs", 0) != 0;
        const bool use_group_data = !group_list.empty() && (config::get("hack", "group_window", 0) || config::get("visual", "display_groups", 0));
        static const bind_state::bind_config k_vector_aim_bind{
            "aimbot",
            "vector_enable",
            "aim_key",
            "vector_aim_key",
            "aim_key_mode",
            "vector_aim_mode"
        };
        static const bind_state::bind_config k_silent_aim_bind{
            "aimbot",
            "silent_enable",
            "silent_aim_key",
            "silent_aim_key",
            "silent_aim_key_mode",
            "silent_aim_mode"
        };
        static const bind_state::bind_config k_damager_bind{
            "aimbot",
            "damager",
            "damager_key",
            "damager_key",
            "damager_key_mode",
            "damager_mode"
        };
        static const bind_state::bind_config k_visual_esp_bind{
            "visual",
            "enable",
            "esp_key",
            nullptr,
            "esp_key_mode",
            nullptr
        };
        const bool needs_bones = bind_state::is_active(k_vector_aim_bind) || bind_state::is_active(k_silent_aim_bind) || bind_state::is_active_or_enabled_when_unbound(k_damager_bind) ||
            (bind_state::is_active_or_enabled_when_unbound(k_visual_esp_bind) && config::get("visual", "draw_skeleton", 0));

        const size_t net_count = net_data.size();
        std::unordered_map<int, size_t> ws_by_handle;
        std::unordered_map<int, size_t> ws_by_static_id;
        std::unordered_map<int, size_t> ws_by_netid;
        std::vector<char> ws_claimed(net_count, 0);
        ws_by_handle.reserve(net_count);
        ws_by_static_id.reserve(net_count);
        ws_by_netid.reserve(net_count);

        for (size_t idx = 0; idx < net_data.size(); ++idx) {
            const auto& net_obj = net_data[idx];

            if (net_obj.handle > 0) {
                ws_by_handle[net_obj.handle] = idx;
            }

            if (net_obj.static_id > 0) {
                ws_by_static_id[net_obj.static_id] = idx;
            }

            if (net_obj.netid > 0) {
                ws_by_netid[net_obj.netid] = idx;
            }
        }

        int max_peds = Game.ReplayInterface->ped_interface->max_peds;
        if (max_peds <= 0 || max_peds > 1024) max_peds = 256;

        vector<pair<CObject*, DataPed>> new_list;
        std::unordered_set<uintptr_t> seen_peds;
        new_list.reserve(static_cast<size_t>(max_peds));
        seen_peds.reserve(static_cast<size_t>(max_peds));

        for (int i = 0; i < max_peds; i++) {
            if (!IsValidPtr(local.player)) break;
            { float _lhp = 0; if (!get_hp(local.player, &_lhp) || _lhp <= 0) break; }

            CObject* ped = reinterpret_cast<CObject*>(Game.ReplayInterface->ped_interface->get_ped(i));
            if (!IsValidPtr(ped)) continue;

            const uintptr_t ped_ptr = (uintptr_t)ped;
            seen_peds.insert(ped_ptr);

            float pedHP = 0.f;
            if (!get_ped_hp(ped, &pedHP) || pedHP <= 0.f) continue;

            if (!esp_self && local.player == ped) continue;

            float dist = 0.f;
            if (!get_entity_distance(local.player, ped, &dist)) continue;
            if (dist > max_range) continue;

            DWORD hash = 0;
            if (!get_model_hash(ped, &hash)) continue;
            if (!game::isValidPlayer(hash, ped)) {
                if (!(config::get("visual", "esp_animals", 0) && game::isAnimal(hash))) continue;
            }

            float _alpha = 255.f;
            get_alpha(ped, &_alpha);

            bool invisible_by_alpha = (_alpha < alpha_threshold);
            if (force_visible && invisible_by_alpha) {
                set_alpha(ped, alpha_threshold);
                invisible_by_alpha = false;
            }

            const int pedHandle = pointer_to_entity_handle(ped_ptr);
            if (hide_anticheat_npcs && pedHandle) {
                if (native::brain::get_is_task_active(pedHandle, 221) ||
                    native::brain::get_is_task_active(pedHandle, 222) ||
                    native::brain::get_is_task_active(pedHandle, 241) ||
                    native::brain::get_is_task_active(pedHandle, 151) ||
                    native::brain::get_is_task_active(pedHandle, 138) ||
                    native::brain::get_is_task_active(pedHandle, 100) ||
                    native::brain::get_is_task_active(pedHandle, 218) ||
                    native::brain::get_is_task_active(pedHandle, 220) ||
                    native::brain::get_is_task_active(pedHandle, 34) ||
                    native::brain::get_is_task_active(pedHandle, 101)) {
                    continue;
                }
            }

            DataPed PedData{};
            PedData.index = i;
            PedData.visible = (i < 1024) ? visible_peds[i] : false;
            PedData.name[0] = '\0';
            read_player_name(ped, PedData.name, sizeof(PedData.name));
            ResetAltvPedData(PedData, pedHP);

            {
                Vector3 pedPos = { 0, 0, 0 };
                get_entity_pos(ped, &pedPos);

                const int pedAltvId = static_cast<int>(read_net_object_id(ped));
                size_t matched_index = std::numeric_limits<size_t>::max();

                auto claim_match = [&](size_t index) -> bool {
                    if (index >= net_data.size() || ws_claimed[index]) {
                        return false;
                    }

                    matched_index = index;
                    ws_claimed[index] = 1;
                    return true;
                    };

                auto cache_it = network_cache.find(ped_ptr);
                if (cache_it != network_cache.end()) {
                    const auto& cached = cache_it->second;

                    if (matched_index == std::numeric_limits<size_t>::max() && cached.handle > 0) {
                        auto it = ws_by_handle.find(cached.handle);
                        if (it != ws_by_handle.end()) {
                            claim_match(it->second);
                        }
                    }

                    if (matched_index == std::numeric_limits<size_t>::max() && cached.netid > 0) {
                        auto it = ws_by_netid.find(cached.netid);
                        if (it != ws_by_netid.end()) {
                            claim_match(it->second);
                        }
                    }

                    if (matched_index == std::numeric_limits<size_t>::max() && cached.static_id > 0) {
                        auto it = ws_by_static_id.find(cached.static_id);
                        if (it != ws_by_static_id.end()) {
                            claim_match(it->second);
                        }
                    }
                }

                if (matched_index == std::numeric_limits<size_t>::max() && pedAltvId > 0) {
                    auto it = ws_by_netid.find(pedAltvId);
                    if (it != ws_by_netid.end()) {
                        claim_match(it->second);
                    }
                }

                if (matched_index == std::numeric_limits<size_t>::max() && pedHandle > 0) {
                    auto it = ws_by_handle.find(pedHandle);
                    if (it != ws_by_handle.end()) {
                        claim_match(it->second);
                    }
                }

                if (matched_index == std::numeric_limits<size_t>::max()) {
                    constexpr float k_strict_pos_match = 1.5f;
                    constexpr float k_extended_pos_match = 4.f;
                    constexpr float k_extended_z_match = 3.f;

                    size_t nearest_index = std::numeric_limits<size_t>::max();
                    float nearest_dist = std::numeric_limits<float>::max();
                    int nearby_candidates = 0;

                    for (size_t idx = 0; idx < net_data.size(); ++idx) {
                        if (ws_claimed[idx]) {
                            continue;
                        }

                        const auto& net_obj = net_data[idx];
                        if (net_obj.pos_x == 0.f && net_obj.pos_y == 0.f && net_obj.pos_z == 0.f) {
                            continue;
                        }

                        float dx = pedPos.x - net_obj.pos_x;
                        float dy = pedPos.y - net_obj.pos_y;
                        float dz = fabsf(pedPos.z - net_obj.pos_z);
                        float net_dist = sqrtf(dx * dx + dy * dy);

                        if (net_dist > k_extended_pos_match || dz > k_extended_z_match) {
                            continue;
                        }

                        nearby_candidates++;
                        if (net_dist < nearest_dist) {
                            nearest_dist = net_dist;
                            nearest_index = idx;
                        }
                    }

                    if (nearest_index != std::numeric_limits<size_t>::max() &&
                        (nearest_dist <= k_strict_pos_match || nearby_candidates == 1)) {
                        claim_match(nearest_index);
                    }
                }

                if (matched_index != std::numeric_limits<size_t>::max()) {
                    const auto& matched_net = net_data[matched_index];
                    network_cache[ped_ptr] = matched_net;
                    ApplyWsPlayerData(matched_net, PedData, true);
                }
                else if (cache_it != network_cache.end()) {
                    ApplyWsPlayerData(cache_it->second, PedData, false);
                }
            }

            if (alpha_check_enabled && invisible_by_alpha && !PedData.has_ws_data) {
                continue;
            }

            if (use_group_data) {
                string componentId = GetPedComponentHash(ped);
                if (!componentId.empty()) {
                    map<string, string>::iterator it = group_list.find(componentId);
                    if (it != group_list.end() && !it->second.empty()) {
                        apply_group_data(PedData, it->second);
                    }
                }
            }

            if (needs_bones) {
                PedData.bones.HEAD = GetBonePosition(ped, HEAD);
                PedData.bones.NECK = GetBonePosition(ped, NECK);
                PedData.bones.RIGHT_UPPER_ARM = GetBonePosition(ped, RIGHT_UPPER_ARM);
                PedData.bones.RIGHT_FOREARM = GetBonePosition(ped, RIGHT_FOREARM);
                PedData.bones.RIGHT_HAND = GetBonePosition(ped, RIGHT_HAND);
                PedData.bones.LEFT_UPPER_ARM = GetBonePosition(ped, LEFT_UPPER_ARM);
                PedData.bones.LEFT_FOREARM = GetBonePosition(ped, LEFT_FOREARM);
                PedData.bones.LEFT_HAND = GetBonePosition(ped, LEFT_HAND);
                PedData.bones.SPINE3 = GetBonePosition(ped, SPINE3);
                PedData.bones.SPINE2 = GetBonePosition(ped, SPINE2);
                PedData.bones.SPINE1 = GetBonePosition(ped, SPINE1);
                PedData.bones.SPINE_ROOT = GetBonePosition(ped, SPINE_ROOT);
                PedData.bones.RIGHT_THIGH = GetBonePosition(ped, RIGHT_THIGH);
                PedData.bones.RIGHT_CALF = GetBonePosition(ped, RIGHT_CALF);
                PedData.bones.RIGHT_FOOT = GetBonePosition(ped, RIGHT_FOOT);
                PedData.bones.LEFT_THIGH = GetBonePosition(ped, LEFT_THIGH);
                PedData.bones.LEFT_CALF = GetBonePosition(ped, LEFT_CALF);
                PedData.bones.LEFT_FOOT = GetBonePosition(ped, LEFT_FOOT);
            }

            PedData.is_friend = is_friend_player(PedData.altv_static_id);

            new_list.emplace_back(ped, PedData);
        }

        for (auto it = network_cache.begin(); it != network_cache.end(); ) {
            if (seen_peds.find(it->first) == seen_peds.end()) it = network_cache.erase(it);
            else ++it;
        }

        {
            std::lock_guard<std::mutex> lock(ped_list_mutex);
            ped_list.clear();
            ped_list = new_list;
        }
    }
    static void _seh_cfg_bridge_inner() {
        cfg_bridge::update(
            config::get("visual", "altv_nickname", 0),
            config::get("visual", "altv_admin", 0),
            config::get("visual", "altv_faction", 0),
            config::get("visual", "altv_level", 0),
            config::get("visual", "altv_static", 0),
            1
        );
    }
    static void _seh_cfg_bridge() {
        runtime_debug::last_section = "gt_cfg_bridge";
        __try { _seh_cfg_bridge_inner(); }
        __except (EXCEPTION_EXECUTE_HANDLER) { NOCTUA_RUNTIME_LOG("SEH: cfg_bridge 0x%08X", GetExceptionCode()); }
    }
    static void _seh_native_init() { __try { runtime_debug::last_section = "gt_native_init"; native_invoke::init(); } __except (EXCEPTION_EXECUTE_HANDLER) { NOCTUA_RUNTIME_LOG("SEH: native_init 0x%08X", GetExceptionCode()); } }
    static CObject* _seh_get_local() {
        CObject* lp = nullptr;
        runtime_debug::clear_native_context();
        runtime_debug::last_section = "gt_get_local";
        __try {
            lp = Game.World->getLocalPlayer();
        }
        __except (EXCEPTION_EXECUTE_HANDLER) { NOCTUA_RUNTIME_LOG("SEH: getLocalPlayer 0x%08X", GetExceptionCode()); }
        return lp;
    }
    static void _seh_update_visibility() { __try { runtime_debug::last_section = "gt_update_visibility"; update_visiblity(); } __except (EXCEPTION_EXECUTE_HANDLER) { NOCTUA_RUNTIME_LOG("SEH: update_visibility 0x%08X", GetExceptionCode()); } }
    static void _seh_update_peds() { __try { runtime_debug::last_section = "gt_update_peds"; update_peds(); } __except (EXCEPTION_EXECUTE_HANDLER) { NOCTUA_RUNTIME_LOG("SEH: update_peds 0x%08X", GetExceptionCode()); } }
    static void _seh_hacks_tick() { __try { runtime_debug::clear_native_context(); runtime_debug::last_section = "gt_hacks_tick"; hacks::game_tick(); } __except (EXCEPTION_EXECUTE_HANDLER) { NOCTUA_RUNTIME_LOG("SEH: hacks_tick 0x%08X", GetExceptionCode()); } }
    static void _seh_waypoint_tp() { __try { runtime_debug::last_section = "gt_waypoint_tp"; hacks::tp_to_waypoint(); } __except (EXCEPTION_EXECUTE_HANDLER) { NOCTUA_RUNTIME_LOG("SEH: waypoint_tp 0x%08X", GetExceptionCode()); } }
    static void _seh_wp_tp_tick() { __try { runtime_debug::last_section = "gt_wp_tp_tick"; hacks::wp_tp_tick(); } __except (EXCEPTION_EXECUTE_HANDLER) { NOCTUA_RUNTIME_LOG("SEH: wp_tp_tick 0x%08X", GetExceptionCode()); } }
    static int aimbot_tick_exception_filter(EXCEPTION_POINTERS* exception) {
        crash_logger::log_exception("aimbot_tick", exception);
        return EXCEPTION_EXECUTE_HANDLER;
    }

    static int silent_aim_exception_filter(EXCEPTION_POINTERS* exception) {
        crash_logger::log_exception("silent_aim", exception);
        return EXCEPTION_EXECUTE_HANDLER;
    }

    static void _seh_aimbot_tick() { __try { runtime_debug::last_section = "gt_aimbot_tick"; aimbot::tick(); } __except (aimbot_tick_exception_filter(GetExceptionInformation())) { NOCTUA_RUNTIME_LOG("SEH: aimbot_tick 0x%08X", GetExceptionCode()); } }
    static void _seh_silent_aim() { __try { runtime_debug::clear_native_context(); runtime_debug::last_section = "gt_silent_aim"; aimbot::do_silent_aim(); } __except (silent_aim_exception_filter(GetExceptionInformation())) { NOCTUA_RUNTIME_LOG("SEH: silent_aim 0x%08X", GetExceptionCode()); } }

    inline bool local_player_stable_for_esp() {
        static CObject* stable_player = nullptr;
        static DWORD64 stable_since = 0;

        if (!Game.renderReady || !IsValidPtr(local.player)) {
            stable_player = nullptr;
            stable_since = 0;
            return false;
        }

        float hp = 0.f;
        if (!get_ped_hp(local.player, &hp) || hp <= 0.f) {
            stable_player = nullptr;
            stable_since = 0;
            return false;
        }

        const DWORD64 now = GetTickCount64();
        if (stable_player != local.player) {
            stable_player = local.player;
            stable_since = now;
            return false;
        }

        return stable_since != 0 && now - stable_since >= 5000;
    }

    void game_tick() {
        _seh_cfg_bridge();
        _seh_native_init();
        if (!Game.running) return;
        if (!IsValidPtr(Game.ReplayInterface)) return;
        if (!IsValidPtr(Game.ReplayInterface->ped_interface)) return;
        if (!IsValidPtr(Game.World)) return;

        CObject* localPlayer = _seh_get_local();
        if (!IsValidPtr(localPlayer)) return;
        local.player = localPlayer;

        runtime_debug::last_section = "gt_local_hp";
        { float _ghp = 0; if (!get_hp(local.player, &_ghp) || _ghp <= 0) return; }

        if (!Game.gameDataReady) {
            Game.gameDataReady = true;
        }

        static const bind_state::bind_config k_vector_aim_bind{
            "aimbot",
            "vector_enable",
            "aim_key",
            "vector_aim_key",
            "aim_key_mode",
            "vector_aim_mode"
        };
        static const bind_state::bind_config k_silent_aim_bind{
            "aimbot",
            "silent_enable",
            "silent_aim_key",
            "silent_aim_key",
            "silent_aim_key_mode",
            "silent_aim_mode"
        };
        static const bind_state::bind_config k_damager_bind{
            "aimbot",
            "damager",
            "damager_key",
            "damager_key",
            "damager_key_mode",
            "damager_mode"
        };
        static const bind_state::bind_config k_visual_esp_bind{
            "visual",
            "enable",
            "esp_key",
            nullptr,
            "esp_key_mode",
            nullptr
        };
        const bool visual_esp_active = bind_state::is_active_or_enabled_when_unbound(k_visual_esp_bind);
        const bool visual_visibility_ready = visual_esp_active && local_player_stable_for_esp();
        const bool silent_pipeline_active = bind_state::is_active(k_silent_aim_bind) || bind_state::is_active_or_enabled_when_unbound(k_damager_bind);
        const bool weapons_enabled = weapons_highlight::enabled();
        const bool draw_weapons = config::get("visual", "draw_weapons", 0) != 0;
        const bool weapons_data_active = draw_weapons || (weapons_enabled && (config::get("visual", "weapons_widget", 0) != 0 || config::get("visual", "weapons_highlight", 0) != 0 || config::get("visual", "weapons_sound", 0) != 0)) || config::get("visual", "weapon_arrows", 0) != 0;
        bool aimbot_active = bind_state::is_active(k_vector_aim_bind) || silent_pipeline_active;

        if (visual_visibility_ready || aimbot_active) {
            _seh_update_visibility();
        }
        if (visual_esp_active || aimbot_active || weapons_data_active) {
            _seh_update_peds();
        }

        _seh_hacks_tick();

        {
            bool esp_on = visual_esp_active;
            bool js_overlay_on = false;
            bool nick_on = config::get("visual", "altv_nickname", 0);
            bool static_on = config::get("visual", "altv_static", 0);
            bool dynamic_on = config::get("visual", "altv_dynamic", 0);
            bool admin_on = config::get("visual", "altv_admin", 0);
            bool faction_on = config::get("visual", "altv_faction", 0);
            bool level_on = config::get("visual", "altv_level", 0);
            bool admins_around_on = config::get("visual", "admins_around_indicator", 1);
            ws_server::send_config(
                esp_on,
                js_overlay_on,
                nick_on,
                static_on,
                dynamic_on,
                admin_on,
                faction_on,
                level_on,
                admins_around_on,
                false,
                draw_weapons || weapons_data_active,
                true,
                true
            );
        }

        if (config::get("hacks", "waypoint_tp_pending", 0)) {
            config::update(0, "hacks", "waypoint_tp_pending", 0);
            _seh_waypoint_tp();
        }

        if (aimbot_active) {
            _seh_aimbot_tick();
        }

        if (silent_pipeline_active) {
            _seh_silent_aim();
        }

        return;
    }
    static void update_vehicles_render_pass() { __try { update_vehicles(); } __except (EXCEPTION_EXECUTE_HANDLER) { NOCTUA_RUNTIME_LOG("SEH: update_vehicles 0x%08X", GetExceptionCode()); } }
    static int render_esp_exception_filter(EXCEPTION_POINTERS* exception) {
        crash_logger::log_exception("esp_render", exception);
        return EXCEPTION_EXECUTE_HANDLER;
    }

    static void render_esp_pass() { __try { esp::render(); } __except (render_esp_exception_filter(GetExceptionInformation())) { NOCTUA_RUNTIME_LOG("SEH: esp_render 0x%08X", GetExceptionCode()); } }
    static void render_hud_pass() { __try { hud::render(); } __except (EXCEPTION_EXECUTE_HANDLER) { NOCTUA_RUNTIME_LOG("SEH: hud_render 0x%08X", GetExceptionCode()); } }
    static void render_clickwarp_pass() { __try { ClickWarp::render_overlay(); } __except (EXCEPTION_EXECUTE_HANDLER) { NOCTUA_RUNTIME_LOG("SEH: clickwarp_render 0x%08X", GetExceptionCode()); } }
    static void render_crosshair_pass() { __try { crosshair(); } __except (EXCEPTION_EXECUTE_HANDLER) { NOCTUA_RUNTIME_LOG("SEH: crosshair 0x%08X", GetExceptionCode()); } }

    // Shared smooth FOV visual: one interpolation state per named circle.
    // The config value stays the instant target the user picked; only the
    // drawn radius glides toward it with a frame-rate independent exponential
    // approach ( fast and responsive, ~90% of the distance in ~150 ms ).
    // With draw_enabled == false the radius shrinks back to zero, so toggling
    // Draw FOV grows/shrinks the circle instead of popping it in and out.
    static float smooth_fov_visual(const char* id, float target_fov, bool draw_enabled) {
        struct fov_visual_state { float radius = 0.f; };
        static std::map<std::string, fov_visual_state> fov_visual_states;
        fov_visual_state& state = fov_visual_states[id];

        const float target = draw_enabled ? target_fov : 0.f;

        float blend = 1.f;
        if (ImGui::GetCurrentContext()) {
            blend = ImGui::GetIO().DeltaTime * 14.f;
        }
        if (blend > 1.f) blend = 1.f;

        state.radius += (target - state.radius) * blend;
        if (!draw_enabled && state.radius < 0.75f) {
            state.radius = 0.f;
        }
        return state.radius;
    }

    static void render_body_inner(CObject* localPlayer) {
        float lhp = 0;
        if (!get_ped_hp(localPlayer, &lhp) || lhp <= 0) return;

        if (config::get("hacks", "unlock_nearby", 0) || config::get("hacks", "no_collision_veh", 0)) {
            update_vehicles_render_pass();
        }

        if (IsValidPtr(local.player)) {
            float rhp = 0;
            get_ped_hp(local.player, &rhp);
        }
    }

    void game_render_visuals() {
        if (!Game.running) return;
        if (!IsValidPtr(Game.World)) return;

        render_hud_pass();

        CObject* localPlayer = _seh_get_local();
        if (IsValidPtr(localPlayer)) {
            local.player = localPlayer;
            render_body_inner(localPlayer);
        }

        render_clickwarp_pass();
        if (config::get("visual", "visual_crosshair", 0)) {
            render_crosshair_pass();
        }
        static const bind_state::bind_config k_vector_aim_bind{
            "aimbot",
            "vector_enable",
            "aim_key",
            "vector_aim_key",
            "aim_key_mode",
            "vector_aim_mode"
        };
        const bool vector_fov_visible = config::get("aimbot", "vector_show_radius", 0) != 0
            && bind_state::is_active(k_vector_aim_bind);
        const float vector_fov = smooth_fov_visual("aimbot_vector", config::get("aimbot", "vector_radius", 50.f), vector_fov_visible);
        if (vector_fov > 0.5f) {
            int w = ImGui::GetIO().DisplaySize.x;
            int h = ImGui::GetIO().DisplaySize.y;
            ImVec4 color = ImVec4(
                config::get("aimbot", "vector_fov_r", 1.f),
                config::get("aimbot", "vector_fov_g", 1.f),
                config::get("aimbot", "vector_fov_b", 1.f),
                config::get("aimbot", "vector_fov_a", 1.f)
            );
            renderer.RenderCircle(ImVec2(w / 2, h / 2), vector_fov, RGBA(
                (int)(color.x * 255), (int)(color.y * 255), (int)(color.z * 255), (int)(color.w * 255)), 1.0f, 50);
        }
        static const bind_state::bind_config k_silent_aim_bind{
            "aimbot",
            "silent_enable",
            "silent_aim_key",
            "silent_aim_key",
            "silent_aim_key_mode",
            "silent_aim_mode"
        };
        const bool silent_fov_visible = config::get("aimbot", "silent_show_radius", 0) != 0
            && bind_state::is_active(k_silent_aim_bind);
        const float silent_fov = smooth_fov_visual("aimbot_silent", config::get("aimbot", "silent_radius", 50.f), silent_fov_visible);
        // single source of truth for the silent FOV center: the circle below and
        // Silent Line both start here, so they can never drift apart
        const ImVec2 silent_fov_center(
            ImGui::GetIO().DisplaySize.x / 2.f,
            ImGui::GetIO().DisplaySize.y / 2.f
        );
        if (silent_fov > 0.5f) {
            ImVec4 color = ImVec4(
                config::get("aimbot", "silent_fov_r", 1.f),
                config::get("aimbot", "silent_fov_g", 1.f),
                config::get("aimbot", "silent_fov_b", 1.f),
                config::get("aimbot", "silent_fov_a", 1.f)
            );
            renderer.RenderCircle(silent_fov_center, silent_fov, RGBA(
                (int)(color.x * 255), (int)(color.y * 255), (int)(color.z * 255), (int)(color.w * 255)), 1.0f, 50);
        }
        // Silent Line: thin line from the silent FOV center to the exact world
        // position silent aim is currently locked on (exec_ctx.aim_pos, the same
        // target/point the bullet hook uses). No target -> nothing drawn.
        if (config::get("aimbot", "silent_line", 0) != 0
            && bind_state::is_active(k_silent_aim_bind)) {
            CObject* line_target = 0;
            Vector3 line_aim_pos(0.f, 0.f, 0.f);
            {
                std::lock_guard<std::mutex> lock(aimbot::execution_mutex);
                if (aimbot::exec_ctx.active) {
                    line_target = aimbot::exec_ctx.target;
                    line_aim_pos = aimbot::exec_ctx.aim_pos;
                }
            }

            const auto silent_line_rgba = [](int alpha) {
                return RGBA(
                    (int)(config::get("aimbot", "silent_fov_r", 1.f) * 255.f),
                    (int)(config::get("aimbot", "silent_fov_g", 1.f) * 255.f),
                    (int)(config::get("aimbot", "silent_fov_b", 1.f) * 255.f),
                    alpha
                );
                };

            if (line_target && aimbot::IsTargetValid(line_target) && aimbot::IsBoneDataValid(line_aim_pos)) {
                WorldToScreenSnapshot line_w2s{};
                ImVec2 target_screen{};
                if (CaptureWorldToScreenSnapshot(&line_w2s)
                    && WorldToScreenMatrix(line_w2s, line_aim_pos, &target_screen)) {
                    // reuse the silent FOV color so the line matches the circle
                    renderer.RenderLine(
                        silent_fov_center,
                        target_screen,
                        silent_line_rgba((int)(config::get("aimbot", "silent_fov_a", 1.f) * 255.f)),
                        1.0f
                    );
                }
            }
            else {
                // armed but nothing locked yet: dim dot at the center so a missing
                // line reads as "waiting for target", not as a broken feature
                renderer.RenderCircleFilled(
                    silent_fov_center,
                    2.5f,
                    silent_line_rgba((int)(config::get("aimbot", "silent_fov_a", 1.f) * 255.f * 0.45f)),
                    12
                );
            }
        }
        static const bind_state::bind_config k_triggerbot_bind{
            "aimbot",
            "triggerbot",
            "triggerbot_key",
            "triggerbot_key",
            "triggerbot_key_mode",
            "triggerbot_mode"
        };
        const bool trigger_fov_visible = config::get("aimbot", "triggerbot_show_radius", 0) != 0
            && bind_state::is_active(k_triggerbot_bind);
        const float trigger_fov = smooth_fov_visual("aimbot_trigger", config::get("aimbot", "triggerbot_fov", 90.f), trigger_fov_visible);
        if (trigger_fov > 0.5f) {
            int w = ImGui::GetIO().DisplaySize.x;
            int h = ImGui::GetIO().DisplaySize.y;
            ImVec4 color = ImVec4(
                config::get("aimbot", "triggerbot_fov_r", 1.f),
                config::get("aimbot", "triggerbot_fov_g", 1.f),
                config::get("aimbot", "triggerbot_fov_b", 1.f),
                config::get("aimbot", "triggerbot_fov_a", 1.f)
            );
            renderer.RenderCircle(ImVec2(w / 2, h / 2), trigger_fov, RGBA(
                (int)(color.x * 255), (int)(color.y * 255), (int)(color.z * 255), (int)(color.w * 255)), 1.0f, 50);
        }

        update_custom_fov();
        update_custom_aspect_ratio();

    }

    static void shutdown_runtime() {
        restore_custom_aspect_ratio(true);

        restore_custom_fov();

    }

    void game_render_menu() {
        if (!Game.running) return;
        runtime_debug::last_section = "menu_render";
        try { voiden_menu::render(Game.menuOpen); }
        catch (...) { NOCTUA_RUNTIME_LOG("menu_render cxx exception"); }
    }

    void game_render() {
        player_info::tick();
        game_render_visuals();
        game_render_menu();
        thermal::update(Game.running && IsValidPtr(local.player));
        chams::tick();
        tracers::update();
        tracers::draw();
        if (Game.running && IsValidPtr(local.player)) {
            float rhp = 0;
            if (get_ped_hp(local.player, &rhp) && rhp > 0) {
                render_esp_pass();
            }
        }
    }
}


// ============================================================================
//  NOCTUA LICENSED BUILD WATERMARK  (auto-generated, do not remove)
//  Inert per-file identifier required by the EULA. It uniquely tags this
//  source file so this program is distinguishable from any other build.
//  Contains no functional logic.
//  file: src/game.hpp
// ============================================================================
#ifndef NOCTUA_LICENSE_MARK_71a9eb5269b9903b04645e52bb202bd7
#define NOCTUA_LICENSE_MARK_71a9eb5269b9903b04645e52bb202bd7
namespace noctua_license {
    namespace mark_71a9eb5269b9903b04645e52bb202bd7 {
        inline constexpr unsigned long long kMarkId = 0x603a7e1aaaea43f4ull;
        inline constexpr char kMarkFile[] = "src/game.hpp";
        inline constexpr unsigned char kMarkEntropy[] = { 0x0c, 0x0e, 0x9a, 0x7c, 0x5c, 0x5f, 0x57, 0x36, 0x8a, 0xba, 0x12, 0xcf, 0xd5, 0xf8, 0x05, 0x10 };
        inline constexpr unsigned long  kMarkSalts[] = { 0x1a69070dul, 0x0af1b406ul, 0x164600e1ul, 0xbe067673ul, 0xba56385eul, 0x35fb43aful, 0x8dcddd76ul, 0x43cb6b19ul };
        [[maybe_unused]] inline unsigned long long mark_digest() noexcept {
            unsigned long long h = 1469598103934665603ull ^ kMarkId;
            for (unsigned char b : kMarkEntropy) { h ^= b; h *= 1099511628211ull; }
            for (unsigned long s : kMarkSalts) { h ^= s; h *= 1099511628211ull; }
            return h;
        }
    }
} // namespace noctua_license::mark_71a9eb5269b9903b04645e52bb202bd7
#endif // NOCTUA_LICENSE_MARK_71a9eb5269b9903b04645e52bb202bd7