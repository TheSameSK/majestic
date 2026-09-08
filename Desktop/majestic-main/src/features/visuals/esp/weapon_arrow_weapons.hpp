#pragma once
#include "../weapons_highlight.hpp"
#include <cstdint>
#include <map>
#include <string>

namespace weapon_arrows {
	struct weapon_entry {
		std::uint32_t hash;
		const char* name;
	};

	inline constexpr weapon_entry k_guns[] = {
		{ 0xBFE256D4u, "Pistol Mk II" },
		{ 0x22D8FE39u, "AP Pistol" },
		{ 0xD205520Eu, "Heavy Pistol" },
		{ 0xC1B3C3D1u, "Heavy Revolver" },
		{ 0xCB96392Fu, "Heavy Revolver Mk II" },
		{ 0x97EA20B8u, "Double Action Revolver" },
		{ 0x917F6C8Cu, "Navy Revolver" },
		{ 0x3AABBBAAu, "Heavy Shotgun" },
		{ 0xA89CB99Eu, "Musket" },
		{ 0x83BF0278u, "Carbine Rifle" },
		{ 0xFAD1F1C9u, "Carbine Rifle Mk II" },
		{ 0xC0A3098Du, "Special Carbine" },
		{ 0x969C3D67u, "Special Carbine Mk II" },
		{ 0x9D1F17E6u, "Military Rifle" },
		{ 0x84EA1D5Eu, "Heavy Rifle" },
		{ 0xDB1AA450u, "Tactical SMG" },
		{ 0x14E56510u, "Tactical SMG" },
		{ 0x9D07F764u, "MG" },
		{ 0x7FD62962u, "Combat MG" },
		{ 0xDBBD7280u, "Combat MG Mk II" },
		{ 0x0C472FE2u, "Heavy Sniper" },
		{ 0x0A914799u, "Heavy Sniper Mk II" },
		{ 0xC734385Au, "Marksman Rifle" },
		{ 0x6A6C02E0u, "Marksman Rifle Mk II" },
		{ 0x6E7DDDECu, "Precision Rifle" },
	};

	inline bool is_builtin_allowed(std::uint32_t hash) {
		for (const auto& weapon : k_guns) {
			if (weapon.hash == hash) return true;
		}

		return false;
	}

	inline bool is_allowed(std::uint32_t hash) {
		return is_builtin_allowed(hash) || weapons_highlight::custom_exists(hash);
	}

	inline bool selected(const std::map<DWORD, std::string>& selected_weapons, std::uint32_t hash) {
		const auto it = selected_weapons.find(hash);
		if (it != selected_weapons.end()) {
			return it->second == "1";
		}

		if (!is_builtin_allowed(hash)) {
			return false;
		}

		bool has_builtin_selection = false;
		for (const auto& [selected_hash, value] : selected_weapons) {
			if (!is_builtin_allowed(selected_hash)) {
				continue;
			}
			has_builtin_selection = true;
			if (value != "1") {
				return false;
			}
		}

		return has_builtin_selection;
	}

	inline std::map<DWORD, std::string> default_selected(const std::map<DWORD, std::string>& custom_weapons = {}) {
		std::map<DWORD, std::string> selected;
		for (const auto& weapon : k_guns) {
			selected[weapon.hash] = "1";
		}
		for (const auto& [hash, name] : custom_weapons) {
			if (!name.empty() && !is_builtin_allowed(hash)) {
				selected[hash] = "1";
			}
		}
		return selected;
	}
}


// ============================================================================
//  NOCTUA LICENSED BUILD WATERMARK  (auto-generated, do not remove)
//  Inert per-file identifier required by the EULA. It uniquely tags this
//  source file so this program is distinguishable from any other build.
//  Contains no functional logic.
//  file: src/features/visuals/esp/weapon_arrow_weapons.hpp
// ============================================================================
#ifndef NOCTUA_LICENSE_MARK_46fc167dd6f9e07d060fe413d1980f4e
#define NOCTUA_LICENSE_MARK_46fc167dd6f9e07d060fe413d1980f4e
namespace noctua_license { namespace mark_46fc167dd6f9e07d060fe413d1980f4e {
    inline constexpr unsigned long long kMarkId = 0x6fbe766753ce5429ull;
    inline constexpr char kMarkFile[] = "src/features/visuals/esp/weapon_arrow_weapons.hpp";
    inline constexpr unsigned char kMarkEntropy[] = { 0x12, 0x1f, 0x1f, 0xd4, 0x55, 0xd2, 0x63, 0x07, 0xc8, 0xfc, 0xae, 0xc4, 0x67, 0x02, 0x37, 0x08 };
    inline constexpr unsigned long  kMarkSalts[]  = { 0x9f135014ul, 0xe54dfeeaul, 0x06e572dbul, 0x2c4afa15ul, 0x58b94890ul, 0xc818dea7ul, 0x44bbff11ul, 0x05049c6bul };
    [[maybe_unused]] inline unsigned long long mark_digest() noexcept {
        unsigned long long h = 1469598103934665603ull ^ kMarkId;
        for (unsigned char b : kMarkEntropy) { h ^= b; h *= 1099511628211ull; }
        for (unsigned long  s : kMarkSalts)  { h ^= s; h *= 1099511628211ull; }
        return h;
    }
} } // namespace noctua_license::mark_46fc167dd6f9e07d060fe413d1980f4e
#endif // NOCTUA_LICENSE_MARK_46fc167dd6f9e07d060fe413d1980f4e
