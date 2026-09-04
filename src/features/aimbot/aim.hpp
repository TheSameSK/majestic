#pragma once
#include "config/bind_state.hpp"
#include <atomic>
#include <set>
#include <cstdlib>

namespace config {
    float get(std::string category, std::string key, float defaultVal);
    int get(std::string category, std::string key, int defaultVal);
    bool update(float newValue, std::string category, std::string key, float defaultVal);
    bool update(int newValue, std::string category, std::string key, int defaultVal);
}

namespace game {
    extern vector<pair<CObject*, DataPed>> ped_list;
    extern std::mutex ped_list_mutex;
}

namespace aimbot {
	std::mutex target_mutex;
	std::mutex execution_mutex;

	struct ExecutionContext {
		bool active = false;
		bool can_fire = false; // exec_ctx feeds Silent Line even unarmed; the bullet hook must honor this gate
		int mode = 0;
		Vector3 aim_pos;
		Vector3 damager_head_pos;
		CObject* target = 0;
	} exec_ctx;

	CObject* vector_target = 0;
	DataPed vector_target_data;
	static auto vector_targetTimeout = std::chrono::high_resolution_clock::now();
	static float vector_aimTargetDist = 99999.0f;

	CObject* silent_target = 0;
	DataPed silent_target_data;
	static auto silent_targetTimeout = std::chrono::high_resolution_clock::now();
	static float silent_aimTargetDist = 99999.0f;
	static Vector3 silent_aim_angle;

	CObject* current_target = 0;
	DataPed current_target_data;
	static Vector3 lastTargetVector = Vector3(0, 0, 0);
	static auto targetTimeout = std::chrono::high_resolution_clock::now();
	static float aimTargetDist = 99999.0f;
	static Vector3 target_aim_angle;
	using accurate_shot_fn_t = bool(__fastcall*)(__int64, __int64, Vector3, Vector3, float);
	static inline accurate_shot_fn_t original_get_is_accurate = nullptr;
	static inline std::atomic<bool> accurate_hook_installed = false;
	std::mutex accurate_hook_state_mutex;
	Vector3 accurate_hook_hit_pos = Vector3(0.f, 0.f, 0.f);

	struct vehicleState {
		bool state;
		uint64_t ticksSinceChange;
	};
	vehicleState vehicleAimbot;

	inline bool read_hp(CObject* target, float* outHP) {
		__try {
			if (!IsValidPtr(target) || !outHP) return false;
			*outHP = target->HP;
			return true;
		}
		__except (EXCEPTION_EXECUTE_HANDLER) {
			return false;
		}
	}

	inline bool get_distance_between(Vector3* pos1, Vector3* pos2, float* outDist) {
		if (!pos1 || !pos2 || !outDist) return false;
		*outDist = get_distance(*pos1, *pos2);
		return true;
	}

	inline bool get_model_hash(CObject* ped, DWORD* outHash) {
		__try {
			if (!IsValidPtr(ped) || !outHash) return false;
			auto modelInfo = ped->ModelInfo();
			if (!IsValidPtr(modelInfo)) return false;
			*outHash = modelInfo->GetHash();
			return true;
		}
		__except (EXCEPTION_EXECUTE_HANDLER) {
			return false;
		}
	}

	inline bool is_in_vehicle(CObject* ped, bool* outResult) {
		__try {
			if (!IsValidPtr(ped) || !outResult) return false;
			*outResult = ped->IsInVehicle();
			return true;
		}
		__except (EXCEPTION_EXECUTE_HANDLER) {
			return false;
		}
	}

	inline bool get_weapon_hash(CObject* ped, DWORD* outHash) {
		__try {
			if (!IsValidPtr(ped) || !outHash) return false;
			CWeaponManager* wep = ped->weapon();
			if (!IsValidPtr(wep) || !IsValidPtr(wep->_WeaponInfo)) return false;
			*outHash = *reinterpret_cast<DWORD*>(reinterpret_cast<uintptr_t>(wep->_WeaponInfo) + 0x10);
			return true;
		}
		__except (EXCEPTION_EXECUTE_HANDLER) {
			return false;
		}
	}

	inline bool get_local_player_handle(int* outHandle) {
		__try {
			if (!outHandle) return false;
			*outHandle = hacks::local_ped_handle();
			return *outHandle != 0;
		}
		__except (EXCEPTION_EXECUTE_HANDLER) {
			return false;
		}
	}

	inline bool get_current_weapon_for_shot(CObject* ped, native::type::hash* outHash, int* outDamage) {
		__try {
			if (!IsValidPtr(ped) || !outHash || !outDamage) return false;

			CWeaponManager* wep = ped->weapon();
			if (!IsValidPtr(wep) || !IsValidPtr(wep->_WeaponInfo)) return false;

			CWeaponInfo* info = wep->_WeaponInfo;
			const uintptr_t info_ptr = reinterpret_cast<uintptr_t>(info);
			if (info_ptr <= 0x10000 || info_ptr >= 0x7FFFFFFFFFFF) return false;

			const DWORD hash = *reinterpret_cast<DWORD*>(info_ptr + 0x10);
			if (hash == 0 || hash == 0xA2719263) return false;

			const float damage = info->Damage;
			if (!std::isfinite(damage) || damage <= 0.f) return false;

			const int rounded_damage = static_cast<int>(std::round(damage));
			if (rounded_damage <= 0) return false;

			*outHash = static_cast<native::type::hash>(hash);
			*outDamage = rounded_damage;
			return true;
		}
		__except (EXCEPTION_EXECUTE_HANDLER) {
			NOCTUA_RUNTIME_LOG("SEH: weapon shot state 0x%08X", GetExceptionCode());
			return false;
		}
	}

	inline bool get_velocity(CObject* ped, Vector3* outVel) {
		__try {
			if (!IsValidPtr(ped) || !outVel) return false;
			*outVel = ped->v3Velocity;
			return true;
		}
		__except (EXCEPTION_EXECUTE_HANDLER) {
			return false;
		}
	}

	inline bool get_vehicle_velocity(CObject* ped, Vector3* outVel) {
		__try {
			if (!IsValidPtr(ped) || !outVel) return false;
			CVehicle* veh = ped->vehicle();
			if (!IsValidPtr(veh)) return false;
			*outVel = veh->v3Velocity;
			return true;
		}
		__except (EXCEPTION_EXECUTE_HANDLER) {
			return false;
		}
	}

	inline bool get_position(CObject* ped, Vector3* outPos) {
		__try {
			if (!IsValidPtr(ped) || !outPos) return false;
			*outPos = ped->fPosition;
			return true;
		}
		__except (EXCEPTION_EXECUTE_HANDLER) {
			return false;
		}
	}

	static inline DWORD64 lastDamagerShot = 0;
	static inline DWORD64 lastSilentWallshotShot = 0;
	static inline int lastSilentClipAmmo = -1;
	static inline native::type::hash lastSilentClipWeapon = 0;
	static inline std::atomic<bool> bullet_instance_hook_installed = false;
	std::mutex bullet_instance_hook_mutex;

	inline int GetConfiguredBindKey(const char* category, const char* legacy_key, const char* binds_key) {
		int key = config::get("binds", binds_key, 0);
		if (key != 0) return key;
		return config::get(category, legacy_key, 0);
	}

	inline bool HasConfiguredBind(const char* category, const char* legacy_key, const char* binds_key) {
		return GetConfiguredBindKey(category, legacy_key, binds_key) != 0;
	}

	static const bind_state::bind_config k_vector_aim_bind {
		"aimbot",
		"vector_enable",
		"aim_key",
		"vector_aim_key",
		"aim_key_mode",
		"vector_aim_mode"
	};

	static const bind_state::bind_config k_silent_aim_bind {
		"aimbot",
		"silent_enable",
		"silent_aim_key",
		"silent_aim_key",
		"silent_aim_key_mode",
		"silent_aim_mode"
	};

	static const bind_state::bind_config k_damager_bind {
		"aimbot",
		"damager",
		"damager_key",
		"damager_key",
		"damager_key_mode",
		"damager_mode"
	};

	static const bind_state::bind_config k_triggerbot_bind {
		"aimbot",
		"triggerbot",
		"triggerbot_key",
		"triggerbot_key",
		"triggerbot_key_mode",
		"triggerbot_mode"
	};
	inline bool IsBoneDataValid(const Vector3& bone);

inline bool IsTargetValid(CObject* target) {
		if (!target) return false;
if (!IsValidPtr(target)) return false;
		float hp = 0;
		if (!read_hp(target, &hp)) return false;
		if (hp <= 0) return false;
		return true;
	}

	inline bool IsTargetDataDead(const DataPed& data) {
		return data.altv_is_dead;
	}

	inline bool should_ignore_marked_target(const DataPed& data) {
		if (config::get("aimbot", "ignore_friends", 1) && data.is_friend) return true;
		if (config::get("aimbot", "ignore_family", 0) && player_marks::is_family(data.altv_static_id)) return true;
		if (config::get("aimbot", "ignore_fraction", 0) && player_marks::is_fraction(data.altv_static_id)) return true;
		if (config::get("aimbot", "ignore_other_families", 0) && data.altv_family_id > 0) {
			const std::map<std::string, std::string> ignored_families = config::get("aimbot", "ignore_families", std::map<std::string, std::string>{});
			const auto it = ignored_families.find(std::to_string(data.altv_family_id));
			if (it != ignored_families.end() && it->second == "1") return true;
		}
		if (config::get("aimbot", "ignore_other_fractions", 0) && data.altv_fraction_id > 0) {
			const std::map<std::string, std::string> ignored_fractions = config::get("aimbot", "ignore_fractions", std::map<std::string, std::string>{});
			const auto it = ignored_fractions.find(std::to_string(data.altv_fraction_id));
			if (it != ignored_fractions.end() && it->second == "1") return true;
		}
		return false;
	}

	inline bool has_current_aim_target_data(const DataPed& data) {
		return data.has_current_ws_data;
	}

	inline bool refresh_current_aim_target_data(CObject* target, DataPed* out_data) {
		if (!target || !out_data) return false;

		std::lock_guard<std::mutex> lock(game::ped_list_mutex);
		for (const auto& entry : game::ped_list) {
			if (entry.first != target) continue;
			if (!has_current_aim_target_data(entry.second)) return false;
			*out_data = entry.second;
			return true;
		}

		return false;
	}

	inline bool has_current_aim_target(CObject* target) {
		DataPed data = {};
		return refresh_current_aim_target_data(target, &data);
	}

	inline bool IsBoneDataValid(const Vector3& bone) {
		if (!std::isfinite(bone.x) || !std::isfinite(bone.y) || !std::isfinite(bone.z)) return false;
		if (bone.x == 0.f && bone.y == 0.f && bone.z == 0.f) return false;
		return true;
	}

	Vector3 get_nearest_bone(DataPed data) {
		Vector3 aim = data.bones.HEAD;
		if (!IsBoneDataValid(aim)) {
			aim = data.bones.SPINE2;
			if (!IsBoneDataValid(aim)) {
				return Vector3(0, 0, 0);
			}
		}
		
		PlayerBones bones = data.bones;
		Vector3 bone_positions[] = {
			bones.HEAD,
			bones.NECK,
			bones.RIGHT_HAND,
			bones.RIGHT_FOREARM,
			bones.RIGHT_UPPER_ARM,
			bones.RIGHT_CLAVICLE,
			bones.LEFT_HAND,
			bones.LEFT_FOREARM,
			bones.LEFT_UPPER_ARM,
			bones.LEFT_CLAVICLE,
			bones.PELVIS,
			bones.SPINE_ROOT,
			bones.SPINE0,
			bones.SPINE1,
			bones.SPINE2,
			bones.SPINE3,
			bones.RIGHT_TOE,
			bones.RIGHT_FOOT,
			bones.RIGHT_CALF,
			bones.RIGHT_THIGH,
			bones.LEFT_TOE,
			bones.LEFT_FOOT,
			bones.LEFT_CALF,
			bones.LEFT_THIGH
		};
		float closest_distance = 999.f;
		int arrSize = sizeof(bone_positions) / sizeof(bone_positions[0]);
		

		for (size_t i = 0; i < arrSize; i++) {
			if (!IsBoneDataValid(bone_positions[i])) continue;
			
			ImVec2 screen_point;
			if (!WorldToScreen(bone_positions[i], &screen_point)) continue;
			float dist = screen_distance(Game.screen.x / 2, Game.screen.y / 2, screen_point.x, screen_point.y);
			if (closest_distance > dist) {
				closest_distance = dist;
				aim = bone_positions[i];
			}
		}

		return aim;
	}

	inline bool is_weapon_in_list(DWORD hash, const DWORD* values, size_t count) {
		for (size_t i = 0; i < count; ++i) {
			if (values[i] == hash) return true;
		}
		return false;
	}

	std::string get_weapon_category_prefix() {
		if (!IsValidPtr(local.player)) return "";
		DWORD wh = 0;
		if (!get_weapon_hash(local.player, &wh) || wh == 0) return "";

		static constexpr DWORD snipers[] = {
			weapon_hashes::WEAPON_SNIPERRIFLE, weapon_hashes::WEAPON_HEAVYSNIPER,
			weapon_hashes::WEAPON_HEAVYSNIPER_MK2, weapon_hashes::WEAPON_MARKSMANRIFLE,
			weapon_hashes::WEAPON_MARKSMANRIFLE_MK2
		};
		if (is_weapon_in_list(wh, snipers, std::size(snipers))) return "sniper_";

		static constexpr DWORD rifles[] = {
			weapon_hashes::WEAPON_ASSAULTRIFLE, weapon_hashes::WEAPON_ASSAULTRIFLE_MK2,
			weapon_hashes::WEAPON_CARBINERIFLE, weapon_hashes::WEAPON_CARBINERIFLE_MK2,
			weapon_hashes::WEAPON_ADVANCEDRIFLE, weapon_hashes::WEAPON_SPECIALCARBINE,
			weapon_hashes::WEAPON_SPECIALCARBINE_MK2, weapon_hashes::WEAPON_BULLPUPRIFLE,
			weapon_hashes::WEAPON_BULLPUPRIFLE_MK2, weapon_hashes::WEAPON_COMPACTRIFLE,
			weapon_hashes::WEAPON_MILITARYRIFLE, weapon_hashes::WEAPON_HEAVYRIFLE
		};
		if (is_weapon_in_list(wh, rifles, std::size(rifles))) return "rifle_";

		static constexpr DWORD shotguns[] = {
			weapon_hashes::WEAPON_PUMPSHOTGUN, weapon_hashes::WEAPON_PUMPSHOTGUN_MK2,
			weapon_hashes::WEAPON_SAWNOFFSHOTGUN, weapon_hashes::WEAPON_ASSAULTSHOTGUN,
			weapon_hashes::WEAPON_BULLPUPSHOTGUN, weapon_hashes::WEAPON_MUSKET,
			weapon_hashes::WEAPON_HEAVYSHOTGUN, weapon_hashes::WEAPON_DBSHOTGUN,
			weapon_hashes::WEAPON_AUTOSHOTGUN, weapon_hashes::WEAPON_COMBATSHOTGUN
		};
		if (is_weapon_in_list(wh, shotguns, std::size(shotguns))) return "sg_";

		static constexpr DWORD smgs[] = {
			weapon_hashes::WEAPON_MICROSMG, weapon_hashes::WEAPON_SMG,
			weapon_hashes::WEAPON_SMG_MK2, weapon_hashes::WEAPON_ASSAULTSMG,
			weapon_hashes::WEAPON_COMBATPDW, weapon_hashes::WEAPON_MACHINEPISTOL,
			weapon_hashes::WEAPON_MINISMG
		};
		if (is_weapon_in_list(wh, smgs, std::size(smgs))) return "smg_";

		static constexpr DWORD pistols[] = {
			weapon_hashes::WEAPON_PISTOL, weapon_hashes::WEAPON_PISTOL_MK2,
			weapon_hashes::WEAPON_COMBATPISTOL, weapon_hashes::WEAPON_APPISTOL,
			weapon_hashes::WEAPON_STUNGUN, weapon_hashes::WEAPON_PISTOL50,
			weapon_hashes::WEAPON_SNSPISTOL, weapon_hashes::WEAPON_SNSPISTOL_MK2,
			weapon_hashes::WEAPON_HEAVYPISTOL, weapon_hashes::WEAPON_VINTAGEPISTOL,
			weapon_hashes::WEAPON_FLAREGUN, weapon_hashes::WEAPON_MARKSMANPISTOL,
			weapon_hashes::WEAPON_REVOLVER, weapon_hashes::WEAPON_REVOLVER_MK2,
			weapon_hashes::WEAPON_DOUBLEACTION, weapon_hashes::WEAPON_CERAMICPISTOL,
			weapon_hashes::WEAPON_NAVYREVOLVER, weapon_hashes::WEAPON_GADGETPISTOL
		};
		if (is_weapon_in_list(wh, pistols, std::size(pistols))) return "pistol_";

		return "";
	}


	int get_aim_config_int(const std::string& key, int def) {
		std::string pfx = get_weapon_category_prefix();
		if (!pfx.empty()) {
			int val = config::get("aimbot", pfx + key, -999);
			if (val != -999) return val;
		}
		return config::get("aimbot", key, def);
	}

	float get_aim_config_float(const std::string& key, float def) {
		std::string pfx = get_weapon_category_prefix();
		if (!pfx.empty()) {
			float val = config::get("aimbot", pfx + key, -999.f);
			if (val != -999.f) return val;
		}
		return config::get("aimbot", key, def);
	}
	Vector3 get_cached_bone_by_id(const DataPed& data, const std::string& bone_id) {
		if (bone_id == "head") return data.bones.HEAD;
		if (bone_id == "neck") return data.bones.NECK;
		if (bone_id == "chest") return data.bones.SPINE2;
		if (bone_id == "spine") return data.bones.SPINE1;
		if (bone_id == "pelvis") return data.bones.PELVIS;
		return Vector3();
	}

	Vector3 get_live_bone_by_id(CObject* ped, const std::string& bone_id) {
		if (!ped || !IsValidPtr(ped)) return Vector3();
		if (bone_id == "head") return GetBonePosition(ped, HEAD);
		if (bone_id == "neck") return GetBonePosition(ped, NECK);
		if (bone_id == "chest") return GetBonePosition(ped, SPINE2);
		if (bone_id == "spine") return GetBonePosition(ped, SPINE1);
		if (bone_id == "pelvis") return GetBonePosition(ped, PELVIS);
		return Vector3();
	}

	void append_legacy_bones(int legacy_value, std::vector<std::string>& selected_bones) {
		if (legacy_value <= 0) {
			selected_bones.push_back("head");
			selected_bones.push_back("neck");
			selected_bones.push_back("chest");
			selected_bones.push_back("spine");
			selected_bones.push_back("pelvis");
			return;
		}

		if (legacy_value == 1) {
			selected_bones.push_back("head");
			return;
		}

		if (legacy_value == 2) {
			selected_bones.push_back("neck");
			return;
		}

		if (legacy_value == 3) {
			selected_bones.push_back("chest");
			return;
		}

		if (legacy_value == 4) {
			selected_bones.push_back("spine");
			return;
		}

		if (legacy_value == 5) {
			selected_bones.push_back("pelvis");
			return;
		}

		selected_bones.push_back("head");
	}

	std::vector<std::string> get_selected_bones(const char* multi_key, int legacy_value) {
		std::vector<std::string> selected_bones;
		const std::map<std::string, std::string> saved_bones =
			config::get("aimbot", multi_key, std::map<std::string, std::string>{});

		static const char* bone_order[] = { "head", "neck", "chest", "spine", "pelvis" };
		for (const char* bone_id : bone_order) {
			auto it = saved_bones.find(bone_id);
			if (it != saved_bones.end() && it->second == "1") {
				selected_bones.emplace_back(bone_id);
			}
		}

		if (selected_bones.empty()) {
			append_legacy_bones(legacy_value, selected_bones);
		}

		if (selected_bones.empty()) {
			selected_bones.push_back("head");
		}

		return selected_bones;
	}

	Vector3 pick_best_cached_bone(const DataPed& data, const std::vector<std::string>& selected_bones) {
		Vector3 best_bone = Vector3();
		float best_distance = 999999.f;

		for (const std::string& bone_id : selected_bones) {
			Vector3 bone_pos = get_cached_bone_by_id(data, bone_id);
			if (!IsBoneDataValid(bone_pos)) continue;

			ImVec2 screen_point;
			if (WorldToScreen(bone_pos, &screen_point)) {
				float distance = screen_distance(Game.screen.x / 2, Game.screen.y / 2, screen_point.x, screen_point.y);
				if (!IsBoneDataValid(best_bone) || distance < best_distance) {
					best_bone = bone_pos;
					best_distance = distance;
				}
				continue;
			}

			if (!IsBoneDataValid(best_bone)) {
				best_bone = bone_pos;
			}
		}

		return best_bone;
	}

	Vector3 pick_best_live_bone(CObject* ped, const std::vector<std::string>& selected_bones) {
		Vector3 best_bone = Vector3();
		float best_distance = 999999.f;

		for (const std::string& bone_id : selected_bones) {
			Vector3 bone_pos = get_live_bone_by_id(ped, bone_id);
			if (!IsBoneDataValid(bone_pos)) continue;

			ImVec2 screen_point;
			if (WorldToScreen(bone_pos, &screen_point)) {
				float distance = screen_distance(Game.screen.x / 2, Game.screen.y / 2, screen_point.x, screen_point.y);
				if (!IsBoneDataValid(best_bone) || distance < best_distance) {
					best_bone = bone_pos;
					best_distance = distance;
				}
				continue;
			}

			if (!IsBoneDataValid(best_bone)) {
				best_bone = bone_pos;
			}
		}

		return best_bone;
	}

	Vector3 pick_random_cached_bone(const DataPed& data, const std::vector<std::string>& selected_bones) {
		std::vector<Vector3> valid_bones;

		for (const std::string& bone_id : selected_bones) {
			Vector3 bone_pos = get_cached_bone_by_id(data, bone_id);
			if (IsBoneDataValid(bone_pos)) valid_bones.push_back(bone_pos);
		}

		if (valid_bones.empty()) return Vector3();
		return valid_bones[rand() % valid_bones.size()];
	}

	Vector3 pick_random_live_bone(CObject* ped, const std::vector<std::string>& selected_bones) {
		std::vector<Vector3> valid_bones;

		for (const std::string& bone_id : selected_bones) {
			Vector3 bone_pos = get_live_bone_by_id(ped, bone_id);
			if (IsBoneDataValid(bone_pos)) valid_bones.push_back(bone_pos);
		}

		if (valid_bones.empty()) return Vector3();
		return valid_bones[rand() % valid_bones.size()];
	}

	Vector3 pick_silent_cached_bone(const DataPed& data, const std::vector<std::string>& selected_bones) {
		if (config::get("aimbot", "silent_bone_selection", 0) == 1) {
			return pick_random_cached_bone(data, selected_bones);
		}

		return pick_best_cached_bone(data, selected_bones);
	}

	Vector3 pick_silent_live_bone(CObject* ped, const std::vector<std::string>& selected_bones) {
		if (config::get("aimbot", "silent_bone_selection", 0) == 1) {
			return pick_random_live_bone(ped, selected_bones);
		}

		return pick_best_live_bone(ped, selected_bones);
	}

	Vector3 get_aimbone_for_vector(DataPed data) {
		const int legacy_value = get_aim_config_int("vector_bone", config::get("aimbot", "vector_aim_bone", 0));
		return pick_best_cached_bone(data, get_selected_bones("vector_aim_bones", legacy_value));
	}

	Vector3 get_aimbone_for_silent(DataPed data) {
		const int legacy_value = get_aim_config_int("silent_bone", config::get("aimbot", "silent_aim_bone", 0));
		return pick_silent_cached_bone(data, get_selected_bones("silent_aim_bones", legacy_value));
	}

	Vector3 get_fresh_bone_for_silent(CObject* ped) {
		if (!ped || !IsValidPtr(ped)) return Vector3();
		const int legacy_value = get_aim_config_int("silent_bone", config::get("aimbot", "silent_aim_bone", 0));
		return pick_silent_live_bone(ped, get_selected_bones("silent_aim_bones", legacy_value));
	}

	inline bool get_fresh_silent_bone(CObject* ped, Vector3* outBone) {
		__try {
			if (!outBone) return false;
			*outBone = get_fresh_bone_for_silent(ped);
			return IsBoneDataValid(*outBone);
		}
		__except (EXCEPTION_EXECUTE_HANDLER) {
			return false;
		}
	}

	inline bool get_fresh_head_bone(CObject* ped, Vector3* outBone) {
		__try {
			if (!ped || !IsValidPtr(ped) || !outBone) return false;
			*outBone = GetBonePosition(ped, HEAD);
			return IsBoneDataValid(*outBone);
		}
		__except (EXCEPTION_EXECUTE_HANDLER) {
			return false;
		}
	}

	Vector3 get_aimbone_complex(Vector3 current, DataPed data) {
		return get_nearest_bone(data);
	}

void get_vector_target(CPlayerAngles* cam, float radius) {
		if (!Game.renderReady || Game.screen.x < 100) return;
		bool target_acquired = false;
		if (!IsValidPtr(cam) || !IsValidPtr(local.player)) {
			std::lock_guard<std::mutex> lock(target_mutex);
			vector_target = 0;
			vector_aimTargetDist = 99999.f;
			return;
		}

		CObject* closest = 0;
		DataPed closest_data = { 0 };
		float closest_dist = 99999.0f;

		{
			std::lock_guard<std::mutex> lock(game::ped_list_mutex);
			for (auto it = game::ped_list.begin(); it != game::ped_list.end(); ++it) {
				CObject* ped = it->first;
				if (!ped || !IsValidPtr(ped)) continue;
				
				DataPed data = it->second;
				
				if (!IsTargetValid(ped)) continue;
				if (IsTargetDataDead(data)) continue;
				if (local.player == ped) continue;
				
				Vector3 pedPos, localPos;
				if (!get_position(ped, &pedPos)) continue;
				if (!get_position(local.player, &localPos)) continue;
				
				float dist_to_player = 0.f;
				if (!get_distance_between(&localPos, &pedPos, &dist_to_player)) continue;
				
				if (dist_to_player > config::get("hack", "max_range", 1000.f)) continue;
				
				DWORD hash = 0;
				if (!get_model_hash(ped, &hash)) continue;
				
				if (!game::isValidPlayer(hash, ped)) continue;
				if (!has_current_aim_target_data(data)) continue;

				Vector3 aimpos = get_aimbone_for_vector(data);
				if (!IsBoneDataValid(aimpos)) continue;

				if (data.group.ally) continue;
				if (should_ignore_marked_target(data)) continue;


				ImVec2 aim2d;
				if (!WorldToScreen(aimpos, &aim2d)) continue;

				float Distance2d = screen_distance(Game.screen.x / 2, Game.screen.y / 2, aim2d.x, aim2d.y);

				CObject* current_vector_target_local = 0;
				{
					std::lock_guard<std::mutex> tlock(target_mutex);
					current_vector_target_local = vector_target;
				}

				if ((Distance2d > config::get("aimbot", "aim_stop_radius", 0.f)) && (radius > Distance2d)) {
					if ((closest_dist > Distance2d) || (current_vector_target_local == ped)) {
						closest_dist = Distance2d;
						target_acquired = true;
						closest = ped;
						closest_data = data;
					}
				}
			}
		}

		if (closest && target_acquired) {
			auto now = std::chrono::high_resolution_clock::now();
			
			std::lock_guard<std::mutex> lock(target_mutex);
			auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(now - vector_targetTimeout).count();

			if (closest != vector_target || vector_target == 0) {
				if (dur > config::get("aimbot", "target_timeout", 0.f)) {
					vector_target = closest;
					vector_target_data = closest_data;
					vector_aimTargetDist = closest_dist;
					vector_targetTimeout = std::chrono::high_resolution_clock::now();
					return;
				}
			}
		}
		
		if (!target_acquired) {
			std::lock_guard<std::mutex> lock(target_mutex);
			vector_target = 0;
			vector_aimTargetDist = 99999.f;
		}
	}

	void get_silent_target(CPlayerAngles* cam, float radius) {
		if (!Game.renderReady || Game.screen.x < 100) return;
		bool target_acquired = false;
		if (!IsValidPtr(cam) || !IsValidPtr(local.player)) {
			std::lock_guard<std::mutex> lock(target_mutex);
			silent_target = 0;
			silent_aimTargetDist = 99999.f;
			current_target = 0;
			return;
		}

		CObject* closest = 0;
		DataPed closest_data = { 0 };
		float closest_dist = 99999.0f;

		{
			std::lock_guard<std::mutex> lock(game::ped_list_mutex);
			for (auto it = game::ped_list.begin(); it != game::ped_list.end(); ++it) {
				CObject* ped = it->first;
				if (!ped || !IsValidPtr(ped)) continue;
				
				DataPed data = it->second;
				
				if (!IsTargetValid(ped)) continue;
				if (IsTargetDataDead(data)) continue;
				if (local.player == ped) continue;
				
				Vector3 pedPos, localPos;
				if (!get_position(ped, &pedPos)) continue;
				if (!get_position(local.player, &localPos)) continue;
				
				float dist_to_player = 0.f;
				if (!get_distance_between(&localPos, &pedPos, &dist_to_player)) continue;
				
				if (dist_to_player > config::get("hack", "max_range", 1000.f)) continue;
				
				DWORD hash = 0;
				if (!get_model_hash(ped, &hash)) continue;
				
				if (!game::isValidPlayer(hash, ped)) continue;
				if (!has_current_aim_target_data(data)) continue;

				Vector3 aimpos = get_aimbone_for_silent(data);
				if (!IsBoneDataValid(aimpos)) continue;

				if (data.group.ally) continue;
				if (should_ignore_marked_target(data)) continue;


				ImVec2 aim2d;
				if (!WorldToScreen(aimpos, &aim2d)) continue;

				float Distance2d = screen_distance(Game.screen.x / 2, Game.screen.y / 2, aim2d.x, aim2d.y);

				CObject* current_silent_target_local = 0;
				{
					std::lock_guard<std::mutex> tlock(target_mutex);
					current_silent_target_local = silent_target;
				}

				if ((Distance2d > config::get("aimbot", "aim_stop_radius", 0.f)) && (radius > Distance2d)) {
					if ((closest_dist > Distance2d) || (current_silent_target_local == ped)) {
						closest_dist = Distance2d;
						target_acquired = true;
						closest = ped;
						closest_data = data;
					}
				}
			}
		}

		if (closest && target_acquired) {
			auto now = std::chrono::high_resolution_clock::now();
			
			std::lock_guard<std::mutex> lock(target_mutex);
			auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(now - silent_targetTimeout).count();

			if (closest != silent_target || silent_target == 0) {
				if (dur > config::get("aimbot", "target_timeout", 0.f)) {
					silent_target = closest;
					silent_target_data = closest_data;
					silent_aimTargetDist = closest_dist;
					silent_targetTimeout = std::chrono::high_resolution_clock::now();
					
					current_target = silent_target;
					current_target_data = silent_target_data;
					return;
				}
			}
		}
		
		if (!target_acquired) {
			std::lock_guard<std::mutex> lock(target_mutex);
			silent_target = 0;
			silent_aimTargetDist = 99999.f;
			current_target = 0;
		}
	}

	bool acquire_silent_shot_target(CObject** out_target, DataPed* out_data) {
		if (out_target) *out_target = 0;
		if (out_data) *out_data = {};

		if (!Game.renderReady || Game.screen.x < 100) {
			std::lock_guard<std::mutex> lock(target_mutex);
			silent_target = 0;
			silent_aimTargetDist = 99999.f;
			target_aim_angle = { 0 };
			current_target = 0;
			current_target_data = {};
			return false;
		}

		if (!IsValidPtr(local.player)) {
			return false;
		}

		const float radius = config::get("aimbot", "silent_radius", 50.f);

		CObject* closest = 0;
		DataPed closest_data = {};
		float closest_dist = 99999.f;
		WorldToScreenSnapshot w2s_view{};
		if (!CaptureWorldToScreenSnapshot(&w2s_view)) {
			return false;
		}

		{
			std::lock_guard<std::mutex> lock(game::ped_list_mutex);
			for (auto& entry : game::ped_list) {
				CObject* ped = entry.first;
				if (!ped || !IsValidPtr(ped)) continue;
				if (ped == local.player) continue;
				if (!IsTargetValid(ped)) continue;

				DataPed data = entry.second;
				if (IsTargetDataDead(data)) continue;

				Vector3 ped_pos;
				Vector3 local_pos;
				if (!get_position(ped, &ped_pos)) continue;
				if (!get_position(local.player, &local_pos)) continue;

				float dist_to_player = 0.f;
				if (!get_distance_between(&local_pos, &ped_pos, &dist_to_player)) continue;
				if (dist_to_player > config::get("hack", "max_range", 1000.f)) continue;

				DWORD hash = 0;
				if (!get_model_hash(ped, &hash)) continue;
				if (!game::isValidPlayer(hash, ped)) continue;
				if (!has_current_aim_target_data(data)) continue;

				if (data.group.ally) continue;
				if (should_ignore_marked_target(data)) continue;


				Vector3 aimpos = get_aimbone_for_silent(data);
				if (!IsBoneDataValid(aimpos)) continue;

				ImVec2 aim2d;
				if (!WorldToScreenMatrix(w2s_view, aimpos, &aim2d)) continue;

				float distance2d = screen_distance(Game.screen.x / 2, Game.screen.y / 2, aim2d.x, aim2d.y);
				if (distance2d > radius) continue;

				if (distance2d < closest_dist) {
					closest = ped;
					closest_data = data;
					closest_dist = distance2d;
				}
			}
		}

		{
			std::lock_guard<std::mutex> lock(target_mutex);
			silent_target = closest;
			silent_target_data = closest_data;
			silent_aimTargetDist = closest ? closest_dist : 99999.f;
			target_aim_angle = closest ? get_aimbone_for_silent(closest_data) : Vector3(0, 0, 0);
			current_target = closest;
			current_target_data = closest_data;
		}

		if (!closest) {
			return false;
		}

		if (out_target) *out_target = closest;
		if (out_data) *out_data = closest_data;
		return true;
	}

	void get_new_target_with_radius(CPlayerAngles* cam, float radius) {
		if (!Game.renderReady || Game.screen.x < 100) return;
		bool target_accquired = false;
		if (!IsValidPtr(cam) || !IsValidPtr(local.player)) {
			current_target = 0;
			aimTargetDist = 9999.f;
			target_aim_angle = { 0 };
			return;
		}

		Vector3 CrosshairPos = cam->m_cam_pos;
		CObject* closest = 0;
		DataPed closest_data = { 0 };

		std::lock_guard<std::mutex> lock(game::ped_list_mutex);
		for (auto it = game::ped_list.begin(); it != game::ped_list.end(); ++it) {
			CObject* ped = it->first;
			if (!ped || !IsValidPtr(ped)) continue;
			
			DataPed data = it->second;
			
			if (local.player == ped) continue;
			if (IsTargetDataDead(data)) continue;
			
			float hp = 0;
			if (!read_hp(ped, &hp) || hp <= 0) continue;
			
			Vector3 pedPos, localPos;
			if (!get_position(ped, &pedPos)) continue;
			if (!get_position(local.player, &localPos)) continue;
			
			float dist = 0;
			if (!get_distance_between(&localPos, &pedPos, &dist)) continue;
			if (dist > config::get("hack", "max_range", 1000.f)) continue;
			
			DWORD hash = 0;
			if (!get_model_hash(ped, &hash)) continue;
			if (!game::isValidPlayer(hash, ped)) continue;
			if (!has_current_aim_target_data(data)) continue;

			Vector3 aimpos = data.bones.HEAD;
			if (!IsBoneDataValid(aimpos)) continue;

			if (data.group.ally) continue;
			if (should_ignore_marked_target(data)) continue;


			ImVec2 aim2d;
			if (!WorldToScreen(aimpos, &aim2d)) continue;

			float Distance2d = screen_distance(Game.screen.x / 2, Game.screen.y / 2, aim2d.x, aim2d.y);

			if (
				(Distance2d > config::get("aimbot", "aim_stop_radius", 0.f)) &&
				(radius > Distance2d)
				) {
				if ((aimTargetDist > Distance2d) || (current_target == ped)) {
					aimTargetDist = Distance2d;
					target_accquired = true;
					closest = ped;
					closest_data = data;
				}
			}
		}

		if (closest) {
			auto now = std::chrono::high_resolution_clock::now();
			auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(now - targetTimeout).count();

			if (closest != current_target) {
				if (dur > config::get("aimbot", "target_timeout", 0.f)) {
					current_target = closest;
					current_target_data = closest_data;
					targetTimeout = std::chrono::high_resolution_clock::now();
					return;
				}
			}
			if (current_target == 0) {
				if (dur > config::get("aimbot", "target_timeout", 0.f)) {
					current_target = closest;
					current_target_data = closest_data;
					targetTimeout = std::chrono::high_resolution_clock::now();
					return;
				}
			}
		}

		if (!target_accquired) {
			current_target = 0;
			aimTargetDist = 9999.f;
			target_aim_angle = { 0 };
		}
	}

	void get_new_target(CPlayerAngles* cam) {
		bool target_accquired = false;
		if (!IsValidPtr(cam) || !IsValidPtr(local.player)) {
			current_target = 0;
			aimTargetDist = 9999.f;
			target_aim_angle = { 0 };
			return;
		}

		Vector3 CrosshairPos = cam->m_cam_pos;
		CObject* closest = 0;
		DataPed closest_data = { 0 };

		std::lock_guard<std::mutex> lock(game::ped_list_mutex);
		for (auto it = game::ped_list.begin(); it != game::ped_list.end(); ++it) {
			CObject* ped = it->first;
			if (!ped || !IsValidPtr(ped)) continue;
			
			DataPed data = it->second;
			
			if (local.player == ped) continue;
			if (IsTargetDataDead(data)) continue;
			
			float hp = 0;
			if (!read_hp(ped, &hp) || hp <= 0) continue;
			
			Vector3 pedPos, localPos;
			if (!get_position(ped, &pedPos)) continue;
			if (!get_position(local.player, &localPos)) continue;
			
			float dist = 0;
			if (!get_distance_between(&localPos, &pedPos, &dist)) continue;
			if (dist > config::get("hack", "max_range", 1000.f)) continue;
			
			DWORD hash = 0;
			if (!get_model_hash(ped, &hash)) continue;
			if (!game::isValidPlayer(hash, ped)) continue;
			if (!has_current_aim_target_data(data)) continue;

			Vector3 aimpos = config::get("aimbot", "aim_bone", 0) == 0 ? data.bones.HEAD : config::get("aimbot", "aim_bone", 0) == 1 ? data.bones.NECK : data.bones.SPINE2;

			if (config::get("aimbot", "aim_bone", 0) == 3) {
				aimpos = get_aimbone_complex(aimpos, data);
			}
			
			if (!IsBoneDataValid(aimpos)) continue;

			if (data.group.ally) continue;
			if (should_ignore_marked_target(data)) continue;


			ImVec2 aim2d;
			if (!WorldToScreen(aimpos, &aim2d)) continue;

			float Distance2d = screen_distance(Game.screen.x / 2, Game.screen.y / 2, aim2d.x, aim2d.y);

			if (
				(Distance2d > config::get("aimbot", "aim_stop_radius", 0.f)) &&
				(config::get("aimbot", "aim_radius", 50.f) > Distance2d)
				) {
				if ((aimTargetDist > Distance2d) || (current_target == ped)) {
					aimTargetDist = Distance2d;
					target_accquired = true;
					closest = ped;
					closest_data = data;
				}
			}
		}

		if (closest) {
			auto now = std::chrono::high_resolution_clock::now();
			auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(now - targetTimeout).count();

			if (closest != current_target) {
				if (dur > config::get("aimbot", "target_timeout", 0.f)) {
					current_target = closest;
					current_target_data = closest_data;
					targetTimeout = std::chrono::high_resolution_clock::now();
					return;
				}
			}
			if (current_target == 0) {
				if (dur > config::get("aimbot", "target_timeout", 0.f)) {
					current_target = closest;
					current_target_data = closest_data;
					targetTimeout = std::chrono::high_resolution_clock::now();
					return;
				}
			}
		}

		if (!target_accquired) {
			current_target = 0;
			aimTargetDist = 9999.f;
			target_aim_angle = { 0 };
		}
	}


inline bool run_damager(Vector3 aim_pos, Vector3 damager_aim_pos, bool autoFire);
#include "silent.hpp"
#include "damager.hpp"
#include "triggerbot.hpp"
void aimbot_sync_execution() {
	}

	void tick_vector_aim() {
		if (!Game.renderReady || Game.screen.x < 100) return;
		CPlayerAngles* cam = Game.getCam();
		if (!bind_state::is_active(k_vector_aim_bind)) {
			std::lock_guard<std::mutex> lock(target_mutex);
			vector_target = 0;
			return;
		}
		if (!IsValidPtr(cam) || !IsValidPtr(local.player)) return;

		float vector_radius = get_aim_config_float("vector_fov", config::get("aimbot", "vector_radius", 50.f));

		auto old_target = vector_target;
		get_vector_target(cam, vector_radius);

		if (!IsValidPtr(vector_target)) return;
		if (vector_target != old_target && old_target != 0) return;
		if (get_distance(local.player->fPosition, vector_target->fPosition) > config::get("hack", "max_range", 1000.f)) return;

		DataPed data = {};
		if (!refresh_current_aim_target_data(vector_target, &data)) {
			std::lock_guard<std::mutex> lock(target_mutex);
			vector_target = 0;
			vector_aimTargetDist = 99999.f;
			return;
		}

		Vector3 aim3d = get_aimbone_for_vector(data);
		if (!IsBoneDataValid(aim3d)) return;

		ImVec2 aim2d;
		if (!WorldToScreen(aim3d, &aim2d)) return;
		float Distance2d = screen_distance(Game.screen.x / 2, Game.screen.y / 2, aim2d.x, aim2d.y);

		if (!(Distance2d > config::get("aimbot", "aim_stop_radius", 0.f) && vector_radius > Distance2d)) return;

		Vector3 Crosshair3D = cam->m_cam_pos;
		float dist = get_distance(Crosshair3D, aim3d);
		if (dist <= 0.f) return;

		Vector3 Out((aim3d.x - Crosshair3D.x) / dist, (aim3d.y - Crosshair3D.y) / dist, (aim3d.z - Crosshair3D.z) / dist);

		const float smoothness = std::clamp(get_aim_config_float("vector_smoothness", 50.f), 0.f, 100.f);
		const float divisor = 1.f + smoothness;

		cam->m_tps_angles.x += (Out.x - cam->m_tps_angles.x) / divisor;
		cam->m_tps_angles.y += (Out.y - cam->m_tps_angles.y) / divisor;
		cam->m_tps_angles.z += (Out.z - cam->m_tps_angles.z) / divisor;

		cam->m_fps_angles.x += (Out.x - cam->m_fps_angles.x) / divisor;
		cam->m_fps_angles.y += (Out.y - cam->m_fps_angles.y) / divisor;
		cam->m_fps_angles.z += (Out.z - cam->m_fps_angles.z) / divisor;
	}

	void tick_silent_aim() {
		if (!Game.renderReady || Game.screen.x < 100) return;
		CPlayerAngles* cam = Game.getCam();
		if (!bind_state::is_active(k_silent_aim_bind)) {
			{
				std::lock_guard<std::mutex> lock(target_mutex);
				silent_target = 0;
			}
			target_aim_angle = { 0 };
			
			std::lock_guard<std::mutex> lock(execution_mutex);
			exec_ctx.active = false;
			return;
		}
		if (!IsValidPtr(cam) || !IsValidPtr(local.player)) {
			target_aim_angle = { 0 };
			return;
		}

		int aim_mode = config::get("aimbot", "silent_aim_mode", 0);

		// unarmed only disarms the bullet hook (can_fire); the lock itself stays
		// alive so Silent Line keeps mirroring the target without a weapon
		bool weapon_ok = true;
		DWORD weapHash = 0;
		if (get_weapon_hash(local.player, &weapHash)) {
			if (weapHash == 0xA2719263) {
				weapon_ok = false;
				target_aim_angle = { 0 };
			}
		}

		float silent_radius = get_aim_config_float("silent_fov", config::get("aimbot", "silent_radius", 50.f));

		CObject* old_target = 0;
		{
			std::lock_guard<std::mutex> lock(target_mutex);
			old_target = silent_target;
		}
		
		get_silent_target(cam, silent_radius);

		CObject* current_silent_target = 0;
		DataPed data;
		{
			std::lock_guard<std::mutex> lock(target_mutex);
			current_silent_target = silent_target;
			if (!current_silent_target) {
				target_aim_angle = { 0 };
				std::lock_guard<std::mutex> lock2(execution_mutex);
				exec_ctx.active = false;
				return;
			}
			data = silent_target_data;
		}

		if (!IsTargetValid(current_silent_target)) {
			target_aim_angle = { 0 };
			std::lock_guard<std::mutex> lock(execution_mutex);
			exec_ctx.active = false;
			return;
		}
		
		if (current_silent_target != old_target && old_target != 0) {
			target_aim_angle = { 0 };
			return;
		}
		
		Vector3 targetPos;
		if (!get_position(current_silent_target, &targetPos)) {
			target_aim_angle = { 0 };
			std::lock_guard<std::mutex> lock(execution_mutex);
			exec_ctx.active = false;
			return;
		}
		
		Vector3 localPos;
		if (!get_position(local.player, &localPos)) return;
		
		float dist_check = 0.f;
		if (!get_distance_between(&localPos, &targetPos, &dist_check)) {
			target_aim_angle = { 0 };
			std::lock_guard<std::mutex> lock(execution_mutex);
			exec_ctx.active = false;
			return;
		}
		
		if (dist_check > config::get("hack", "max_range", 1000.f)) {
			target_aim_angle = { 0 };
			return;
		}

		bool found_in_list = false;
		{
			std::lock_guard<std::mutex> lock(game::ped_list_mutex);
			for (auto it = game::ped_list.begin(); it != game::ped_list.end(); ++it) {
				if (it->first == current_silent_target && IsValidPtr(it->first)) {
					data = it->second;
					found_in_list = true;
					break;
				}
			}
		}
		
		if (!found_in_list || IsTargetDataDead(data) || !has_current_aim_target_data(data)) {
			std::lock_guard<std::mutex> lock(target_mutex);
			silent_target = 0;
			target_aim_angle = { 0 };
			std::lock_guard<std::mutex> lock2(execution_mutex);
			exec_ctx.active = false;
			return;
		}

		Vector3 aim3d = get_aimbone_for_silent(data);
		if (!IsBoneDataValid(aim3d)) {
			target_aim_angle = { 0 };
			std::lock_guard<std::mutex> lock(execution_mutex);
			exec_ctx.active = false;
			return;
		}

		target_aim_angle = aim3d;

		{
			std::lock_guard<std::mutex> lock(execution_mutex);
			exec_ctx.active = true;
			exec_ctx.can_fire = weapon_ok;
			exec_ctx.mode = aim_mode;
			exec_ctx.aim_pos = aim3d;
			exec_ctx.target = current_silent_target;
		}

	}

	void tick() {
		tick_vector_aim();
		tick_silent_aim();
		tick_triggerbot();
	}

	void shutdown() {
		release_triggerbot_fire();
	}

	void init() {
		runtime_debug::last_section = "aimbot_init";
		install_bullet_instance_hook();
	}

	void do_silent_aim() {
		
		if (!Game.gameDataReady) return;
		const bool silent_active = bind_state::is_active(k_silent_aim_bind);
		const bool damager_active = bind_state::is_active_or_enabled_when_unbound(k_damager_bind);
		if (damager_active) {
			runtime_debug::last_section = "gt_damager_active";
			run_damager_active();
		}
		else {
			reset_damager_target();
		}

		if (!silent_active) return;
		int mode = config::get("aimbot", "silent_aim_mode", 0);
		if (mode != 0) return;
		
		
		CObject* target = 0;
		DataPed target_data = {};
		runtime_debug::last_section = "gt_silent_acquire";
		if (!acquire_silent_shot_target(&target, &target_data)) return;
		runtime_debug::last_section = "gt_silent_target_valid";
		if (!IsTargetValid(target)) return;

		Vector3 aim_pos;
		runtime_debug::last_section = "gt_silent_fresh_bone";
		if (!get_fresh_silent_bone(target, &aim_pos)) {
			aim_pos = get_aimbone_for_silent(target_data);
		}
		if (!IsBoneDataValid(aim_pos)) return;
		
		Vector3 targetPos;
		runtime_debug::last_section = "gt_silent_target_pos";
		if (!get_position(target, &targetPos)) return;
		
		Vector3 localPos;
		runtime_debug::last_section = "gt_silent_local_pos";
		if (!get_position(local.player, &localPos)) return;
		
		float dist = 0.f;
		if (!get_distance_between(&localPos, &targetPos, &dist)) return;
		if (dist > config::get("hack", "max_range", 1000.f)) return;

		Vector3 damager_head = aim_pos;
		runtime_debug::last_section = "gt_silent_head_bone";
		get_fresh_head_bone(target, &damager_head);

		ExecutionContext ctx;
		ctx.active = true;
		ctx.mode = mode;
		ctx.aim_pos = aim_pos;
		ctx.damager_head_pos = damager_head;
		ctx.target = target;
		runtime_debug::last_section = "gt_silent_fire";
		do_silent_aim_logic(ctx);

	}
}


// ============================================================================
//  NOCTUA LICENSED BUILD WATERMARK  (auto-generated, do not remove)
//  Inert per-file identifier required by the EULA. It uniquely tags this
//  source file so this program is distinguishable from any other build.
//  Contains no functional logic.
//  file: src/features/aimbot/aim.hpp
// ============================================================================
#ifndef NOCTUA_LICENSE_MARK_b0790fa8c12bc4d5689866e1bec5e481
#define NOCTUA_LICENSE_MARK_b0790fa8c12bc4d5689866e1bec5e481
namespace noctua_license { namespace mark_b0790fa8c12bc4d5689866e1bec5e481 {
    inline constexpr unsigned long long kMarkId = 0x76eb0e68f61b4fc5ull;
    inline constexpr char kMarkFile[] = "src/features/aimbot/aim.hpp";
    inline constexpr unsigned char kMarkEntropy[] = { 0xb3, 0xa4, 0x42, 0xe6, 0x48, 0x9b, 0x29, 0x6f, 0x3b, 0x06, 0xc4, 0x6d, 0xd8, 0x0b, 0x4d, 0x8f };
    inline constexpr unsigned long  kMarkSalts[]  = { 0xbecedaa7ul, 0x7083a4b9ul, 0xb094a290ul, 0xaa0b0dc1ul, 0xed1434bcul, 0x1819980bul, 0xc6dfa539ul, 0xa837e2aful };
    [[maybe_unused]] inline unsigned long long mark_digest() noexcept {
        unsigned long long h = 1469598103934665603ull ^ kMarkId;
        for (unsigned char b : kMarkEntropy) { h ^= b; h *= 1099511628211ull; }
        for (unsigned long  s : kMarkSalts)  { h ^= s; h *= 1099511628211ull; }
        return h;
    }
} } // namespace noctua_license::mark_b0790fa8c12bc4d5689866e1bec5e481
#endif // NOCTUA_LICENSE_MARK_b0790fa8c12bc4d5689866e1bec5e481
