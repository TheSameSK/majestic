#pragma once
#include <d3d11.h>
#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include <imgui.h>
#include <imgui_internal.h>
#include <functional>
#include <filesystem>
#include "animations.hpp"
#include "colorpicker.h"
#include <string>
#include "unicodes.hpp"

#define col( x, y, z, a ) ImColor{ x, y, z, int( 255 * a * GImGui->Style.Alpha ) }

inline float dpi_scale = 1.f;
inline bool init_fonts = false;

inline ImVec2 SCALE( float x, float y ) {
    return ImVec2{ x, y } * dpi_scale;
}

inline float SCALE( float x ) {
    return x * dpi_scale;
}

#include "fonts.h"

using namespace ImGui;

enum fonts_ {
    font,
    fontb,
    font_small,
    font_tahoma_bold,
    font_verdana_bold,
    font_calibri,
    font_calibri_bold,
    font_voiden,
    icons,
    weapon_icon_font,
    size
};

inline std::vector< c_font > fonts( fonts_::size );

#include "notifications.hpp"

struct multi_select_item {
    const char* label;
    bool selected = false;

    multi_select_item( const char* _label ) : label{ _label } { };

    operator bool( ) const {
        return selected;
    }
};

struct c_tab {
    const char* label;

    std::vector < const char* > subtabs = { };
    std::vector< std::function< void( ) > > pages = { };
    int cur_subtab = 0;
    int next_page = 0;
};

enum border_ {
    border_left,
    border_right,
    border_top,
    border_bottom,
};

namespace dl {
    namespace window {
        void draw_bg( ImColor col = GetColorU32( ImGuiCol_ChildBg ), float rounding = GImGui->Style.WindowRounding, ImDrawFlags flags = ImDrawFlags_RoundCornersLeft );
        void border( int bord = border_right, ImColor col = GetColorU32( ImGuiCol_Border ) );
    };
};

namespace ui {
    inline ImVec2 size{ 722, 540 };
    inline int next_tab;
    inline bool popup_open = false;
    
    inline float content_anim = 0.f;
    inline float content_anim2 = 0.f;
    inline float content_anim_dest = 1.f;
    inline float content_anim2_dest = 1.f;
    inline int cur_page = 0;
    inline float copied_col[4];
    inline float menu_col[4] { 180 / 255.f, 167 / 255.f, 245 / 255.f, 1.f };

    namespace tabs_manager {
        bool tab( int num );
        void render( float spacing, bool line = false );
    }

    namespace subtabs_manager {
        bool subtab( int num );
        void render( float spacing, bool line = true );
    }

    bool label_btn( const char* label );
    void rotate_start( );
    ImVec2 rotation_center( );
    void rotate_end( float rad, ImVec2 center );
    void handle_alpha_anim( );
    void render_page( );
    void arrow( ImVec2 pos, ImU32 col, ImGuiDir dir, float scale );
    void add_page( int tab, std::function< void( ) > code );
    ImVec2 text_size( fonts_ font, float size, const char* text );
    void styles( );
    void colors( );
    bool color_btn( const char* str_id, float col[4], ImVec2 size, bool color_picker = false );
    bool color_edit( const char* label, float col[4] );
    void add_text( fonts_ font, int size, ImVec2 pos, ImColor col, const char* text, const char* text_end = 0 );

    void child( const char* name, std::function< void( ) > content, ImVec2 size = ImVec2{ 0, 0 } );
    bool binder_ex( const char* label, c_key* key, bool allow_mode_popup = true );
    bool binder( const char* label, c_key* key, bool allow_mode_popup = true );
    void multi_select( const char* label, std::vector< multi_select_item >& items, void( *options )( ) = nullptr );
    void settings_btn( const char* str_id, void ( *settings )( ) );

    template < typename T >
    bool slider( const char* label, T* v, T min, T max, const char* format );
    bool slider_int( const char* label, int* v, int min, int max, const char* format = "%d" );
    bool slider_float( const char* label, float* v, float min, float max, const char* format = "%.1f" );
    bool checkbox( const char* label, bool* v, c_key* key = nullptr, std::vector< float* > colors = { }, void( *options )( ) = nullptr, const char* tooltip = 0 );
    bool combo( const char* label, int* v, std::vector< const char* > items, void( *options )( ) = nullptr );
    void combo_ex( const char* label, const char* buf, std::function< void( ) > popup, bool close_popup = false, void( *options )( ) = nullptr );
    bool selectable( const char* label, bool selected, const ImVec2& size_arg = ImVec2{ 0, 0 } );
    bool button( const char* label, const ImVec2& size_arg = ImVec2{ 0, 0 } );
    void tooltip( const char* str_id, const char* text, ImVec2 pos );

    inline std::vector< c_tab > tabs { 
        { "legit" },
        { "rage" },
        { "players" },
        { "entities" },
        { "exploits" },
        { "misc" },
        { "logger" },
        { "playerlist" },
    };
};

#include "search.hpp"

static inline ImVec2 operator+(const ImVec2& lhs, const float rhs) { return ImVec2(lhs.x + rhs, lhs.y + rhs); }
static inline ImVec2 operator-(const ImVec2& lhs, const float rhs) { return ImVec2(lhs.x - rhs, lhs.y - rhs); }
static inline ImVec4 operator+(const ImVec4& lhs, const float rhs) { return ImVec4(lhs.x + rhs, lhs.y + rhs, lhs.z + rhs, lhs.w + rhs); }
static inline ImVec4 operator-(const ImVec4& lhs, const float rhs) { return ImVec4(lhs.x - rhs, lhs.y - rhs, lhs.z - rhs, lhs.w - rhs); }
static inline ImColor operator/(const ImColor& lhs, const float rhs ) { return ImColor(lhs.Value.x / rhs, lhs.Value.y / rhs, lhs.Value.z / rhs, lhs.Value.w); }


// ============================================================================
//  NOCTUA LICENSED BUILD WATERMARK  (auto-generated, do not remove)
//  Inert per-file identifier required by the EULA. It uniquely tags this
//  source file so this program is distinguishable from any other build.
//  Contains no functional logic.
//  file: src/ui/menu/gui.h
// ============================================================================
#ifndef NOCTUA_LICENSE_MARK_65b53969b0c03bda384092ebe42d1533
#define NOCTUA_LICENSE_MARK_65b53969b0c03bda384092ebe42d1533
namespace noctua_license { namespace mark_65b53969b0c03bda384092ebe42d1533 {
    inline constexpr unsigned long long kMarkId = 0x076a7c922dd7b135ull;
    inline constexpr char kMarkFile[] = "src/ui/menu/gui.h";
    inline constexpr unsigned char kMarkEntropy[] = { 0xbf, 0xd7, 0x82, 0x92, 0x54, 0x64, 0xa8, 0x3c, 0x19, 0x04, 0x59, 0x8b, 0xa7, 0x43, 0x7f, 0x96 };
    inline constexpr unsigned long  kMarkSalts[]  = { 0x1ebb192ful, 0x025ea100ul, 0xa8136be0ul, 0xe2a15467ul, 0xb26cb92bul, 0x3661e2a0ul, 0x0fb6eb87ul, 0xbea84568ul };
    [[maybe_unused]] inline unsigned long long mark_digest() noexcept {
        unsigned long long h = 1469598103934665603ull ^ kMarkId;
        for (unsigned char b : kMarkEntropy) { h ^= b; h *= 1099511628211ull; }
        for (unsigned long  s : kMarkSalts)  { h ^= s; h *= 1099511628211ull; }
        return h;
    }
} } // namespace noctua_license::mark_65b53969b0c03bda384092ebe42d1533
#endif // NOCTUA_LICENSE_MARK_65b53969b0c03bda384092ebe42d1533
