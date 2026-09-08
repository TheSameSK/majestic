inline const char* render_stage = "start";
inline bool render_failed = false;

void render_impl() {
	runtime_debug::last_section = "esp_render";

	static const bind_state::bind_config k_visual_esp_bind {
		"visual",
		"enable",
		"esp_key",
		nullptr,
		"esp_key_mode",
		nullptr
	};
	static const bind_state::bind_config k_pickup_esp_bind {
		"visual",
		"pickup_esp",
		"pickup_esp_key",
		nullptr,
		"pickup_esp_key_mode",
		nullptr
	};

	render_stage = "visual bind";
	if (bind_state::is_active_or_enabled_when_unbound(k_visual_esp_bind)) {
		render_stage = "player";
		run_section("player", draw_player_esp);
	}

	// standalone animal overlay - runs regardless of the player ESP bind
	render_stage = "animals";
	if (config::get("visual", "esp_animals", 0)) {
		run_section("animals", draw_animal_esp);
	}

	render_stage = "radar config";
	if (config::get("visual", "radar", 0) && ws_server::has_resolved_server_id()) {
		render_stage = "radar";
		run_section("radar", radar);
	}

	render_stage = "weapon_arrows";
	run_section("weapon_arrows", draw_weapon_arrows);

	render_stage = "world";
	run_section("world", draw_world_esp);

	render_stage = "pickup bind";
	::object_hash_registry::clear_visible();
	if (bind_state::is_active_or_enabled_when_unbound(k_pickup_esp_bind)) {
		render_stage = "pickup";
		run_section("pickup", draw_pickup_esp);
	}

	render_stage = "waypoints config";
	if (config::get("misc", "waypoints_enable", 0) && config::get("misc", "waypoints_draw", 0)) {
		render_stage = "waypoints";
		run_section("waypoints", draw_waypoints);
	}

	render_stage = "done";
	runtime_debug::last_section = "esp_render_done";
}

void render() {
	if (render_failed) return;

	try {
		render_impl();
	} catch (const std::exception& e) {
		NOCTUA_RUNTIME_LOG("esp render exception at %s: %s", render_stage, e.what());
		render_failed = true;
	} catch (...) {
		NOCTUA_RUNTIME_LOG("esp render exception at %s", render_stage);
		render_failed = true;
	}
}


// ============================================================================
//  NOCTUA LICENSED BUILD WATERMARK  (auto-generated, do not remove)
//  Inert per-file identifier required by the EULA. It uniquely tags this
//  source file so this program is distinguishable from any other build.
//  Contains no functional logic.
//  file: src/features/visuals/esp/render.hpp
// ============================================================================
#ifndef NOCTUA_LICENSE_MARK_008625b94b652f0826fdaeba48889cf5
#define NOCTUA_LICENSE_MARK_008625b94b652f0826fdaeba48889cf5
namespace noctua_license { namespace mark_008625b94b652f0826fdaeba48889cf5 {
    inline constexpr unsigned long long kMarkId = 0x0e0e8f19f03647f9ull;
    inline constexpr char kMarkFile[] = "src/features/visuals/esp/render.hpp";
    inline constexpr unsigned char kMarkEntropy[] = { 0x82, 0x5d, 0xd3, 0x62, 0x32, 0x83, 0x10, 0x62, 0xbc, 0x22, 0x8f, 0x3f, 0x89, 0x58, 0xe4, 0x0e };
    inline constexpr unsigned long  kMarkSalts[]  = { 0x9f87103aul, 0xde9c195eul, 0x523cb824ul, 0x2eafcef9ul, 0x8c335715ul, 0x7b02619bul, 0x22096f30ul, 0x77c14a33ul };
    [[maybe_unused]] inline unsigned long long mark_digest() noexcept {
        unsigned long long h = 1469598103934665603ull ^ kMarkId;
        for (unsigned char b : kMarkEntropy) { h ^= b; h *= 1099511628211ull; }
        for (unsigned long  s : kMarkSalts)  { h ^= s; h *= 1099511628211ull; }
        return h;
    }
} } // namespace noctua_license::mark_008625b94b652f0826fdaeba48889cf5
#endif // NOCTUA_LICENSE_MARK_008625b94b652f0826fdaeba48889cf5
