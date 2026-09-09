#pragma once

#include "core/imports.h"
#include "config/interface.hpp"
#include "render/renderer.h"
#include <string>
#include <vector>

extern imgui_render renderer;

// pointer_to_entity_handle is defined later in game.hpp (namespace game);
// forward-declared here so the draw path can convert a CObject* to the
// entity handle the player natives expect.
namespace game {
    int pointer_to_entity_handle(uintptr_t ptr);
}

// Alternative ESP - reads peds straight from the game's ped pool (memory),
// completely independent from the JS/websocket bridge. Proof-of-concept that
// direct C++ memory reading works: box + name + health bar for every ped,
// plus a status line with the total counts. Player names come from the
// GET_PLAYER_NAME native (matched by ped handle) - the CPlayerInfo::sName
// field is uninitialized garbage on alt:V.
namespace alt_esp {
    struct player_entry {
        int handle = 0;
        std::string name;
    };

    inline std::vector<player_entry> s_players;
    inline bool s_enabled = false;

    inline void tick() {
        s_enabled = config::get("visual", "alt_esp", 0) != 0;
    }

    // sName on RU RP servers is often stored in Windows-1251; ImGui expects
    // UTF-8 and renders every invalid byte as '?'. Bytes that already form
    // valid UTF-8 are passed through unchanged.
    static std::string name_to_utf8(const char* s, size_t n) {
        std::string out;
        out.reserve(n * 2 + 1);
        size_t i = 0;
        while (i < n && s[i]) {
            const unsigned char c = (unsigned char)s[i];
            if (c < 0x80) { out.push_back((char)c); ++i; continue; }
            // possible UTF-8 lead byte - keep if continuation bytes are valid
            if (c >= 0xC2 && c <= 0xF4) {
                const size_t len = (c >= 0xF0) ? 4 : (c >= 0xE0) ? 3 : 2;
                bool ok = i + len <= n;
                for (size_t k = 1; ok && k < len; ++k) ok = ((unsigned char)s[i + k] & 0xC0) == 0x80;
                if (ok) { out.append(s + i, len); i += len; continue; }
            }
            // CP1251 Cyrillic (C0-FF -> U+0410-U+044F) and punctuation
            if (c >= 0xC0 && c <= 0xFF) {
                const unsigned u = 0x410 + (c - 0xC0);
                out.push_back((char)(0xD0 + ((u >> 6) - 0x10)));
                out.push_back((char)(0x80 + (u & 0x3F)));
            }
            else if (c == 0xA8) { out.push_back((char)0xD0); out.push_back((char)0x81); }  // Ё
            else if (c == 0xB8) { out.push_back((char)0xD1); out.push_back((char)0x91); }  // ё
            else if (c >= 0x80) { out.push_back((char)0xC2); out.push_back((char)(c - 0x40)); }
            else { out.push_back('?'); }
            ++i;
        }
        return out;
    }

    // once per frame: map active players (by ped handle) to their names
    static void collect_players() {
        s_players.clear();
        for (int i = 0; i < 32; ++i) {
            if (!native::player::network_is_player_active(i)) continue;
            const int ped_handle = native::player::get_player_ped(i);
            if (ped_handle <= 0) continue;
            const char* nm = native::player::get_player_name(i);
            s_players.push_back({ ped_handle, nm ? nm : "" });
        }
    }

    // per-ped drawing - NO __try here (ImVec2/RGBA have user ctors, which the
    // SEH-compiled function must not own); read faults from this function are
    // caught by the caller's __except
    static void draw_ped(ImDrawList* dl, CObject* ped, int index) {
        const Vector3& pos = ped->fPosition;
        if (pos.x == 0.f && pos.y == 0.f && pos.z == 0.f) return;

        ImVec2 top2d, bottom2d;
        if (!WorldToScreen2(Vector3(pos.x, pos.y, pos.z + 0.9f), &top2d)) return;
        if (!WorldToScreen2(Vector3(pos.x, pos.y, pos.z - 1.0f), &bottom2d)) return;

        float box_h = fabsf(top2d.y - bottom2d.y);
        if (box_h < 12.f) box_h = 12.f;
        const float box_w = box_h / 2.f;
        const ImVec2 p0(top2d.x - box_w * 0.5f, top2d.y);
        const ImVec2 p1(top2d.x + box_w * 0.5f, top2d.y + box_h);

        CPlayerInfo* pinfo = ped->player_info();
        const bool is_player = IsValidPtr(pinfo);
        const RGBA col = is_player ? RGBA(0, 230, 230, 255) : RGBA(255, 165, 0, 220);

        renderer.RenderRect(p0, p1, col, 0.f, ImDrawFlags_RoundCornersAll, 1.5f);

        // name from the GET_PLAYER_NAME native, matched by ped handle
        std::string label = "ALT player";
        if (is_player) {
            const int handle = game::pointer_to_entity_handle(reinterpret_cast<uintptr_t>(ped));
            if (handle > 0) {
                for (const auto& p : s_players) {
                    if (p.handle == handle) {
                        label = "ALT " + name_to_utf8(p.name.c_str(), p.name.size());
                        break;
                    }
                }
            }
        }
        else {
            char ped_label[32];
            sprintf_s(ped_label, "ALT ped#%d", index);
            label = ped_label;
        }

        // same font as the main ESP name (loaded with cyrillic ranges) -
        // RenderText's espFont fallback chain may end on a latin-only font
        ImFont* name_font = renderer.EspNameFont(12.f, nullptr);
        if (name_font) {
            const ImVec2 ts = name_font->CalcTextSizeA(12.f, FLT_MAX, 0.f, label.c_str());
            const ImVec2 text_pos(top2d.x - ts.x * 0.5f, top2d.y - ts.y - 3.f);
            const ImU32 text_col = IM_COL32(col.r, col.g, col.b, col.a);
            dl->AddText(name_font, 12.f, ImVec2(text_pos.x + 1.f, text_pos.y + 1.f), IM_COL32(0, 0, 0, (int)(col.a * 0.7f)), label.c_str());
            dl->AddText(name_font, 12.f, text_pos, text_col, label.c_str());
        }
        else {
            renderer.RenderText(label.c_str(), ImVec2(top2d.x, top2d.y - 14.f), 12.f, col, true, true);
        }

        float hp_ratio = ped->MaxHP > 0.f ? ped->HP / ped->MaxHP : 0.f;
        hp_ratio = hp_ratio > 1.f ? 1.f : (hp_ratio < 0.f ? 0.f : hp_ratio);
        const ImVec2 bar0(p0.x, p1.y + 2.f);
        const ImVec2 bar1(p0.x + box_w, bar0.y + 3.f);
        renderer.RenderRectFilled(bar0, bar1, RGBA(40, 40, 40, 200), 0.f, ImDrawFlags_None);
        renderer.RenderRectFilled(bar0, ImVec2(bar0.x + box_w * hp_ratio, bar1.y), RGBA(0, 220, 0, 220), 0.f, ImDrawFlags_None);
    }

    // pool enumeration under SEH - keeps only POD locals so /EHsc does not
    // demand unwinding inside __try
    static void draw_pool() {
        int found = 0;
        int players = 0;
        auto* dl = ImGui::GetBackgroundDrawList();

        const int max_peds = Game.ReplayInterface->ped_interface->max_peds;
        if (max_peds <= 0 || max_peds > 1024) return;

        collect_players();

        for (int i = 0; i < max_peds; ++i) {
            CObject* ped = Game.ReplayInterface->ped_interface->get_ped(i);
            if (!IsValidPtr(ped)) continue;
            if (ped->HP <= 0.f) continue;

            if (IsValidPtr(ped->player_info())) ++players;
            draw_ped(dl, ped, i);
            ++found;
        }

        char status[96];
        sprintf_s(status, "ALT ESP: %d peds (%d players)", found, players);
        renderer.RenderText(status, ImVec2(12.f, 12.f), 14.f, RGBA(0, 230, 230, 255), false, true);
    }

    inline void draw() {
        if (!s_enabled) return;
        if (!IsValidPtr(Game.ReplayInterface) || !IsValidPtr(Game.ReplayInterface->ped_interface)) return;
        if (!IsValidPtr(local.player)) return;
        if (!ImGui::GetBackgroundDrawList()) return;

        __try {
            draw_pool();
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            // memory reads are guarded; on failure just skip the frame
        }
    }
}
