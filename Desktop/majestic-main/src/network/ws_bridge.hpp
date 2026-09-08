#pragma once

#include <winsock2.h>
#include <ws2tcpip.h>
#include <vector>
#include <mutex>
#include <string>
#include <memory>
#include <set>
#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <atomic>
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <map>
#include <regex>
#include "IXWebSocketServer.h"
#include "network/bridge/parser.hpp"
#include "network/nick_cache.h"
#include "platform/build_profile.hpp"
#include "platform/noctua_paths.hpp"
#include "platform/debug.hpp"
#include "runtime/session.hpp"
#include "platform/vmp.hpp"
#include "json.hpp"

#pragma comment(lib, "ws2_32.lib")

using json = nlohmann::json;

namespace ws_server {

    struct EspPlayer {
        int handle;
        std::string name;
        std::string login;
        std::string gender;
        int static_id;
        int level;
        int admin_level;
        std::string fraction;
        int fraction_id;
        int family_id;
        int leader_id;
        float hp;
        float armor;
        uint32_t weapon_hash;
        bool is_admin;
        bool is_dead;
        bool is_media;
        bool is_tester;
        bool is_afk;
        uint16_t netid;
        float pos_x, pos_y, pos_z;
        uint64_t sequence;
        uint8_t bone_count;
        uint8_t auto_relation;
        float bones[24][3];
    };

    inline std::mutex g_mutex;
    inline std::vector<EspPlayer> g_players;
    inline std::unique_ptr<ix::WebSocketServer> g_server;
    inline bool g_running = false;

    inline std::mutex g_clients_mutex;
    inline std::set<std::shared_ptr<ix::WebSocket>> g_clients;
    inline std::set<std::shared_ptr<ix::WebSocket>> g_authenticated_clients;

    inline std::string g_last_config_json;
    inline std::string g_log_server_id;
    inline std::string g_payload_server_id;
    inline std::string g_resolved_server_id;
    inline uint64_t g_sequence = 0;
    inline std::atomic<bool> g_freecam_stop_requested{false};

    struct MenuItem {
        std::string script;
        std::string id;
        std::string group;
        std::string kind;
        std::string label;
        std::string tooltip;
        std::string parent;
        std::vector<std::string> path;
        json value;
        double min = 0.0;
        double max = 0.0;
        std::vector<std::string> options;
        std::vector<std::string> option_ids;
        bool visible = true;
        bool disabled = false;
        uint64_t order = 0;
    };

    struct MenuUpdate {
        std::string id;
        json patch;
    };

    inline std::mutex g_menu_mutex;
    inline std::map<std::string, MenuItem> g_menu_items;
    inline std::map<std::string, MenuItem> g_builtin_menu_items;
    inline std::vector<MenuUpdate> g_builtin_menu_updates;
    inline std::atomic<uint64_t> g_next_menu_item_order{0};
    inline std::atomic<bool> g_builtin_menu_dirty{false};
    inline std::mutex g_user_scripts_mutex;
    inline std::map<std::string, std::string> g_user_scripts;

    struct UserScriptInfo {
        std::string name;
        std::string cloud_id;
    };

    inline bool is_user_script_loaded(const std::string& name) {
        std::lock_guard<std::mutex> lk(g_user_scripts_mutex);
        return g_user_scripts.find(name) != g_user_scripts.end();
    }

    inline std::vector<UserScriptInfo> user_scripts_snapshot() {
        std::vector<UserScriptInfo> scripts;
        std::lock_guard<std::mutex> lk(g_user_scripts_mutex);
        scripts.reserve(g_user_scripts.size());
        for (const auto& [name, cloud_id] : g_user_scripts) {
            scripts.push_back({ name, cloud_id });
        }
        return scripts;
    }

    inline void send_to_all(const json& j);

    inline bool consume_freecam_stop_requested() {
        return g_freecam_stop_requested.exchange(false);
    }

    inline std::string normalize_server_text(std::string value) {
        while (!value.empty() && std::isspace(static_cast<unsigned char>(value.front()))) value.erase(value.begin());
        while (!value.empty() && std::isspace(static_cast<unsigned char>(value.back()))) value.pop_back();
        std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
            if (ch == '-' || ch == '_') return ' ';
            return static_cast<char>(std::tolower(ch));
        });

        std::string collapsed;
        bool last_space = false;
        for (const char ch : value) {
            const bool is_space = std::isspace(static_cast<unsigned char>(ch)) != 0;
            if (is_space) {
                if (!last_space && !collapsed.empty()) collapsed.push_back(' ');
            } else {
                collapsed.push_back(ch);
            }
            last_space = is_space;
        }
        if (!collapsed.empty() && collapsed.back() == ' ') collapsed.pop_back();
        return collapsed;
    }

    struct ServerAlias {
        const char* code;
        const char* id;
    };

    inline constexpr ServerAlias k_server_aliases[] = {
        { "RU1", "new york" },
        { "RU2", "detroit" },
        { "RU3", "chicago" },
        { "RU4", "san francisco" },
        { "RU5", "atlanta" },
        { "RU6", "san diego" },
        { "RU7", "los angeles" },
        { "RU8", "miami" },
        { "RU9", "las vegas" },
        { "RU10", "washington" },
        { "RU11", "dallas" },
        { "RU12", "boston" },
        { "RU13", "houston" },
        { "RU14", "seattle" },
        { "RU15", "phoenix" },
        { "RU16", "denver" },
        { "RU17", "portland" },
        { "RU18", "orlando" },
        { "RU19", "memphis" },
    };

    inline std::string canonical_server_id_from_code(std::string code) {
        code.erase(std::remove_if(code.begin(), code.end(), [](unsigned char ch) {
            return std::isspace(ch) != 0;
        }), code.end());
        std::transform(code.begin(), code.end(), code.begin(), [](unsigned char ch) {
            return static_cast<char>(std::toupper(ch));
        });

        for (const auto& server : k_server_aliases) {
            if (std::strcmp(code.c_str(), server.code) == 0) return server.id;
        }
        return {};
    }

    inline std::string canonical_server_id_from_text(const std::string& value) {
        const std::string normalized = normalize_server_text(value);
        if (normalized.empty()) return {};

        std::string code_candidate = normalized;
        code_candidate.erase(std::remove_if(code_candidate.begin(), code_candidate.end(), [](unsigned char ch) {
            return std::isspace(ch) != 0;
        }), code_candidate.end());
        const std::string from_code = canonical_server_id_from_code(code_candidate);
        if (!from_code.empty()) return from_code;

        for (const auto& server : k_server_aliases) {
            if (normalized == server.id) return normalized;
        }
        return {};
    }

    inline void refresh_resolved_server_id_locked() {
        const std::string resolved = !g_log_server_id.empty() ? g_log_server_id : g_payload_server_id;
        if (resolved == g_resolved_server_id) return;
        g_resolved_server_id = resolved;
        player_marks::set_server_id(g_resolved_server_id);
    }

    inline void set_log_server_id(const std::string& server_id) {
        const std::string canonical = canonical_server_id_from_text(server_id);
        if (canonical.empty()) return;

        std::lock_guard<std::mutex> lk(g_mutex);
        g_log_server_id = canonical;
        refresh_resolved_server_id_locked();
    }

    inline void set_payload_server_id(const json& source) {
        std::string canonical;
        if (source.value("recognized", false)) {
            canonical = canonical_server_id_from_text(source.value("server", std::string()));
            if (canonical.empty()) canonical = canonical_server_id_from_code(source.value("code", std::string()));
        }

        std::lock_guard<std::mutex> lk(g_mutex);
        g_payload_server_id = canonical;
        refresh_resolved_server_id_locked();
    }

    inline std::vector<std::filesystem::path> client_log_dirs() {
        std::vector<std::filesystem::path> dirs;
        if (const char* appdata = std::getenv("APPDATA")) {
            dirs.emplace_back(std::filesystem::path(appdata) / "majestic-launcher" / "Multiplayer" / "logs");
        }
        if (const char* userprofile = std::getenv("USERPROFILE")) {
            dirs.emplace_back(std::filesystem::path(userprofile) / "AppData" / "Roaming" / "majestic-launcher" / "Multiplayer" / "logs");
        }
        dirs.emplace_back(noctua_paths::local_root());
        return dirs;
    }

    inline std::string menu_key(const std::string& script, const std::string& id) {
        return script + "\x1f" + id;
    }

    inline MenuItem parse_menu_item(const json& source, const std::string& fallback_script) {
        MenuItem item;
        item.script = source.value("script", fallback_script);
        item.id = source.value("id", std::string());
        item.group = source.value("group", std::string());
        item.kind = source.value("kind", std::string());
        item.label = source.value("label", item.id);
        item.tooltip = source.value("tooltip", std::string());
        item.parent = source.value("parent", std::string());
        item.value = source.contains("value") ? source["value"] : json();
        item.min = source.value("min", 0.0);
        item.max = source.value("max", 0.0);
        item.visible = source.value("visible", true);
        item.disabled = source.value("disabled", false);
        if (source.contains("path") && source["path"].is_array()) {
            for (const auto& part : source["path"]) {
                if (part.is_string()) item.path.push_back(part.get<std::string>());
            }
        }
        if (source.contains("options") && source["options"].is_array()) {
            for (const auto& option : source["options"]) {
                if (option.is_string()) {
                    const std::string value = option.get<std::string>();
                    item.option_ids.push_back(value);
                    item.options.push_back(value);
                } else if (option.is_object()) {
                    const std::string id = option.value("id", std::string());
                    const std::string label = option.value("label", id);
                    if (!id.empty()) {
                        item.option_ids.push_back(id);
                        item.options.push_back(label.empty() ? id : label);
                    }
                }
            }
        }
        return item;
    }

    inline json menu_item_to_json(const MenuItem& item) {
        json out;
        out["script"] = item.script;
        out["id"] = item.id;
        out["group"] = item.group;
        out["kind"] = item.kind;
        out["label"] = item.label;
        out["tooltip"] = item.tooltip;
        out["parent"] = item.parent;
        out["path"] = item.path;
        out["value"] = item.value;
        out["min"] = item.min;
        out["max"] = item.max;
        out["visible"] = item.visible;
        out["disabled"] = item.disabled;
        out["options"] = json::array();
        for (size_t i = 0; i < item.options.size(); ++i) {
            json option;
            option["id"] = i < item.option_ids.size() ? item.option_ids[i] : item.options[i];
            option["label"] = item.options[i];
            out["options"].push_back(option);
        }
        return out;
    }

    inline void process_menu_register(const json& j) {
        const std::string script = j.value("script", std::string());
        const json& source = j.contains("item") && j["item"].is_object() ? j["item"] : j;
        MenuItem item = parse_menu_item(source, script);
        if (item.script.empty() || item.id.empty() || item.kind.empty()) return;
        item.order = ++g_next_menu_item_order;
        std::lock_guard<std::mutex> lock(g_menu_mutex);
        g_menu_items[menu_key(item.script, item.id)] = std::move(item);
    }

    inline void process_menu_update(const json& j) {
        const std::string script = j.value("script", std::string());
        const std::string id = j.value("id", std::string());
        if (script.empty() || id.empty()) return;
        const json& patch = j.contains("patch") && j["patch"].is_object() ? j["patch"] : j;
        if (script == "__builtin") {
            std::lock_guard<std::mutex> lock(g_menu_mutex);
            g_builtin_menu_updates.push_back({ id, patch });
            auto it = g_builtin_menu_items.find(id);
            if (it != g_builtin_menu_items.end()) {
                if (patch.contains("value")) it->second.value = patch["value"];
                if (patch.contains("visible")) it->second.visible = patch.value("visible", it->second.visible);
                if (patch.contains("disabled")) it->second.disabled = patch.value("disabled", it->second.disabled);
                if (patch.contains("tooltip")) it->second.tooltip = patch.value("tooltip", it->second.tooltip);
            }
            g_builtin_menu_dirty.store(true);
            return;
        }
        std::lock_guard<std::mutex> lock(g_menu_mutex);
        auto it = g_menu_items.find(menu_key(script, id));
        if (it == g_menu_items.end()) return;
        if (patch.contains("value")) it->second.value = patch["value"];
        if (patch.contains("visible")) it->second.visible = patch.value("visible", it->second.visible);
        if (patch.contains("disabled")) it->second.disabled = patch.value("disabled", it->second.disabled);
        if (patch.contains("label")) it->second.label = patch.value("label", it->second.label);
        if (patch.contains("tooltip")) it->second.tooltip = patch.value("tooltip", it->second.tooltip);
    }

    inline void process_menu_remove(const json& j) {
        const std::string script = j.value("script", std::string());
        const std::string id = j.value("id", std::string());
        if (script.empty()) return;
        std::lock_guard<std::mutex> lock(g_menu_mutex);
        if (!id.empty()) {
            g_menu_items.erase(menu_key(script, id));
            return;
        }
        for (auto it = g_menu_items.begin(); it != g_menu_items.end();) {
            if (it->second.script == script) it = g_menu_items.erase(it);
            else ++it;
        }
    }

    inline std::string read_server_id_from_client_log() {
        try {
            std::filesystem::path newest_path;
            std::filesystem::file_time_type newest_time{};
            const auto log_dirs = client_log_dirs();
            for (const auto& log_dir : log_dirs) {
                if (!std::filesystem::exists(log_dir)) continue;
                for (const auto& entry : std::filesystem::directory_iterator(log_dir)) {
                if (!entry.is_regular_file()) continue;
                    const auto filename = entry.path().filename().string();
                    if (entry.path().extension() != ".log") continue;
                    // accept launcher client logs (client_*.log) and main.log (majestic launcher)
                    if (!(filename.rfind("client_", 0) == 0 || filename == "main.log" || filename.rfind("main_", 0) == 0)) continue;
                    const auto write_time = entry.last_write_time();
                    if (newest_path.empty() || write_time > newest_time) {
                        newest_path = entry.path();
                        newest_time = write_time;
                    }
                }
            }
            if (newest_path.empty()) return {};
            std::ifstream file(newest_path, std::ios::binary);
            if (!file.is_open()) return {};

            // Determine file size
            file.seekg(0, std::ios::end);
            std::streamsize file_size = file.tellg();
            file.seekg(0, std::ios::beg);

            std::string data;
            const std::streamsize read_size = (file_size > 262144) ? 262144 : file_size; // 256KB max
            data.resize(static_cast<size_t>(read_size));

            // If file is large, read from the beginning (where Connect name usually is)
            file.read(data.data(), read_size);
            data.resize(static_cast<size_t>(file.gcount()));

            std::regex name_pattern(R"(Connect name:\s*([^\r\n]+))");
            std::smatch match;
            if (std::regex_search(data, match, name_pattern)) {
                std::string server_name = match[1].str();
                std::string result = canonical_server_id_from_text(server_name);
                if (!result.empty()) return result;
            }

            // look for launcher payloads like: serverId: 'ru6' or serverName: 'San Diego'
            std::regex server_id_pattern(R"(serverId:\s*'([^']+)'|serverId:\s*\"([^\"]+)\")");
            if (std::regex_search(data, match, server_id_pattern)) {
                std::string server_code = match[1].matched ? match[1].str() : match[2].str();
                return canonical_server_id_from_code(server_code);
            }
            std::regex server_name_pattern(R"(serverName:\s*'([^']+)'|serverName:\s*\"([^\"]+)\")");
            if (std::regex_search(data, match, server_name_pattern)) {
                std::string server_name = match[1].matched ? match[1].str() : match[2].str();
                return canonical_server_id_from_text(server_name);
            }
            return {};
        } catch (...) {
            return {};
        }
    }

    inline std::string get_server_id() {
        {
            std::lock_guard<std::mutex> lk(g_mutex);
            if (!g_log_server_id.empty()) return g_resolved_server_id;
        }
        const std::string log_server_id = read_server_id_from_client_log();
        if (!log_server_id.empty()) set_log_server_id(log_server_id);

        std::lock_guard<std::mutex> lk(g_mutex);
        return g_resolved_server_id;
    }

    inline bool has_resolved_server_id() {
        std::lock_guard<std::mutex> lk(g_mutex);
        return !g_resolved_server_id.empty();
    }

    inline void rescan_server_id() {
        const std::string log_server_id = read_server_id_from_client_log();
        if (!log_server_id.empty()) {
            set_log_server_id(log_server_id);
        }
    }

    inline std::string fraction_name(int id) {
        switch (id) {
        case 1: return "LSPD";
        case 2: return "EMS";
        case 3: return "Sheriff";
        case 4: return "SANG";
        case 5: return "GOV";
        case 6: return "WN";
        case 7: return "FIB";
        case 8: return "Ballas";
        case 9: return "Vagos";
        case 10: return "Families";
        case 11: return "Bloods";
        case 12: return "Marabunta";
        default: return "None";
        }
    }

    inline bool same_player(const EspPlayer& a, const EspPlayer& b) {
        if (a.netid > 0 && b.netid > 0) return a.netid == b.netid;
        if (a.handle > 0 && b.handle > 0) return a.handle == b.handle;
        if (a.static_id > 0 && b.static_id > 0) return a.static_id == b.static_id;
        return false;
    }

    inline void commit_players(std::vector<EspPlayer>&& players) {
        {
            std::lock_guard<std::mutex> lk(g_mutex);
            ++g_sequence;
            for (auto& player : players) {
                player.sequence = g_sequence;
            }
            g_players = std::move(players);
        }

        std::lock_guard<std::mutex> lk(g_nick_cache_mutex);
        g_nick_cache.clear();
        g_nick_cache_by_altv_id.clear();
        g_nick_entries.clear();
        for (auto& player : g_players) {
            if (player.handle > 0) g_nick_cache[player.handle] = player.name;
            if (player.netid > 0) g_nick_cache_by_altv_id[player.netid] = player.name;
            g_nick_entries.push_back({ player.name, player.netid, static_cast<uint32_t>(player.handle) });
        }
    }

    inline bool process_binary_data(const std::string& data) {
        if (data.size() < 3) return false;
        const uint8_t packet_type = static_cast<uint8_t>(data[0]);
        if (packet_type != 0x01 && packet_type != 0x02 && packet_type != 0x03 && packet_type != 0x04 && packet_type != 0x05) return false;
        const bool has_afk = packet_type >= 0x02;
        const bool has_auto_relation = packet_type >= 0x03;
        const bool has_family_id = packet_type >= 0x04;
        const bool has_radar_fields = packet_type >= 0x05;
        size_t offset = 1;
        uint16_t count = 0;
        if (!bridge_parser::read_u16(data, offset, count)) return true;
        if (count > 512) return true;

        std::vector<EspPlayer> players;
        players.reserve(count);
        std::vector<std::pair<int, player_marks::relation>> auto_marks;
        auto_marks.reserve(count);

        for (uint16_t i = 0; i < count; ++i) {
            uint16_t remote_id = 0;
            uint32_t static_id = 0;
            uint8_t name_len = 0;
            uint8_t login_len = 0;
            uint8_t gender_len = 0;
            int8_t fraction_id = 0;
            uint16_t health = 0;
            uint8_t armor = 0;
            uint8_t admin = 0;
            uint8_t media = 0;
            uint8_t afk = 0;
            uint8_t level = 0;
            uint8_t dead = 0;
            float x = 0.f, y = 0.f, z = 0.f;
            float rotation = 0.f;
            uint32_t weapon = 0;
            uint8_t bones = 0;
            uint8_t auto_relation = 0;
            uint32_t family_id = 0;
            uint32_t leader_id = 0;
            uint8_t tester = 0;
            float bone_data[24][3]{};
            uint8_t stored_bones = 0;

            if (!bridge_parser::read_u16(data, offset, remote_id)) return true;
            if (!bridge_parser::read_u32(data, offset, static_id)) return true;
            if (!bridge_parser::read_u8(data, offset, name_len)) return true;
            if (offset + name_len > data.size()) return true;
            std::string name(data.data() + offset, data.data() + offset + name_len);
            offset += name_len;
            std::string login;
            std::string gender;
            if (has_radar_fields) {
                if (!bridge_parser::read_u8(data, offset, login_len)) return true;
                if (offset + login_len > data.size()) return true;
                login.assign(data.data() + offset, data.data() + offset + login_len);
                offset += login_len;
                if (!bridge_parser::read_u8(data, offset, gender_len)) return true;
                if (offset + gender_len > data.size()) return true;
                gender.assign(data.data() + offset, data.data() + offset + gender_len);
                offset += gender_len;
            }
            if (!bridge_parser::read_i8(data, offset, fraction_id)) return true;
            if (!bridge_parser::read_u16(data, offset, health)) return true;
            if (!bridge_parser::read_u8(data, offset, armor)) return true;
            if (!bridge_parser::read_u8(data, offset, admin)) return true;
            if (!bridge_parser::read_u8(data, offset, media)) return true;
            if (has_afk && !bridge_parser::read_u8(data, offset, afk)) return true;
            if (!bridge_parser::read_u8(data, offset, level)) return true;
            if (!bridge_parser::read_u8(data, offset, dead)) return true;
            if (has_auto_relation && !bridge_parser::read_u8(data, offset, auto_relation)) return true;
            if (has_family_id && !bridge_parser::read_u32(data, offset, family_id)) return true;
            if (has_radar_fields && !bridge_parser::read_u32(data, offset, leader_id)) return true;
            if (has_radar_fields && !bridge_parser::read_u8(data, offset, tester)) return true;
            if (!bridge_parser::read_f32(data, offset, x)) return true;
            if (!bridge_parser::read_f32(data, offset, y)) return true;
            if (!bridge_parser::read_f32(data, offset, z)) return true;
            if (!bridge_parser::read_f32(data, offset, rotation)) return true;
            if (!bridge_parser::read_u32(data, offset, weapon)) return true;
            if (!bridge_parser::read_u8(data, offset, bones)) return true;

            for (uint8_t b = 0; b < bones; ++b) {
                float bx = 0.f, by = 0.f, bz = 0.f;
                if (!bridge_parser::read_f32(data, offset, bx)) return true;
                if (!bridge_parser::read_f32(data, offset, by)) return true;
                if (!bridge_parser::read_f32(data, offset, bz)) return true;
                if (b < 24) {
                    bone_data[b][0] = bx;
                    bone_data[b][1] = by;
                    bone_data[b][2] = bz;
                    stored_bones = b + 1;
                }
            }

            EspPlayer player{};
            player.handle = remote_id;
            player.netid = remote_id;
            // prefer login over name for display (playerlist/esp) when login available
            player.name = login.empty() ? (name.empty() ? std::string("Unknown") : name) : login;
            player.login = login;
            player.gender = gender;
            player.static_id = static_cast<int>(static_id);
            player.level = level;
            player.admin_level = static_cast<int>(admin);
            player.fraction = fraction_name(fraction_id);
            player.fraction_id = static_cast<int>(fraction_id);
            player.family_id = static_cast<int>(family_id);
            player.leader_id = static_cast<int>(leader_id);
            player.hp = static_cast<float>(health);
            player.armor = static_cast<float>(armor);
            player.weapon_hash = weapon;
            player.is_admin = admin != 0;
            player.is_dead = dead != 0;
            player.is_media = media != 0;
            player.is_tester = tester != 0;
            player.is_afk = afk != 0;
            player.auto_relation = auto_relation;
            player.pos_x = x;
            player.pos_y = y;
            player.pos_z = z;
            player.bone_count = stored_bones;
            for (uint8_t b = 0; b < stored_bones; ++b) {
                player.bones[b][0] = bone_data[b][0];
                player.bones[b][1] = bone_data[b][1];
                player.bones[b][2] = bone_data[b][2];
            }
            NOCTUA_RUNTIME_LOG("ESP_INCOMING: handle=%u name='%s' login='%s' static=%d fraction_id=%d level=%d gender='%s' pos=(%f,%f,%f) netid=%u",
                static_cast<unsigned int>(player.handle), player.name.c_str(), player.login.c_str(), player.static_id, player.fraction_id, player.level, player.gender.c_str(), player.pos_x, player.pos_y, player.pos_z, static_cast<unsigned int>(player.netid));

            // Также логируем полную структуру игрока в виде JSON, чтобы в noctua.log была вся полученная информация
            try {
                json esp_full;
                esp_full["handle"] = player.handle;
                esp_full["name"] = player.name;
                esp_full["login"] = player.login;
                esp_full["gender"] = player.gender;
                esp_full["static"] = player.static_id;
                esp_full["fraction"] = player.fraction;
                esp_full["fraction_id"] = player.fraction_id;
                esp_full["family_id"] = player.family_id;
                esp_full["leader_id"] = player.leader_id;
                esp_full["level"] = player.level;
                esp_full["admin_level"] = player.admin_level;
                esp_full["is_admin"] = player.is_admin;
                esp_full["is_dead"] = player.is_dead;
                esp_full["is_media"] = player.is_media;
                esp_full["is_tester"] = player.is_tester;
                esp_full["is_afk"] = player.is_afk;
                esp_full["hp"] = player.hp;
                esp_full["armor"] = player.armor;
                esp_full["weapon_hash"] = player.weapon_hash;
                esp_full["netid"] = player.netid;
                esp_full["pos"] = { {"x", player.pos_x}, {"y", player.pos_y}, {"z", player.pos_z} };
                esp_full["auto_relation"] = player.auto_relation;
                esp_full["bone_count"] = player.bone_count;
                // bones (if any)
                if (player.bone_count > 0) {
                    json bones = json::array();
                    for (uint8_t b = 0; b < player.bone_count && b < 24; ++b) {
                        bones.push_back({ player.bones[b][0], player.bones[b][1], player.bones[b][2] });
                    }
                    esp_full["bones"] = bones;
                }
                const std::string dump = esp_full.dump();
                NOCTUA_RUNTIME_LOG("ESP_INCOMING_FULL: %s", dump.c_str());
            } catch (...) {}

            if (static_id > 0) {
                if (auto_relation == 3) auto_marks.emplace_back(static_cast<int>(static_id), player_marks::relation::family_player);
                else if (auto_relation == 4) auto_marks.emplace_back(static_cast<int>(static_id), player_marks::relation::fraction_player);
            }
            players.push_back(std::move(player));
        }

        if (has_auto_relation) player_marks::replace_local(auto_marks);
        commit_players(std::move(players));
        return true;
    }

    inline std::atomic<int> g_export_files_received{0};
    inline std::atomic<int> g_export_files_total{0};
    inline std::atomic<bool> g_export_in_progress{false};
    inline std::string g_export_status;
    inline std::mutex g_export_mutex;

    inline std::string base64_decode(const std::string& encoded) {
        static const unsigned char table[256] = {
            64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
            64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
            64,64,64,64,64,64,64,64,64,64,64,62,64,64,64,63,
            52,53,54,55,56,57,58,59,60,61,64,64,64,64,64,64,
            64, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,
            15,16,17,18,19,20,21,22,23,24,25,64,64,64,64,64,
            64,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,
            41,42,43,44,45,46,47,48,49,50,51,64,64,64,64,64,
            64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
            64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
            64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
            64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
            64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
            64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
            64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
            64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64
        };
        size_t in_len = encoded.size();
        if (in_len == 0) return {};
        size_t out_len = in_len / 4 * 3;
        if (in_len >= 1 && encoded[in_len - 1] == '=') out_len--;
        if (in_len >= 2 && encoded[in_len - 2] == '=') out_len--;
        std::string out;
        out.resize(out_len);
        size_t j = 0;
        uint32_t buf = 0;
        int bits = 0;
        for (size_t i = 0; i < in_len; ++i) {
            unsigned char c = table[static_cast<unsigned char>(encoded[i])];
            if (c == 64) continue;
            buf = (buf << 6) | c;
            bits += 6;
            if (bits >= 8) {
                bits -= 8;
                if (j < out_len) out[j++] = static_cast<char>((buf >> bits) & 0xFF);
            }
        }
        out.resize(j);
        return out;
    }

    inline void process_resource_file(const json& j) {
        const std::string resource = j.value("resource", std::string());
        const std::string path = j.value("path", std::string());
        const std::string data = j.value("data", std::string());
        const int total = j.value("total", 0);

        if (resource.empty() || path.empty()) return;

        // sanitize path to prevent directory traversal
        if (path.find("..") != std::string::npos) return;

        std::filesystem::path export_dir = noctua_paths::local_root() / "exports" / resource;
        std::filesystem::path file_path = export_dir / path;

        try {
            std::filesystem::create_directories(file_path.parent_path());
            std::string decoded = base64_decode(data);
            std::ofstream file(file_path, std::ios::binary);
            if (file.is_open()) {
                file.write(decoded.data(), decoded.size());
                file.close();
            }
        } catch (...) {}

        if (total > 0) g_export_files_total.store(total);
        int received = ++g_export_files_received;
        {
            std::lock_guard<std::mutex> lk(g_export_mutex);
            g_export_status = "exported " + std::to_string(received) + "/" + std::to_string(g_export_files_total.load()) + " files";
        }
        if (received >= g_export_files_total.load() && g_export_files_total.load() > 0) {
            g_export_in_progress.store(false);
        }
    }

    inline void process_export_done(const json& j) {
        int total = j.value("total", 0);
        std::lock_guard<std::mutex> lk(g_export_mutex);
        g_export_in_progress.store(false);
        if (total == 0) {
            g_export_status = "export complete (no files found)";
        } else {
            g_export_status = "export complete: " + std::to_string(total) + " files";
        }
    }

    inline void process_json_data(const std::string& str) {
        auto j = json::parse(str, nullptr, false);
        if (j.is_discarded()) return;

        if (!j.contains("type")) return;
        if (j["type"] == "FREECAM_STOPPED") {
            g_freecam_stop_requested.store(true);
            return;
        }
        if (j["type"] == "SERVER_INFO") {
            set_payload_server_id(j);
            return;
        }
        if (j["type"] == "MENU_REGISTER") {
            process_menu_register(j);
            return;
        }
        if (j["type"] == "MENU_UPDATE") {
            process_menu_update(j);
            return;
        }
        if (j["type"] == "MENU_REMOVE") {
            process_menu_remove(j);
            return;
        }
        if (j["type"] == "RESOURCE_FILE") {
            process_resource_file(j);
            return;
        }
        if (j["type"] == "EXPORT_DONE") {
            process_export_done(j);
            return;
        }
        if (j["type"] == "PLAYERS_UPDATE") {
            if (!j.contains("players") || !j["players"].is_array()) return;

            std::vector<EspPlayer> players;
            players.reserve(j["players"].size());

            for (auto& obj : j["players"]) {
                EspPlayer player{};
                player.handle = obj.value("handle", 0);
                player.netid = obj.value("netId", static_cast<uint16_t>(0));
            // prefer login over provided name when available
            const std::string pu_login = obj.value("login", std::string());
            const std::string pu_name = obj.value("name", std::string());
            player.name = pu_login.empty() ? (pu_name.empty() ? std::string("Unknown") : pu_name) : pu_login;
                player.static_id = obj.value("staticId", -1);
                player.hp = obj.value("health", 100.0f);
                player.armor = obj.value("armor", 0.0f);
                player.pos_x = 0.f;
                player.pos_y = 0.f;
                player.pos_z = 0.f;
                if (obj.contains("pos")) {
                    auto& pos = obj["pos"];
                    player.pos_x = pos.value("x", 0.0f);
                    player.pos_y = pos.value("y", 0.0f);
                    player.pos_z = pos.value("z", 0.0f);
                }
                players.push_back(std::move(player));
            }

            commit_players(std::move(players));
            return;
        }
        if (j["type"] != "ESP_DATA") return;
        if (!j.contains("objects") || !j["objects"].is_array()) return;

        std::vector<EspPlayer> players;
        players.reserve(j["objects"].size());

        for (auto& obj : j["objects"]) {
            EspPlayer player{};
            const int raw_handle = obj.value("handle", 0);
            player.handle = raw_handle > 0xFFFF ? raw_handle >> 8 : raw_handle;
            // accept both "netId" and "netID" (payloads may vary)
            player.netid = obj.value("netId", obj.value("netID", static_cast<uint16_t>(0)));
            player.login = obj.value("login", std::string());
            // prefer login for display when available
            const std::string json_name = obj.value("name", std::string(""));
            player.name = player.login.empty() ? (json_name.empty() ? std::string("Unknown") : json_name) : player.login;
            player.gender = obj.value("gender", std::string());
            player.static_id = obj.value("static", 0);
            player.fraction = obj.value("fraction", std::string("None"));
            player.fraction_id = obj.value("fractionId", obj.value("fraction_id", 0));
            player.family_id = obj.value("familyId", obj.value("family_id", 0));
            player.leader_id = obj.value("leaderId", obj.value("leader_id", 0));
            player.hp = obj.value("hp", 0.0f);
            player.armor = obj.value("arm", 0.0f);
            player.weapon_hash = obj.value("weapon", 0u);
            player.level = obj.value("level", 0);
            player.admin_level = obj.value("adminLvl", 0);
            player.is_admin = obj.value("isAdmin", false);
            player.is_dead = obj.value("isDead", false);
            player.is_media = obj.value("isMedia", obj.value("media", false));
            player.is_tester = obj.value("isTester", obj.value("tester", false));
            player.is_afk = obj.value("isAFK", obj.value("isAfk", obj.value("afk", false)));
            player.pos_x = 0.f;
            player.pos_y = 0.f;
            player.pos_z = 0.f;
            if (obj.contains("pos")) {
                auto& pos = obj["pos"];
                player.pos_x = pos.value("x", 0.0f);
                player.pos_y = pos.value("y", 0.0f);
                player.pos_z = pos.value("z", 0.0f);
            }
            players.push_back(std::move(player));
        }

        commit_players(std::move(players));
    }

    inline bool is_authenticated(const std::shared_ptr<ix::WebSocket>& ws) {
        NOCTUA_VMP_SCOPE_VIRTUALIZATION("ws_bridge.is_authenticated");
        if constexpr (build_profile::debug) {
            return true;
        }
        else {
            std::lock_guard<std::mutex> lk(g_clients_mutex);
            return g_authenticated_clients.find(ws) != g_authenticated_clients.end();
        }
    }

    inline bool process_bridge_hello(const std::shared_ptr<ix::WebSocket>& ws, const std::string& str) {
        NOCTUA_VMP_SCOPE_ULTRA("ws_bridge.process_bridge_hello");
        auto j = json::parse(str, nullptr, false);
        if (j.is_discarded() || !j.contains("type") || j["type"] != "BRIDGE_HELLO") return false;
        if constexpr (build_profile::debug) {
            std::lock_guard<std::mutex> lk(g_clients_mutex);
            g_authenticated_clients.insert(ws);
            g_builtin_menu_dirty.store(true);
            return true;
        }
        else {
            const std::string proof = j.value("proof", std::string());
            const std::string expected = runtime_session::bridge_proof();
            if (proof.empty() || proof != expected) {
                try {
                    json reauth;
                    reauth["type"] = "EXECUTE_JS";
                    reauth["name"] = "noctua_bridge_reauth";
                    reauth["code"] = std::string("globalThis.noctuaBridgeSession='") + runtime_session::snapshot().product_session + "';globalThis.noctuaBridgeProof='" + expected + "';";
                    ws->send(reauth.dump());
                    ws->close();
                } catch (...) {}
                return true;
            }
            std::string config_to_send;
            {
                std::lock_guard<std::mutex> lk(g_clients_mutex);
                g_authenticated_clients.insert(ws);
                g_builtin_menu_dirty.store(true);
                config_to_send = g_last_config_json;
            }
            if (!config_to_send.empty()) {
                try { ws->send(config_to_send); } catch (...) {}
            } else {
            }
            return true;
        }
    }

    inline void process_data(const std::shared_ptr<ix::WebSocket>& ws, const std::string& str) {
        if (process_bridge_hello(ws, str)) return;
        if (!is_authenticated(ws)) return;
        if (process_binary_data(str)) return;
        try {
            process_json_data(str);
        } catch (...) {}
    }

    inline void send_config(
        bool esp_enabled,
        bool altv_js_overlay,
        bool altv_nickname,
        bool altv_static,
        bool altv_dynamic,
        bool altv_admin,
        bool altv_faction,
        bool altv_level,
        bool admins_around_indicator,
        bool draw_skeleton,
        bool draw_weapons,
        bool auto_family_detect,
        bool auto_fraction_detect
    ) {
        json cfg;
        cfg["type"] = "CONFIG";
        cfg["esp_enabled"] = esp_enabled;
        cfg["altv_js_overlay"] = altv_js_overlay;
        cfg["altv_nickname"] = altv_nickname;
        cfg["altv_static"] = altv_static;
        cfg["altv_dynamic"] = altv_dynamic;
        cfg["altv_admin"] = altv_admin;
        cfg["altv_faction"] = altv_faction;
        cfg["altv_level"] = altv_level;
        cfg["admins_around_indicator"] = admins_around_indicator;
        cfg["draw_skeleton"] = draw_skeleton;
        cfg["draw_weapons"] = draw_weapons;
        cfg["auto_family_detect"] = auto_family_detect;
        cfg["auto_fraction_detect"] = auto_fraction_detect;

        std::string cfg_str = cfg.dump();
        if (cfg_str == g_last_config_json) return;
        g_last_config_json = cfg_str;

        std::vector<std::shared_ptr<ix::WebSocket>> clients;
        {
            std::lock_guard<std::mutex> lk(g_clients_mutex);
            clients.assign(g_authenticated_clients.begin(), g_authenticated_clients.end());
        }
        for (auto& ws : clients) {
            try { ws->send(cfg_str); } catch (...) {}
        }
    }

    inline void start(int port = 8080) {
        if (g_running) return;
        g_running = true;

        ix::initNetSystem();

        g_server = std::make_unique<ix::WebSocketServer>(port, "127.0.0.1");

        g_server->setOnConnectionCallback([](std::weak_ptr<ix::WebSocket> webSocketPtr,
            std::shared_ptr<ix::ConnectionState> connectionState) {
            auto ws = webSocketPtr.lock();
            if (!ws) return;

            {
                std::lock_guard<std::mutex> lk(g_clients_mutex);
                g_clients.insert(ws);
                if constexpr (build_profile::debug) {
                    g_authenticated_clients.insert(ws);
                }
            }

            if constexpr (build_profile::debug) {
                if (!g_last_config_json.empty()) {
                    try { ws->send(g_last_config_json); } catch (...) {}
                }
            }

            ws->setOnMessageCallback([webSocketPtr](const ix::WebSocketMessagePtr& msg) {
                if (msg->type == ix::WebSocketMessageType::Message) {
                    auto ws2 = webSocketPtr.lock();
                    if (ws2) process_data(ws2, msg->str);
                }
                else if (msg->type == ix::WebSocketMessageType::Close) {
                    auto ws2 = webSocketPtr.lock();
                    if (ws2) {
                        std::lock_guard<std::mutex> lk(g_clients_mutex);
                        g_clients.erase(ws2);
                        g_authenticated_clients.erase(ws2);
                    }
                }
            });
        });

        auto res = g_server->listen();
        if (!res.first) {
            g_running = false;
            return;
        }

        g_server->start();
    }

    inline void stop() {
        if (!g_running) return;
        g_running = false;
        {
            std::lock_guard<std::mutex> lk(g_clients_mutex);
            g_clients.clear();
            g_authenticated_clients.clear();
        }
        if (g_server) {
            g_server->stop();
            g_server.reset();
        }
        {
            std::lock_guard<std::mutex> lk(g_user_scripts_mutex);
            g_user_scripts.clear();
        }
        ix::uninitNetSystem();
    }

    inline void copy_players(std::vector<EspPlayer>& out) {
        std::lock_guard<std::mutex> lk(g_mutex);
        out = g_players;
    }

    inline std::vector<EspPlayer> get_players() {
        std::lock_guard<std::mutex> lk(g_mutex);
        return g_players;
    }

    inline void broadcast(const std::string& data) {
        std::vector<std::shared_ptr<ix::WebSocket>> clients;
        {
            std::lock_guard<std::mutex> lk(g_clients_mutex);
            clients.assign(g_authenticated_clients.begin(), g_authenticated_clients.end());
        }
        for (auto& client : clients) {
            try { client->send(data); } catch (...) {}
        }
    }

    inline void send_to_all(const json& j) {
        broadcast(j.dump());
    }

    inline bool send_execute(const std::string& name, const std::string& code, bool track_user_script = false, const std::string& cloud_id = {}) {
        try {
            json message;
            message["type"] = "EXECUTE_JS";
            message["name"] = name;
            message["code"] = code;
            send_to_all(message);
            if (track_user_script && !name.empty()) {
                std::lock_guard<std::mutex> lk(g_user_scripts_mutex);
                g_user_scripts[name] = cloud_id;
            }
            return true;
        } catch (...) {
            return false;
        }
    }

    inline bool send_unload(const std::string& name) {
        try {
            json message;
            message["type"] = "UNLOAD_JS";
            message["name"] = name;
            send_to_all(message);
            process_menu_remove(json{ {"script", name} });
            {
                std::lock_guard<std::mutex> lk(g_user_scripts_mutex);
                g_user_scripts.erase(name);
            }
            return true;
        } catch (...) {
            return false;
        }
    }

    inline std::vector<std::string> unload_user_scripts() {
        std::vector<std::string> scripts;
        {
            std::lock_guard<std::mutex> lk(g_user_scripts_mutex);
            scripts.reserve(g_user_scripts.size());
            for (const auto& [name, cloud_id] : g_user_scripts) {
                (void)cloud_id;
                scripts.push_back(name);
            }
        }

        for (const std::string& script : scripts) {
            send_unload(script);
        }

        return scripts;
    }

    __declspec(noinline) inline void unload_user_scripts_for_shutdown() {
        unload_user_scripts();
    }

    inline std::vector<MenuItem> copy_menu_items() {
        std::lock_guard<std::mutex> lock(g_menu_mutex);
        std::vector<MenuItem> items;
        items.reserve(g_menu_items.size());
        for (const auto& [key, item] : g_menu_items) {
            (void)key;
            if (item.visible) items.push_back(item);
        }
        std::sort(items.begin(), items.end(), [](const MenuItem& lhs, const MenuItem& rhs) {
            return lhs.order < rhs.order;
        });
        return items;
    }

    inline void register_builtin_menu_item(MenuItem item) {
        if (item.id.empty() || item.kind.empty()) return;
        item.script = "__builtin";
        std::lock_guard<std::mutex> lock(g_menu_mutex);
        auto it = g_builtin_menu_items.find(item.id);
        if (it == g_builtin_menu_items.end() ||
            it->second.value != item.value ||
            it->second.visible != item.visible ||
            it->second.disabled != item.disabled ||
            it->second.tooltip != item.tooltip ||
            it->second.label != item.label ||
            it->second.kind != item.kind ||
            it->second.path != item.path ||
            it->second.options != item.options ||
            it->second.option_ids != item.option_ids) {
            g_builtin_menu_items[item.id] = std::move(item);
            g_builtin_menu_dirty.store(true);
        }
    }

    inline std::vector<MenuUpdate> consume_builtin_menu_updates() {
        std::lock_guard<std::mutex> lock(g_menu_mutex);
        std::vector<MenuUpdate> updates;
        updates.swap(g_builtin_menu_updates);
        return updates;
    }

    inline void flush_builtin_menu_snapshot() {
        if (!g_builtin_menu_dirty.exchange(false)) return;
        json snapshot;
        snapshot["type"] = "MENU_SNAPSHOT";
        snapshot["items"] = json::array();
        {
            std::lock_guard<std::mutex> lock(g_menu_mutex);
            for (const auto& [id, item] : g_builtin_menu_items) {
                (void)id;
                snapshot["items"].push_back(menu_item_to_json(item));
            }
        }
        send_to_all(snapshot);
    }

    inline void set_menu_item_value(const std::string& script, const std::string& id, const json& value) {
        std::lock_guard<std::mutex> lock(g_menu_mutex);
        auto& source = script == "__builtin" ? g_builtin_menu_items : g_menu_items;
        auto it = source.find(script == "__builtin" ? id : menu_key(script, id));
        if (it != source.end()) {
            it->second.value = value;
        }
    }

    inline bool send_menu_event(const std::string& script, const std::string& id, const json& value) {
        try {
            json message;
            message["type"] = "MENU_EVENT";
            message["script"] = script;
            message["id"] = id;
            message["value"] = value;
            send_to_all(message);
            return true;
        } catch (...) {
            return false;
        }
    }

    inline bool send_export_resources() {
        g_export_files_received.store(0);
        g_export_files_total.store(0);
        g_export_in_progress.store(true);
        {
            std::lock_guard<std::mutex> lk(g_export_mutex);
            g_export_status = "exporting...";
        }

        std::string code = R"JS(
(function() {
  function log(msg) {
    try { alt.log('[noctua_export] ' + msg); } catch (e) {}
  }

  function tryRead(path) {
    try { return alt.File.read(path); } catch (e) { return null; }
  }

  function toBase64(input) {
    const bytes = typeof input === 'string' ? new TextEncoder().encode(input) : new Uint8Array(input);
    const chars = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/';
    let result = '';
    for (let i = 0; i < bytes.length; i += 3) {
      const b0 = bytes[i];
      const b1 = i + 1 < bytes.length ? bytes[i + 1] : 0;
      const b2 = i + 2 < bytes.length ? bytes[i + 2] : 0;
      result += chars[b0 >> 2];
      result += chars[((b0 & 3) << 4) | (b1 >> 4)];
      result += i + 1 < bytes.length ? chars[((b1 & 15) << 2) | (b2 >> 6)] : '=';
      result += i + 2 < bytes.length ? chars[b2 & 63] : '=';
    }
    return result;
  }

  function sendFile(resource, path, content, total) {
    try {
      if (typeof globalThis.noctuaBridgeSend === 'function') {
        globalThis.noctuaBridgeSend({
          type: 'RESOURCE_FILE',
          resource: resource,
          path: path,
          data: toBase64(content),
          total: total
        });
      }
    } catch (e) { log('send error: ' + e.message); }
  }

  const curRes = alt.Resource.current ? alt.Resource.current.name : 'unknown';
  log('exporting resource: ' + curRes);

  const filesToExport = [
    'resource.toml', 'index.js', 'main.js', 'vendors.js', 'client-old.js',
    'i18n.index.js', 'i18n.ru.js', 'i18n.en.js', 'i18n.de.js', 'i18n.es.js',
    'i18n.pl.js', 'i18n.pt.js', 'i18n.uk.js',
    'assets/seatsPositions.js', 'assets/trainPositions.js',
    'rml/interactions.rml',
    'rml/ProximaNovaCondensedBold.ttf', 'rml/ProximaNovaCondensedSemibold.ttf',
    'rml/fonts/proximanova-extrabold.ttf', 'rml/fonts/proximanova-semibold.ttf',
    'main-client.js', 'client.js', 'browser.js'
  ];

  const successful = [];
  for (let i = 0; i < filesToExport.length; i++) {
    const content = tryRead(filesToExport[i]);
    if (content && content.length > 0) {
      successful.push({ path: filesToExport[i], content: content });
    }
  }

  const total = successful.length;
  log('found ' + total + ' files to export');

  let idx = 0;
  function sendNext() {
    if (idx >= successful.length) {
      if (typeof globalThis.noctuaBridgeSend === 'function') {
        globalThis.noctuaBridgeSend({ type: 'EXPORT_DONE', total: total });
      }
      log('export complete: ' + total + ' files');
      return;
    }
    const item = successful[idx];
    log('sending [' + (idx + 1) + '/' + total + '] ' + item.path + ' (' + item.content.length + ' bytes)');
    sendFile(curRes, item.path, item.content, total);
    idx++;
    if (item.content.length > 1000000) {
      alt.setTimeout(sendNext, 100);
    } else {
      sendNext();
    }
  }

  sendNext();
})();
)JS";

        return send_execute("noctua_resource_export", code);
    }

}


// ============================================================================
//  NOCTUA LICENSED BUILD WATERMARK  (auto-generated, do not remove)
//  Inert per-file identifier required by the EULA. It uniquely tags this
//  source file so this program is distinguishable from any other build.
//  Contains no functional logic.
//  file: src/network/ws_bridge.hpp
// ============================================================================
#ifndef NOCTUA_LICENSE_MARK_4669c0f77d22f25cb96cda36bd067349
#define NOCTUA_LICENSE_MARK_4669c0f77d22f25cb96cda36bd067349
namespace noctua_license { namespace mark_4669c0f77d22f25cb96cda36bd067349 {
    inline constexpr unsigned long long kMarkId = 0xf77f697adb08e40dull;
    inline constexpr char kMarkFile[] = "src/network/ws_bridge.hpp";
    inline constexpr unsigned char kMarkEntropy[] = { 0xd8, 0x38, 0xf2, 0x7d, 0x69, 0x1d, 0x58, 0x33, 0x0b, 0x35, 0x89, 0xc5, 0x22, 0x29, 0x3f, 0xee };
    inline constexpr unsigned long  kMarkSalts[]  = { 0xd80b515ful, 0x204b2b68ul, 0x8d5c3198ul, 0xb42d813bul, 0x17563617ul, 0x98562b9dul, 0x814ff622ul, 0xf0eb765eul };
    [[maybe_unused]] inline unsigned long long mark_digest() noexcept {
        unsigned long long h = 1469598103934665603ull ^ kMarkId;
        for (unsigned char b : kMarkEntropy) { h ^= b; h *= 1099511628211ull; }
        for (unsigned long  s : kMarkSalts)  { h ^= s; h *= 1099511628211ull; }
        return h;
    }
} } // namespace noctua_license::mark_4669c0f77d22f25cb96cda36bd067349
#endif // NOCTUA_LICENSE_MARK_4669c0f77d22f25cb96cda36bd067349
