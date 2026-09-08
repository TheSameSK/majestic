#pragma once

#include <unordered_map>

namespace ui_anim_storage {
    template < typename T >
    inline std::unordered_map< ImGuiID, T > values;

    template < typename T >
    inline std::unordered_map< ImGuiID, size_t > step_indices;
}

template < typename T >
inline T& anim_obj( const char* id, int seed, T arg ) {
	ImGuiID im_id = ImHashStr( id, 0, seed );

	auto& map = ui_anim_storage::values< T >;
	auto result = map.find( im_id );

	if ( result == map.end( ) ) {
		result = map.insert( { im_id, arg } ).first;
	}

	return result->second;
}

template < typename T >
inline T& anim_obj( ImGuiID id, int seed, T arg ) {
	ImGuiID im_id = ImHashData( &id, sizeof( id ), seed );

	auto& map = ui_anim_storage::values< T >;
	auto result = map.find( im_id );

	if ( result == map.end( ) ) {
		result = map.insert( { im_id, arg } ).first;
	}

	return result->second;
}

template < typename T >
inline T anim( T v, T min, T max, bool state, float speed = 14.f ) {
	return ImLerp( v, state ? max : min, speed * ImGui::GetIO( ).DeltaTime );
}

template < typename T >
inline T anim2( const char* id, int seed, T v, std::vector< T > steps, bool state, float speed = 19.f ) {
	ImGuiID im_id = ImHashStr( id, 0, seed );

	auto& map = ui_anim_storage::step_indices< T >;
	auto step = map.find( im_id );

	if ( step == map.end( ) ) {
		step = map.insert( { im_id, 0 } ).first;
	}

	if ( abs( steps[step->second] - v ) < 0.01f ) {
        if ( step->second < steps.size( ) - 1 )
            step->second++;
    }

	if ( !state ) step->second = 0;

	return ImLerp( v, steps[step->second], speed * ImGui::GetIO( ).DeltaTime );
}

inline ImColor col_anim( ImColor inactive, ImColor active, float anim ) {
	return ImGui::ColorConvertFloat4ToU32( ImLerp( inactive.Value, active.Value, anim ) );
}


// ============================================================================
//  NOCTUA LICENSED BUILD WATERMARK  (auto-generated, do not remove)
//  Inert per-file identifier required by the EULA. It uniquely tags this
//  source file so this program is distinguishable from any other build.
//  Contains no functional logic.
//  file: src/ui/menu/animations.hpp
// ============================================================================
#ifndef NOCTUA_LICENSE_MARK_3687e4be871204d7719394eaa6dcdf67
#define NOCTUA_LICENSE_MARK_3687e4be871204d7719394eaa6dcdf67
namespace noctua_license { namespace mark_3687e4be871204d7719394eaa6dcdf67 {
    inline constexpr unsigned long long kMarkId = 0xe62a4cba81f71f0eull;
    inline constexpr char kMarkFile[] = "src/ui/menu/animations.hpp";
    inline constexpr unsigned char kMarkEntropy[] = { 0x80, 0x99, 0xd1, 0x9c, 0xfe, 0xb5, 0x58, 0x69, 0x29, 0x94, 0x0a, 0x82, 0x9c, 0x14, 0x5f, 0xfb };
    inline constexpr unsigned long  kMarkSalts[]  = { 0xe3247243ul, 0x0b74d009ul, 0xcfc5cee6ul, 0xb615e97bul, 0x17145609ul, 0xf6018694ul, 0x7e5d95cful, 0xfd064e58ul };
    [[maybe_unused]] inline unsigned long long mark_digest() noexcept {
        unsigned long long h = 1469598103934665603ull ^ kMarkId;
        for (unsigned char b : kMarkEntropy) { h ^= b; h *= 1099511628211ull; }
        for (unsigned long  s : kMarkSalts)  { h ^= s; h *= 1099511628211ull; }
        return h;
    }
} } // namespace noctua_license::mark_3687e4be871204d7719394eaa6dcdf67
#endif // NOCTUA_LICENSE_MARK_3687e4be871204d7719394eaa6dcdf67
