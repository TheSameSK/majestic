inline bool ShootSingleBulletBetweenCoords_SEH(
float fromX,
		float fromY,
		float fromZ,
		float toX,
		float toY,
		float toZ,
		int damage,
		bool trace,
		native::type::hash weaponHash,
		int ownerPed,
		bool isAudible,
		bool isInvisible,
		float speed,
		DWORD* exceptionCode
	) {
		__try {
			native::gameplay::shoot_single_bullet_between_coords(
				fromX, fromY, fromZ,
				toX, toY, toZ,
				damage, trace, weaponHash, ownerPed, isAudible, isInvisible, speed
			);
			return true;
		}
		__except ((exceptionCode ? (*exceptionCode = GetExceptionCode()) : 0), EXCEPTION_EXECUTE_HANDLER) {
			return false;
		}
	}

	inline bool shoot_single_bullet_between_coords_checked(
		const Vector3& from,
		const Vector3& to,
		int damage,
		bool trace,
		native::type::hash weaponHash,
		int ownerPed,
		bool isAudible,
		bool isInvisible,
		float speed,
		const char* context
	) {
		if (!IsBoneDataValid(from) || !IsBoneDataValid(to)) {
			NOCTUA_RUNTIME_LOG("[%s] skip bullet: invalid vector", context ? context : "silent");
			return false;
		}
		if (damage <= 0 || weaponHash == 0 || ownerPed == 0 || !std::isfinite(speed) || speed <= 0.f) {
			NOCTUA_RUNTIME_LOG(
				"[%s] skip bullet: dmg=%d weapon=0x%X owner=%d speed=%.2f",
				context ? context : "silent",
				damage,
				weaponHash,
				ownerPed,
				speed
			);
			return false;
		}

		runtime_debug::last_section = context ? context : "silent_bullet";
		logs::add(
			std::string("[") + (context ? context : "silent") + "] bullet"
			+ " weapon=0x" + std::to_string(static_cast<unsigned int>(weaponHash))
			+ " dmg=" + std::to_string(damage)
			+ " owner=" + std::to_string(ownerPed)
		);

		DWORD exceptionCode = 0;
		if (!ShootSingleBulletBetweenCoords_SEH(
			from.x, from.y, from.z,
			to.x, to.y, to.z,
			damage, trace, weaponHash, ownerPed, isAudible, isInvisible, speed,
			&exceptionCode
		)) {
			NOCTUA_RUNTIME_LOG(
				"SEH: %s bullet code=0x%08X weapon=0x%X dmg=%d speed=%.2f from=(%.2f %.2f %.2f) to=(%.2f %.2f %.2f) owner=%d",
				context ? context : "silent",
				exceptionCode,
				weaponHash,
				damage,
				speed,
				from.x, from.y, from.z,
				to.x, to.y, to.z,
				ownerPed
			);
			return false;
		}
		return true;
	}

	inline bool BuildCenteredShotSegment(
		const Vector3& shotOrigin,
		const Vector3& impactPos,
		float halfLength,
		Vector3* outFrom,
		Vector3* outTo
	) {
		if (!outFrom || !outTo) return false;
		if (!IsBoneDataValid(shotOrigin) || !IsBoneDataValid(impactPos)) return false;
		if (!std::isfinite(halfLength) || halfLength <= 0.f) return false;

		Vector3 dir(
			impactPos.x - shotOrigin.x,
			impactPos.y - shotOrigin.y,
			impactPos.z - shotOrigin.z
		);
		const float len = std::sqrt(dir.x * dir.x + dir.y * dir.y + dir.z * dir.z);
		if (!std::isfinite(len) || len <= 0.001f) {
			dir = Vector3(0.f, 1.f, 0.f);
		}
		else {
			const float invLen = 1.f / len;
			dir.x *= invLen;
			dir.y *= invLen;
			dir.z *= invLen;
		}

		*outFrom = Vector3(
			impactPos.x - dir.x * halfLength,
			impactPos.y - dir.y * halfLength,
			impactPos.z - dir.z * halfLength
		);
		*outTo = Vector3(
			impactPos.x + dir.x * halfLength,
			impactPos.y + dir.y * halfLength,
			impactPos.z + dir.z * halfLength
		);
		return IsBoneDataValid(*outFrom) && IsBoneDataValid(*outTo);
	}

	inline bool write_current_weapon_shot_pos(CObject* ped, const Vector3& shotPos, const char* context) {
		__try {
			if (!ped || !IsValidPtr(ped)) return false;
			if (!IsBoneDataValid(shotPos)) return false;

			CWeaponManager* wm = ped->weapon();
			if (!wm || !IsValidPtr(wm)) return false;
			if (!wm->_CurrentWeapon || !IsValidPtr(wm->_CurrentWeapon)) return false;

			const uintptr_t weaponObj = reinterpret_cast<uintptr_t>(wm->_CurrentWeapon);
			if (weaponObj < 0x10000 || weaponObj > 0x7FFFFFFFFFFF) return false;

			*reinterpret_cast<float*>(weaponObj + 0x20) = shotPos.x;
			*reinterpret_cast<float*>(weaponObj + 0x24) = shotPos.y;
			*reinterpret_cast<float*>(weaponObj + 0x28) = shotPos.z;
			return true;
		}
		__except (EXCEPTION_EXECUTE_HANDLER) {
			NOCTUA_RUNTIME_LOG(
				"SEH: %s write_current_weapon_pos code=0x%08X pos=(%.2f %.2f %.2f)",
				context ? context : "magic_bullet",
				GetExceptionCode(),
				shotPos.x, shotPos.y, shotPos.z
			);
			return false;
		}
	}

	inline bool apply_magic_bullet(CObject* ped, const Vector3& shotOrigin, const Vector3& aimPos, const char* context) {
		if (!IsBoneDataValid(shotOrigin) || !IsBoneDataValid(aimPos)) return false;

		Vector3 dir(
			aimPos.x - shotOrigin.x,
			aimPos.y - shotOrigin.y,
			aimPos.z - shotOrigin.z
		);
		const float len = std::sqrt(dir.x * dir.x + dir.y * dir.y + dir.z * dir.z);
		if (!std::isfinite(len) || len <= 0.001f) {
			dir = Vector3(0.f, 1.f, 0.f);
		}
		else {
			const float invLen = 1.f / len;
			dir.x *= invLen;
			dir.y *= invLen;
			dir.z *= invLen;
		}

		const Vector3 magicPos(
			aimPos.x - dir.x * 0.20f,
			aimPos.y - dir.y * 0.20f,
			aimPos.z - dir.z * 0.20f + 0.05f
		);
		return write_current_weapon_shot_pos(ped, magicPos, context);
	}

	inline bool IsSilentFireTriggered(int player, bool autoFire) {
		if (player == 0) return false;
		return
			native::ped::is_ped_shooting(player) ||
			native::controls::is_disabled_control_pressed(0, 24) ||
			native::controls::is_disabled_control_just_pressed(0, 24) ||
			autoFire;
	}

	inline int get_weapon_shot_cooldown_ms(CObject* ped, int defaultMs = 75) {
		__try {
			if (!ped || !IsValidPtr(ped)) return defaultMs;
			CWeaponManager* wm = ped->weapon();
			if (!wm || !IsValidPtr(wm)) return defaultMs;
			if (!wm->_WeaponInfo || !IsValidPtr(wm->_WeaponInfo)) return defaultMs;

			float timeToShoot = wm->_WeaponInfo->TimeToShoot;
			if (!std::isfinite(timeToShoot) || timeToShoot <= 0.f) return defaultMs;

			int ms = static_cast<int>(std::round(timeToShoot * 1000.f));
			if (ms < 25) ms = 25;
			if (ms > 250) ms = 250;
			return ms;
		}
		__except (EXCEPTION_EXECUTE_HANDLER) {
			return defaultMs;
		}
	}

	inline bool DidSilentShotActuallyHappen(int player, native::type::hash weapHash, bool autoFire) {
		if (player == 0 || weapHash == 0) return false;

		if (autoFire) {
			const DWORD64 now = GetTickCount64();
			const int shotCooldownMs = get_weapon_shot_cooldown_ms(local.player, 75);
			if (now - lastSilentWallshotShot < static_cast<DWORD64>(shotCooldownMs)) return false;
			lastSilentWallshotShot = now;
			return true;
		}

		int ammoInClip = 0;
		if (!native::weapon::get_ammo_in_clip(player, weapHash, &ammoInClip)) {
			return native::controls::is_disabled_control_just_pressed(0, 24);
		}

		if (lastSilentClipWeapon != weapHash || lastSilentClipAmmo < 0) {
			lastSilentClipWeapon = weapHash;
			lastSilentClipAmmo = ammoInClip;
			return false;
		}

		const bool didShoot = ammoInClip < lastSilentClipAmmo;
		lastSilentClipAmmo = ammoInClip;
		return didShoot;
	}

	inline bool DidSilentFireEventOccur(int player, native::type::hash weapHash, bool autoFire) {
		if (player == 0 || weapHash == 0) return false;
		if (!IsSilentFireTriggered(player, autoFire)) return false;
		if (DidSilentShotActuallyHappen(player, weapHash, autoFire)) return true;
		return !autoFire && native::controls::is_disabled_control_just_pressed(0, 24);
	}

	inline bool IsWallshotFireReady(int player, bool autoFire) {
		if (player == 0) return false;
		if (!IsSilentFireTriggered(player, autoFire)) return false;

		const DWORD64 now = GetTickCount64();
		const int shotCooldownMs = get_weapon_shot_cooldown_ms(local.player, 75);
		if (now - lastSilentWallshotShot < static_cast<DWORD64>(shotCooldownMs)) {
			return false;
		}

		lastSilentWallshotShot = now;
		return true;
	}

	inline bool entity_exists_safe(int entityHandle) {
		__try {
			return entityHandle != 0 && native::entity::does_entity_exist(entityHandle);
		}
		__except (EXCEPTION_EXECUTE_HANDLER) {
			NOCTUA_RUNTIME_LOG("SEH: entity_exists code=0x%08X entity=%d", GetExceptionCode(), entityHandle);
			return false;
		}
	}

	inline bool read_entity_forward_xy(int entityHandle, float* outX, float* outY) {
		if (!outX || !outY) return false;
		if (!entity_exists_safe(entityHandle)) return false;

		__try {
			*outX = native::entity::get_entity_forward_x(entityHandle);
			*outY = native::entity::get_entity_forward_y(entityHandle);
			return true;
		}
		__except (EXCEPTION_EXECUTE_HANDLER) {
			NOCTUA_RUNTIME_LOG("SEH: entity_forward code=0x%08X entity=%d", GetExceptionCode(), entityHandle);
			return false;
		}
	}

	inline bool BuildForwardShotSegment(
		int entityHandle,
		const Vector3& impactPos,
		float halfLength,
		Vector3* outFrom,
		Vector3* outTo
	) {
		if (!outFrom || !outTo) return false;
		if (entityHandle == 0) return false;
		if (!IsBoneDataValid(impactPos)) return false;
		if (!std::isfinite(halfLength) || halfLength <= 0.f) return false;

		float forwardX = 0.f;
		float forwardY = 0.f;
		if (!read_entity_forward_xy(entityHandle, &forwardX, &forwardY)) return false;
		if (!std::isfinite(forwardX) || !std::isfinite(forwardY)) return false;

		Vector3 dir(forwardX, forwardY, 0.f);
		const float len = std::sqrt(dir.x * dir.x + dir.y * dir.y);
		if (!std::isfinite(len) || len <= 0.001f) return false;

		const float invLen = 1.f / len;
		dir.x *= invLen;
		dir.y *= invLen;

		*outFrom = Vector3(
			impactPos.x + dir.x * halfLength,
			impactPos.y + dir.y * halfLength,
			impactPos.z
		);
		*outTo = Vector3(
			impactPos.x - dir.x * halfLength,
			impactPos.y - dir.y * halfLength,
			impactPos.z
		);
		return IsBoneDataValid(*outFrom) && IsBoneDataValid(*outTo);
	}

	inline int pointer_to_entity_handle(uintptr_t ptr) {
		__try {
			if (!pointer_to_handle || !ptr) return 0;
			return pointer_to_handle(static_cast<intptr_t>(ptr));
		}
		__except (EXCEPTION_EXECUTE_HANDLER) {
			return 0;
		}
	}

	static void FireSilentFallback(Vector3 aim_pos, bool autoFire) {
		int player = 0;
		if (!get_local_player_handle(&player)) return;
		if (!IsValidPtr(local.player)) return;
		if (native::ped::is_ped_reloading(player)) return;
		native::type::hash weapHash = 0;
		int weaponDamage = 0;
		if (!get_current_weapon_for_shot(local.player, &weapHash, &weaponDamage)) return;
		if (!DidSilentFireEventOccur(player, weapHash, autoFire)) return;
		native::ped::set_ped_shoots_at_coord(player, aim_pos.x, aim_pos.y, aim_pos.z, false);
	}

	static void FireWallshotSilent(const ExecutionContext& ctx, bool autoFire) {
		int player = 0;
		if (!get_local_player_handle(&player)) return;
		if (!IsBoneDataValid(ctx.aim_pos)) return;

		bool isInVehicle = false;
		if (is_in_vehicle(local.player, &isInVehicle) && isInVehicle) {
			runtime_debug::last_section = "silent_vehicle_fallback";
			FireSilentFallback(ctx.aim_pos, autoFire);
			return;
		}

		if (native::ped::is_ped_reloading(player)) return;
		if (!IsWallshotFireReady(player, autoFire)) return;

		native::type::hash weapHash = 0;
		int weaponDamage = 0;
		if (!get_current_weapon_for_shot(local.player, &weapHash, &weaponDamage)) return;

		Vector3 localPos;
		if (!get_position(local.player, &localPos)) return;

		Vector3 bullet_from, bullet_to;
		int targetHandle = 0;
		if (ctx.target && IsValidPtr(ctx.target)) {
			targetHandle = pointer_to_entity_handle(reinterpret_cast<uintptr_t>(ctx.target));
			if (!entity_exists_safe(targetHandle)) targetHandle = 0;
		}

		if (!BuildForwardShotSegment(targetHandle, ctx.aim_pos, 0.20f, &bullet_from, &bullet_to)) {
			if (!BuildCenteredShotSegment(localPos, ctx.aim_pos, 0.20f, &bullet_from, &bullet_to)) return;
		}

		runtime_debug::last_section = "magic_bullet";
		shoot_single_bullet_between_coords_checked(
			bullet_from,
			bullet_to,
			weaponDamage,
			true,
			weapHash,
			player,
			true,
			false,
			5e4f,
			"magic_bullet"
		);
	}

	static void _run_silent_aim_impl(Vector3 aim_pos, bool autoFire) {
		int player = 0;
		if (!get_local_player_handle(&player)) return;

		auto playerPed = native::player::get_player_ped_script_index(native::player::player_id());
		if (playerPed == 0) return;
		float h = native::entity::get_entity_heading(playerPed);
		if (!std::isfinite(h)) return;

		native::type::hash weapHash = 0;
		int weaponDamage = 0;
		if (!get_current_weapon_for_shot(local.player, &weapHash, &weaponDamage)) return;
		if (native::ped::is_ped_reloading(player)) return;
		if (!DidSilentFireEventOccur(player, weapHash, autoFire)) return;

		const Vector3 bullet_from(aim_pos.x + cos(h) * .15f, aim_pos.y + sin(h) * .15f, aim_pos.z);
		const Vector3 bullet_to(aim_pos.x - cos(h) * .15f, aim_pos.y - sin(h) * .15f, aim_pos.z);
		runtime_debug::last_section = "silent_impl";
		shoot_single_bullet_between_coords_checked(
			bullet_from,
			bullet_to,
			weaponDamage,
			false,
			weapHash,
			player,
			true,
			false,
			3.f,
			"silent_impl"
		);
	}

	inline bool run_silent_aim(Vector3 aim_pos, Vector3, bool autoFire) {
		if (!std::isfinite(aim_pos.x) || !std::isfinite(aim_pos.y) || !std::isfinite(aim_pos.z)) return false;
		if (aim_pos.x == 0 && aim_pos.y == 0 && aim_pos.z == 0) return false;
		if (!Game.gameDataReady) return false;

		_run_silent_aim_impl(aim_pos, autoFire);
		return true;
	}

float RandomFloat(float a, float b) {
float random = ((float)rand()) / (float)RAND_MAX;
		float diff = b - a;
		float r = random * diff;
		return a + r;
	}

	__int64 __fastcall on_bullet_instance(
		__int64* sBulletInstance_1,
		const __int64* pWeaponInfo,
		Vector3* vStart,
		Vector3* vEnd,
		float fVelocity,
		unsigned int weaponHash,
		bool createsTrace,
		bool isAccurate
	) {
		if (!original_bullet_instance) {
			return 0;
		}

		try {
			if (!vStart || !vEnd) {
				return original_bullet_instance(sBulletInstance_1, pWeaponInfo, vStart, vEnd, fVelocity, weaponHash, createsTrace, isAccurate);
			}

			if (!bind_state::is_active(k_silent_aim_bind)) {
				return original_bullet_instance(sBulletInstance_1, pWeaponInfo, vStart, vEnd, fVelocity, weaponHash, createsTrace, isAccurate);
			}

			if (config::get("aimbot", "silent_aim_mode", 0) != 0) {
				return original_bullet_instance(sBulletInstance_1, pWeaponInfo, vStart, vEnd, fVelocity, weaponHash, createsTrace, isAccurate);
			}

			ExecutionContext ctx;
			{
				std::lock_guard<std::mutex> lock(execution_mutex);
				ctx = exec_ctx;
			}

			// can_fire keeps the unarmed case passthrough: the lock exists for
			// Silent Line only, bullets must not be redirected without a weapon
			if (!ctx.active || !ctx.can_fire || !IsBoneDataValid(ctx.aim_pos)) {
				return original_bullet_instance(sBulletInstance_1, pWeaponInfo, vStart, vEnd, fVelocity, weaponHash, createsTrace, isAccurate);
			}

			if (!has_current_aim_target(ctx.target)) {
				return original_bullet_instance(sBulletInstance_1, pWeaponInfo, vStart, vEnd, fVelocity, weaponHash, createsTrace, isAccurate);
			}

			const float chance = std::clamp(config::get("aimbot", "hit_chance_value", 0.f), 0.f, 100.f);
			if (chance > 0.f && chance < 100.f && RandomFloat(0.f, 100.f) > chance) {
				return original_bullet_instance(sBulletInstance_1, pWeaponInfo, vStart, vEnd, fVelocity, weaponHash, createsTrace, isAccurate);
			}

			Vector3 aim_pos = ctx.aim_pos;
			if (ctx.target && IsValidPtr(ctx.target)) {
				Vector3 fresh_bone;
				if (get_fresh_silent_bone(ctx.target, &fresh_bone) && IsBoneDataValid(fresh_bone)) {
					aim_pos = fresh_bone;
				}
			}

			if (!IsBoneDataValid(aim_pos)) {
				return original_bullet_instance(sBulletInstance_1, pWeaponInfo, vStart, vEnd, fVelocity, weaponHash, createsTrace, isAccurate);
			}

			const bool show_tracers = config::get("aimbot", "silent_show_tracers", 0) != 0;
			const bool magic_bullet = config::get("aimbot", "magic_bullet", 0) != 0;
			vEnd->x = aim_pos.x;
			vEnd->y = aim_pos.y;
			vEnd->z = aim_pos.z;

			if (magic_bullet) {
				vStart->x = aim_pos.x;
				vStart->y = aim_pos.y;
				vStart->z = aim_pos.z + 0.10f;
				runtime_debug::last_section = "roe_magic_bullet";
			}
			else {
				runtime_debug::last_section = "roe_silent_bullet";
			}

			return original_bullet_instance(sBulletInstance_1, pWeaponInfo, vStart, vEnd, fVelocity, weaponHash, show_tracers, isAccurate);
		}
		catch (...) {
			NOCTUA_RUNTIME_LOG("EX: on_bullet_instance");
		}

		return original_bullet_instance(sBulletInstance_1, pWeaponInfo, vStart, vEnd, fVelocity, weaponHash, createsTrace, isAccurate);
	}

	inline void install_bullet_instance_hook() {
		if (bullet_instance_hook_installed.load()) {
			return;
		}

		std::lock_guard<std::mutex> lock(bullet_instance_hook_mutex);
		if (bullet_instance_hook_installed.load()) {
			return;
		}

		if (!gta_bullet_instance) {
			add_log("skip silent bullet hook: gta_bullet_instance missing");
			return;
		}

		const MH_STATUS create_status =
			MH_CreateHook(gta_bullet_instance, on_bullet_instance, reinterpret_cast<void**>(&original_bullet_instance));
		if (create_status != MH_OK && create_status != MH_ERROR_ALREADY_CREATED) {
			add_log("failed create silent bullet hook");
			return;
		}

		const MH_STATUS enable_status = MH_EnableHook(gta_bullet_instance);
		if (enable_status != MH_OK && enable_status != MH_ERROR_ENABLED) {
			add_log("failed enable silent bullet hook");
			return;
		}

		bullet_instance_hook_installed.store(true);
		add_log("silent bullet hook ok");
	}

void do_silent_aim_logic(const ExecutionContext& ctx) {
		
		if (!ctx.active) return;
		if (ctx.aim_pos.x == 0 && ctx.aim_pos.y == 0 && ctx.aim_pos.z == 0) return;
		
		if (!std::isfinite(ctx.aim_pos.x) || !std::isfinite(ctx.aim_pos.y) || !std::isfinite(ctx.aim_pos.z)) return;

		if (!IsValidPtr(local.player)) return;
		if (!Game.gameDataReady) return;
		if (!has_current_aim_target(ctx.target)) return;

		const bool autoFire = config::get("aimbot", "silent_aim_auto", 0) != 0;
		const bool silentEnabled = bind_state::is_active(k_silent_aim_bind);
		const bool magicBulletEnabled = config::get("aimbot", "magic_bullet", 0) != 0;
		const bool useBulletHook = bullet_instance_hook_installed.load();

		if (silentEnabled && ctx.mode == 0) {
			if (!useBulletHook) {
				if (magicBulletEnabled) {
					FireWallshotSilent(ctx, autoFire);
				}
				else {
					runtime_debug::last_section = "silent_fallback_bullet";
					run_silent_aim(ctx.aim_pos, ctx.damager_head_pos, autoFire);
				}
			}
		}
	}


// ============================================================================
//  NOCTUA LICENSED BUILD WATERMARK  (auto-generated, do not remove)
//  Inert per-file identifier required by the EULA. It uniquely tags this
//  source file so this program is distinguishable from any other build.
//  Contains no functional logic.
//  file: src/features/aimbot/silent.hpp
// ============================================================================
#ifndef NOCTUA_LICENSE_MARK_5598207abadd0f943bf5bfd79ea883cc
#define NOCTUA_LICENSE_MARK_5598207abadd0f943bf5bfd79ea883cc
namespace noctua_license { namespace mark_5598207abadd0f943bf5bfd79ea883cc {
    inline constexpr unsigned long long kMarkId = 0xfe8fd5c397ce66ffull;
    inline constexpr char kMarkFile[] = "src/features/aimbot/silent.hpp";
    inline constexpr unsigned char kMarkEntropy[] = { 0x9b, 0x06, 0x86, 0x10, 0xc5, 0x0a, 0x23, 0xc6, 0x52, 0x99, 0x77, 0x53, 0xb4, 0xef, 0x7d, 0xd8 };
    inline constexpr unsigned long  kMarkSalts[]  = { 0xb47f3b30ul, 0xfd45b28bul, 0xc8700d62ul, 0xc82dd45bul, 0xee918cc4ul, 0xbfa3d9a6ul, 0x8886dca5ul, 0xd765d51ful };
    [[maybe_unused]] inline unsigned long long mark_digest() noexcept {
        unsigned long long h = 1469598103934665603ull ^ kMarkId;
        for (unsigned char b : kMarkEntropy) { h ^= b; h *= 1099511628211ull; }
        for (unsigned long  s : kMarkSalts)  { h ^= s; h *= 1099511628211ull; }
        return h;
    }
} } // namespace noctua_license::mark_5598207abadd0f943bf5bfd79ea883cc
#endif // NOCTUA_LICENSE_MARK_5598207abadd0f943bf5bfd79ea883cc
