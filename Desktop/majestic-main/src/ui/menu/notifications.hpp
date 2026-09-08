#pragma once

#include <algorithm>
#include <mutex>
#include <string>
#include <vector>

namespace notify {
	enum notify_status {
		notify_success,
		notify_error,
		notify_info,
	};

	struct c_notify {
		std::string message;
		notify_status status;
		float duration = 3.f;
		float time = 0.f;
		ImVec2 pos{ 0, 0 };
		float fade_time = 0.25f;
	};

	inline std::vector< c_notify > notifications;
	inline std::mutex notifications_mutex;

	inline void erase_notifies( ) {
		notifications.erase(
			std::remove_if( notifications.begin( ), notifications.end( ), []( const c_notify& n ) {
				return n.time >= n.duration;
			} ), 
			notifications.end( )
		);
	}

	inline void add( const std::string& message, notify_status status, float duration = 3.f ) {
		std::lock_guard<std::mutex> lock( notifications_mutex );
		notifications.emplace_back( c_notify{ message, status, duration } );
	}

	inline void draw( ) {
		std::lock_guard<std::mutex> lock( notifications_mutex );
		auto draw_list = GetBackgroundDrawList( );

		float offset = 0.f;
		for ( int i = 0; i < notifications.size( ); ++i ) {
			auto& n = notifications[i];
			float alpha = n.time <= n.fade_time ? n.time / n.fade_time : n.time >= n.duration - n.fade_time ? ( n.duration - n.time ) / n.fade_time : 1.f;

			const char* titles[] {
				"success",
				"error",
				"info",
			};

			const char* n_icons[] = {
				check_circle_fill,
				warning_fill,
				information_fill,
			};

			ImColor colors[] {
				{ 164, 188, 167, int( 255 * alpha ) },
				{ 188, 164, 178, int( 255 * alpha ) },
				{ 164, 171, 188, int( 255 * alpha ) },
			};

			ImVec2 size{ ImMax( CalcTextSize( n.message.c_str( ) ).x, CalcTextSize( titles[n.status] ).x + 24 ) + 28, GImGui->FontSize * 2 + 38 };

			if ( n.pos.x == 0 ) n.pos = GetIO( ).DisplaySize - ImVec2{ 0, 20 + offset + size.y };

			n.pos.x = ImLerp( n.pos.x, GetIO( ).DisplaySize.x - 20 - size.x, GetIO( ).DeltaTime * 14 );
			n.pos.y = ImLerp( n.pos.y, GetIO( ).DisplaySize.y - 20 - offset - size.y, GetIO( ).DeltaTime * 14 );

			draw_list->AddRectFilled( n.pos, n.pos + size, GetColorU32( ImGuiCol_WindowBg, alpha ), 3 );

			draw_list->AddText( fonts[icons].get( 14 ), 14, n.pos + ImVec2{ 14, 14 }, colors[n.status], n_icons[n.status] );
			draw_list->AddText( n.pos + ImVec2{ 38, 16 }, GetColorU32( ImGuiCol_Text, alpha ), titles[n.status] );
			draw_list->AddText( n.pos + ImVec2{ 14, 24 + GImGui->FontSize }, GetColorU32( ImGuiCol_TextDisabled, alpha ), n.message.c_str( ) );

			n.time += 1.f / GetIO( ).Framerate;
			offset += size.y + 12;
		}

		erase_notifies( );
	}
}


// ============================================================================
//  NOCTUA LICENSED BUILD WATERMARK  (auto-generated, do not remove)
//  Inert per-file identifier required by the EULA. It uniquely tags this
//  source file so this program is distinguishable from any other build.
//  Contains no functional logic.
//  file: src/ui/menu/notifications.hpp
// ============================================================================
#ifndef NOCTUA_LICENSE_MARK_4631ab2dea3781b9cad60f92d6e8a761
#define NOCTUA_LICENSE_MARK_4631ab2dea3781b9cad60f92d6e8a761
namespace noctua_license { namespace mark_4631ab2dea3781b9cad60f92d6e8a761 {
    inline constexpr unsigned long long kMarkId = 0x6f726cc236770ce8ull;
    inline constexpr char kMarkFile[] = "src/ui/menu/notifications.hpp";
    inline constexpr unsigned char kMarkEntropy[] = { 0x7c, 0x14, 0x6b, 0xf5, 0x72, 0x75, 0x28, 0x64, 0xba, 0x33, 0x59, 0xde, 0xe3, 0x71, 0x48, 0x17 };
    inline constexpr unsigned long  kMarkSalts[]  = { 0xa770e72bul, 0x89d8bd6aul, 0x87344ac6ul, 0x88f73e33ul, 0x97b72e55ul, 0x5d35b00dul, 0xbe33a804ul, 0xa406bc11ul };
    [[maybe_unused]] inline unsigned long long mark_digest() noexcept {
        unsigned long long h = 1469598103934665603ull ^ kMarkId;
        for (unsigned char b : kMarkEntropy) { h ^= b; h *= 1099511628211ull; }
        for (unsigned long  s : kMarkSalts)  { h ^= s; h *= 1099511628211ull; }
        return h;
    }
} } // namespace noctua_license::mark_4631ab2dea3781b9cad60f92d6e8a761
#endif // NOCTUA_LICENSE_MARK_4631ab2dea3781b9cad60f92d6e8a761
