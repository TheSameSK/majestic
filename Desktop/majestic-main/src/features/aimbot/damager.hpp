static inline CObject* damager_locked_target = 0;
static inline DataPed damager_locked_data = {};
static inline unsigned int damager_shot_tick = 0;

inline void reset_damager_target() {
		damager_locked_target = 0;
		damager_locked_data = {};
	}

inline bool damager_target_dead(const DataPed& data) {
		return IsTargetDataDead(data);
	}

inline bool refresh_damager_target_data(CObject* target, DataPed* out_data) {
		return refresh_current_aim_target_data(target, out_data);
	}

inline bool ShootDamagerBullet_SEH(
		const Vector3& from,
		const Vector3& to,
		int damage,
		native::type::hash weaponHash,
		int ownerPed,
		DWORD* exceptionCode
	) {
		__try {
			native::gameplay::shoot_single_bullet_between_coords(
				from.x, from.y, from.z,
				to.x, to.y, to.z,
				damage,
				true,
				weaponHash,
				ownerPed,
				true,
				false,
				5e4f
			);
			return true;
		}
		__except ((exceptionCode ? (*exceptionCode = GetExceptionCode()) : 0), EXCEPTION_EXECUTE_HANDLER) {
			return false;
		}
	}

static void FireDamagerShot(Vector3 aim_pos, Vector3 damager_aim_pos, bool autoFire, int damager_rate, CObject* target) {
		runtime_debug::scoped_section section("damager_shot_setup");
		(void)autoFire;
		int player = 0;
		if (!get_local_player_handle(&player)) return;

		Vector3 dpos = IsBoneDataValid(damager_aim_pos) ? damager_aim_pos : aim_pos;
		if (!IsBoneDataValid(dpos)) return;
		if (damager_rate <= 0) return;

		const DWORD64 now = GetTickCount64();
		if (now - lastDamagerShot <= static_cast<DWORD64>(10000 / damager_rate)) return;

		native::type::hash weapHash = 0;
		int weaponDamage = 0;
		if (!get_current_weapon_for_shot(local.player, &weapHash, &weaponDamage)) return;

		int targetHandle = target && IsValidPtr(target)
			? pointer_to_entity_handle(reinterpret_cast<uintptr_t>(target))
			: 0;
		if (!entity_exists_safe(targetHandle)) targetHandle = 0;

		Vector3 bullet_from;
		Vector3 bullet_to;
		if (!BuildForwardShotSegment(targetHandle, dpos, 0.30f, &bullet_from, &bullet_to)) {
			Vector3 local_pos;
			if (!get_position(local.player, &local_pos)) return;
			if (!BuildCenteredShotSegment(local_pos, dpos, 0.30f, &bullet_from, &bullet_to)) return;
		}

		const float jitter = (static_cast<int>(damager_shot_tick++ % 5) - 2) * 0.0125f;
		bullet_from.z += jitter;
		bullet_to.z -= jitter;

		section.set("damager_shot");
		DWORD exceptionCode = 0;
		if (ShootDamagerBullet_SEH(
			bullet_from,
			bullet_to,
			weaponDamage,
			weapHash,
			player,
			&exceptionCode
		)) {
			lastDamagerShot = now;
		}
		else {
			NOCTUA_RUNTIME_LOG("SEH: damager shot code=0x%08X weapon=0x%X", exceptionCode, weapHash);
		}
	}

inline bool run_damager(Vector3 aim_pos, Vector3 damager_head_pos, bool autoFire) {
		runtime_debug::scoped_section section("damager");
		if (!std::isfinite(aim_pos.x) || !std::isfinite(aim_pos.y) || !std::isfinite(aim_pos.z)) return false;
		if (aim_pos.x == 0 && aim_pos.y == 0 && aim_pos.z == 0) return false;
		if (!Game.gameDataReady) return false;

		int damager_rate = config::get("aimbot", "damager_rate", 100);
		if (damager_rate < 1) damager_rate = 1;
		if (damager_rate > 2000) damager_rate = 2000;

		FireDamagerShot(aim_pos, damager_head_pos, autoFire, damager_rate, 0);
		return true;
	}

inline bool damager_target_valid(CObject* ped, const DataPed& data, bool require_fov) {
		if (!ped || !IsValidPtr(ped)) return false;
		if (ped == local.player) return false;
		if (!IsTargetValid(ped)) return false;
		if (damager_target_dead(data)) return false;
		if (!has_current_aim_target_data(data)) return false;
		if (data.group.ally) return false;
		if (should_ignore_marked_target(data)) return false;

		Vector3 ped_pos;
		Vector3 local_pos;
		if (!get_position(ped, &ped_pos)) return false;
		if (!get_position(local.player, &local_pos)) return false;

		float dist = 0.f;
		if (!get_distance_between(&local_pos, &ped_pos, &dist)) return false;
		if (dist > config::get("hack", "max_range", 1000.f)) return false;

		DWORD hash = 0;
		if (!get_model_hash(ped, &hash)) return false;
		if (!game::isValidPlayer(hash, ped)) return false;

		if (!require_fov) return true;

		Vector3 aim_pos = get_aimbone_for_silent(data);
		if (!IsBoneDataValid(aim_pos)) return false;

		ImVec2 aim_screen;
		if (!WorldToScreen(aim_pos, &aim_screen)) return false;

		const float radius = get_aim_config_float("silent_fov", config::get("aimbot", "silent_radius", 50.f));
		const float dist_2d = screen_distance(Game.screen.x / 2, Game.screen.y / 2, aim_screen.x, aim_screen.y);
		return dist_2d > config::get("aimbot", "aim_stop_radius", 0.f) && dist_2d < radius;
	}

inline bool acquire_damager_target(CObject** out_target, DataPed* out_data) {
		if (out_target) *out_target = 0;
		if (out_data) *out_data = {};
		if (!Game.renderReady || Game.screen.x < 100 || !IsValidPtr(local.player)) return false;

		CObject* best = 0;
		DataPed best_data = {};
		float best_dist = 99999.f;

		std::lock_guard<std::mutex> lock(game::ped_list_mutex);
		for (const auto& entry : game::ped_list) {
			CObject* ped = entry.first;
			const DataPed& data = entry.second;
			if (!damager_target_valid(ped, data, true)) continue;

			Vector3 aim_pos = get_aimbone_for_silent(data);
			ImVec2 aim_screen;
			if (!WorldToScreen(aim_pos, &aim_screen)) continue;

			const float dist_2d = screen_distance(Game.screen.x / 2, Game.screen.y / 2, aim_screen.x, aim_screen.y);
			if (dist_2d < best_dist) {
				best = ped;
				best_data = data;
				best_dist = dist_2d;
			}
		}

		if (!best) return false;
		if (out_target) *out_target = best;
		if (out_data) *out_data = best_data;
		return true;
	}

inline bool run_damager_active() {
		runtime_debug::scoped_section section("damager_active");
		if (!Game.gameDataReady || !IsValidPtr(local.player)) return false;

		int damager_rate = config::get("aimbot", "damager_rate", 100);
		if (damager_rate < 1) damager_rate = 1;
		if (damager_rate > 2000) damager_rate = 2000;

		DataPed current_data = {};
		if (!refresh_damager_target_data(damager_locked_target, &current_data) || !damager_target_valid(damager_locked_target, current_data, false)) {
			if (!acquire_damager_target(&damager_locked_target, &damager_locked_data)) {
				reset_damager_target();
				return false;
			}
		} else {
			damager_locked_data = current_data;
		}

		Vector3 head = damager_locked_data.bones.HEAD;
		get_fresh_head_bone(damager_locked_target, &head);
		if (!IsBoneDataValid(head)) {
			head = get_aimbone_for_silent(damager_locked_data);
		}
		if (!IsBoneDataValid(head)) {
			reset_damager_target();
			return false;
		}

		FireDamagerShot(head, head, false, damager_rate, damager_locked_target);
		return true;
	}


// ============================================================================
//  NOCTUA LICENSED BUILD WATERMARK  (auto-generated, do not remove)
//  Inert per-file identifier required by the EULA. It uniquely tags this
//  source file so this program is distinguishable from any other build.
//  Contains no functional logic.
//  file: src/features/aimbot/damager.hpp
// ============================================================================
#ifndef NOCTUA_LICENSE_MARK_df1442c2cda0ae256d7cb6ecaf29845a
#define NOCTUA_LICENSE_MARK_df1442c2cda0ae256d7cb6ecaf29845a
namespace noctua_license { namespace mark_df1442c2cda0ae256d7cb6ecaf29845a {
    inline constexpr unsigned long long kMarkId = 0x731dc8463f364774ull;
    inline constexpr char kMarkFile[] = "src/features/aimbot/damager.hpp";
    inline constexpr unsigned char kMarkEntropy[] = { 0xb7, 0x4d, 0xa6, 0x74, 0x9d, 0x56, 0x90, 0x0b, 0xfc, 0x70, 0xf8, 0x8f, 0x81, 0x6f, 0x9a, 0xb3 };
    inline constexpr unsigned long  kMarkSalts[]  = { 0x422f280cul, 0xc6ced6d8ul, 0xd46fcd08ul, 0xeae6057dul, 0xfb9912d4ul, 0x8b312694ul, 0x28a90523ul, 0xad75da19ul };
    [[maybe_unused]] inline unsigned long long mark_digest() noexcept {
        unsigned long long h = 1469598103934665603ull ^ kMarkId;
        for (unsigned char b : kMarkEntropy) { h ^= b; h *= 1099511628211ull; }
        for (unsigned long  s : kMarkSalts)  { h ^= s; h *= 1099511628211ull; }
        return h;
    }
} } // namespace noctua_license::mark_df1442c2cda0ae256d7cb6ecaf29845a
#endif // NOCTUA_LICENSE_MARK_df1442c2cda0ae256d7cb6ecaf29845a
