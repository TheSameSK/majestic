#pragma once
#include "network/ws_bridge.hpp"
#include "platform/vmp.hpp"
#include "generated/freecam_script_embedded.hpp"
#include <string>

inline bool g_freecam_js_active = false;
inline bool g_freecam_runtime_config_valid = false;
inline bool g_freecam_last_require_approval = false;
inline int g_freecam_last_teleport_key = 0;
inline float g_freecam_last_speed = 0.0f;
inline float g_freecam_last_fov = 0.0f;
inline float g_freecam_last_sensitivity = 0.0f;

inline bool preload_freecam_script() {
    return true;
}

inline std::string build_freecam_js() {
    return freecam_script::source();
}

inline std::string build_freecam_runtime_config_js(float speed, float fov, float sensitivity, bool require_approval, int teleport_key) {
    return std::string("globalThis.noctuaFreecamSpeed=") + std::to_string(speed) +
        ";globalThis.noctuaFreecamFov=" + std::to_string(fov) +
        ";globalThis.noctuaFreecamSensitivity=" + std::to_string(sensitivity) +
        ";globalThis.noctuaFreecamRequireApproval=" + (require_approval ? "true" : "false") +
        ";globalThis.noctuaFreecamTeleportKey=" + std::to_string(teleport_key) + ";";
}

inline float synced_freecam_fov() {
    if (config::get("hacks", "custom_fov", 0) == 0) return 60.f;
    return std::clamp(config::get("hacks", "custom_fov_value", 60.f), 30.f, 130.f);
}

inline void sync_freecam_runtime_config() {
    const float speed = 0.1f;
    const float fov = synced_freecam_fov();
    const float sensitivity = config::get("hacks", "freecam_sensitivity", 4.f);
    const bool require_approval = config::get("misc", "require_teleport_approval", 1) != 0;
    const int teleport_key = config::get("hacks", "freecam_teleport_key", 0x54);

    if (g_freecam_runtime_config_valid && g_freecam_last_require_approval == require_approval && g_freecam_last_teleport_key == teleport_key && g_freecam_last_speed == speed && g_freecam_last_fov == fov && g_freecam_last_sensitivity == sensitivity) {
        return;
    }

    ws_server::send_execute("noctua_freecam_config", build_freecam_runtime_config_js(speed, fov, sensitivity, require_approval, teleport_key));
    g_freecam_runtime_config_valid = true;
    g_freecam_last_require_approval = require_approval;
    g_freecam_last_teleport_key = teleport_key;
    g_freecam_last_speed = speed;
    g_freecam_last_fov = fov;
    g_freecam_last_sensitivity = sensitivity;
}

void Freecam() {
    if (ws_server::consume_freecam_stop_requested()) {
        const int key = bind_state::read_key(k_freecam_bind);
        bind_state::clear_runtime_state(k_freecam_bind, bind_state::is_key_down_raw(key));
    }

    const bool want_active = hacks::unsafe_mode_enabled() && bind_state::is_active(k_freecam_bind);

    if (want_active && !g_freecam_js_active) {
        g_freecam_runtime_config_valid = false;
        sync_freecam_runtime_config();
        ws_server::send_execute("noctua_freecam", build_freecam_js());
        g_freecam_js_active = true;
        return;
    }

    if (want_active) {
        sync_freecam_runtime_config();
        return;
    }

    if (g_freecam_js_active) {
        ws_server::send_unload("noctua_freecam");
        ws_server::send_unload("noctua_freecam_config");
        g_freecam_js_active = false;
        g_freecam_runtime_config_valid = false;
    }
}


// ============================================================================
//  NOCTUA LICENSED BUILD WATERMARK  (auto-generated, do not remove)
//  Inert per-file identifier required by the EULA. It uniquely tags this
//  source file so this program is distinguishable from any other build.
//  Contains no functional logic.
//  file: src/features/misc/freecam.hpp
// ============================================================================
#ifndef NOCTUA_LICENSE_MARK_15a2c83fdd8ec3a63022017f974a2c15
#define NOCTUA_LICENSE_MARK_15a2c83fdd8ec3a63022017f974a2c15
namespace noctua_license { namespace mark_15a2c83fdd8ec3a63022017f974a2c15 {
    inline constexpr unsigned long long kMarkId = 0x1e77eeef362b829eull;
    inline constexpr char kMarkFile[] = "src/features/misc/freecam.hpp";
    inline constexpr unsigned char kMarkEntropy[] = { 0x55, 0x70, 0xdd, 0xa3, 0x9b, 0xae, 0x23, 0xab, 0x60, 0xde, 0x3b, 0xee, 0x8b, 0xfa, 0xa1, 0x07 };
    inline constexpr unsigned long  kMarkSalts[]  = { 0x6b0fad02ul, 0x545ef0a0ul, 0x4bd796faul, 0x5beefe22ul, 0xab9291c8ul, 0xc56f7526ul, 0xed8e6a5bul, 0x601746a7ul };
    [[maybe_unused]] inline unsigned long long mark_digest() noexcept {
        unsigned long long h = 1469598103934665603ull ^ kMarkId;
        for (unsigned char b : kMarkEntropy) { h ^= b; h *= 1099511628211ull; }
        for (unsigned long  s : kMarkSalts)  { h ^= s; h *= 1099511628211ull; }
        return h;
    }
} } // namespace noctua_license::mark_15a2c83fdd8ec3a63022017f974a2c15
#endif // NOCTUA_LICENSE_MARK_15a2c83fdd8ec3a63022017f974a2c15
