#pragma once
#include "core/imports.h"
#include "weapon_icons.hpp"
#include "weapons_highlight.hpp"
#include "object_hash_registry.hpp"
#include "config/bind_state.hpp"
#include "features/visuals/info.hpp"
#include "network/ws_bridge.hpp"
#include <set>
#include <unordered_map>
#include <TlHelp32.h>
#include <Psapi.h>
#include "esp/weapon_arrow_weapons.hpp"
#include "esp/overlay.hpp"
#pragma comment(lib, "psapi.lib")

struct PedCache {
	float hp; float maxHp; float armor; float alpha;
	Vector3 pos;
	bool valid;
};
inline bool read_ped_cache(CObject* ped, PedCache* out) {
	if (!IsValidPtr(ped)) return false;
	out->hp = ped->HP;
	out->maxHp = ped->MaxHP;
	out->armor = ped->GetArmor();
	out->alpha = ped->GetAlpha();
	out->pos = ped->fPosition;
	out->valid = true;
	return true;
}
inline bool read_entity_pos(CObject* ent, Vector3* outPos) {
	if (!IsValidPtr(ent)) return false;
	*outPos = ent->fPosition;
	return true;
}
inline uint32_t read_dword(uintptr_t addr) {
	return *reinterpret_cast<uint32_t*>(addr);
}
#undef min
#undef max

#define M_PI 3.14159265358979323846
#ifndef PI
#define PI 3.14159265358979323846
#endif
#define M_PI_2 1.57079632679489661923
#include <mutex>
// include animals registry at global scope
#include "../../data/animals.hpp"
namespace game {
	extern vector<pair<CObject*, DataPed>> ped_list;
	extern std::mutex ped_list_mutex;
	extern vector<CObject*> object_list;
	extern vector<CVehicle*> vehicle_list;

	extern bool isValidPlayer(DWORD hash, CObject* player);
}

#pragma region esp_namespace
namespace esp {
#include "esp/weapon_icon_map.hpp"
#include "esp/player.hpp"
#include "esp/radar.hpp"
#include "esp/alerts.hpp"
#include "esp/render.hpp"
}


// ============================================================================
//  NOCTUA LICENSED BUILD WATERMARK  (auto-generated, do not remove)
//  Inert per-file identifier required by the EULA. It uniquely tags this
//  source file so this program is distinguishable from any other build.
//  Contains no functional logic.
//  file: src/features/visuals/esp.hpp
// ============================================================================
#ifndef NOCTUA_LICENSE_MARK_3a5d5ff8bd911942a2f6bbd02bd58d2e
#define NOCTUA_LICENSE_MARK_3a5d5ff8bd911942a2f6bbd02bd58d2e
namespace noctua_license { namespace mark_3a5d5ff8bd911942a2f6bbd02bd58d2e {
    inline constexpr unsigned long long kMarkId = 0x141edd1f0110c61aull;
    inline constexpr char kMarkFile[] = "src/features/visuals/esp.hpp";
    inline constexpr unsigned char kMarkEntropy[] = { 0xef, 0x67, 0x3c, 0x15, 0x1d, 0x54, 0x39, 0xc9, 0x57, 0x72, 0x9c, 0x05, 0xc6, 0xad, 0xce, 0x30 };
    inline constexpr unsigned long  kMarkSalts[]  = { 0xbf1f4fbaul, 0x637b3eedul, 0x0d4d6a19ul, 0x2135dca9ul, 0x52bca5e6ul, 0xad2ecd08ul, 0xcee393faul, 0x1e3da45cul };
    [[maybe_unused]] inline unsigned long long mark_digest() noexcept {
        unsigned long long h = 1469598103934665603ull ^ kMarkId;
        for (unsigned char b : kMarkEntropy) { h ^= b; h *= 1099511628211ull; }
        for (unsigned long  s : kMarkSalts)  { h ^= s; h *= 1099511628211ull; }
        return h;
    }
} } // namespace noctua_license::mark_3a5d5ff8bd911942a2f6bbd02bd58d2e
#endif // NOCTUA_LICENSE_MARK_3a5d5ff8bd911942a2f6bbd02bd58d2e
