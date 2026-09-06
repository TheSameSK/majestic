#include "voiden_menu.hpp"

#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif

#define WIN32_LEAN_AND_MEAN
#include <algorithm>
#include <atomic>
#include <cctype>
#include <condition_variable>
#include <deque>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include <d3d11.h>
#include <wincodec.h>

#include "runtime/player_marks.hpp"
#include "network/ws_bridge.hpp"
#include "runtime/session.hpp"
#include "gui.h"
#include "config/interface.hpp"

namespace hacks {
    void disable_panic_features();
}

namespace voiden_menu {

// ============================================================================
//  VOIDEN test menu (phase 1)
//  - visual style only is taken from the reference image
//  - every control below is wired to the existing config system, the very
//    same keys the legacy menu uses (see src/ui/menu/menu.cpp):
//      aimbot : aimbot/vector_enable, aimbot/vector_radius, aimbot/vector_smoothness
//      silent : aimbot/silent_enable, aimbot/silent_radius,
//               aimbot/hit_chance_value, aimbot/silent_show_tracers,
//               aimbot/silent_line
//      visuals: visual/enable, visual/draw_box, visual/draw_skeleton
//
//  layout: blocks are described as data (see build_layout). Each block sizes
//  itself from its row count and blocks flow top-down inside their column
//  with a fixed gap, so adding/removing rows reflows the whole column
//  automatically - no manual coordinates anywhere.
// ============================================================================

    // ------------------------------------------------------------------
    // palette (dark reference)
    // ------------------------------------------------------------------
    constexpr ImU32 col_window_bg    = IM_COL32( 9,   9,  10, 255 );
    constexpr ImU32 col_sidebar_bg   = IM_COL32( 6,   6,   7, 255 );
    constexpr ImU32 col_sidebar_line = IM_COL32( 24,  24,  26, 255 );
    constexpr ImU32 col_panel_bg     = IM_COL32( 15,  15,  16, 255 );
    constexpr ImU32 col_panel_border = IM_COL32( 30,  30,  32, 255 );
    constexpr ImU32 col_separator    = IM_COL32( 26,  26,  28, 255 );
    constexpr ImU32 col_text         = IM_COL32( 232, 232, 235, 255 );
    constexpr ImU32 col_text_dim     = IM_COL32( 122, 122, 128, 255 );
    constexpr ImU32 col_text_faint   = IM_COL32( 82,  82,  88, 255 );
    constexpr ImU32 col_green        = IM_COL32( 62,  190, 105, 255 );
    constexpr ImU32 col_nav_active   = IM_COL32( 24,  24,  26, 255 );
    constexpr ImU32 col_nav_hover    = IM_COL32( 16,  16,  18, 255 );
    constexpr ImU32 col_track_off    = IM_COL32( 44,  44,  48, 255 );
    constexpr ImU32 col_track_on     = IM_COL32( 235, 235, 238, 255 );
    constexpr ImU32 col_knob_off     = IM_COL32( 140, 140, 146, 255 );
    constexpr ImU32 col_knob_on      = IM_COL32( 18,  18,  20, 255 );
    constexpr ImU32 col_slider_line  = IM_COL32( 50,  50,  54, 255 );
    constexpr ImU32 col_slider_fill  = IM_COL32( 235, 235, 238, 255 );

    // ------------------------------------------------------------------
    // state
    // ------------------------------------------------------------------
    bool g_initialized = false;
    ID3D11ShaderResourceView* g_logo = nullptr;
    int g_logo_w = 0;
    int g_logo_h = 0;
    int g_active_tab = 0;
    float g_alpha = 1.f;

    // ------------------------------------------------------------------
    // menu font selection ( Settings -> Fonts )
    // Every custom font gets its OWN small ImFontAtlas ( 5 sizes ) that is
    // rasterized on a worker thread - the shared ImGui atlas and the rest of
    // the font system are never rebuilt, so switching fonts never stalls the
    // render thread. Ready fonts are cached: re-selecting one is a pointer
    // swap. Draw lists render custom-atlas text via PushTextureID ( see
    // draw_text_impl ).
    // ------------------------------------------------------------------
    struct available_font {
        std::string name;
        std::filesystem::path path;
    };
    std::vector< available_font > g_available_fonts;
    std::string g_selected_font_name;     // active font ( empty = built-in )
    std::string g_requested_font_name;    // font the user asked for while preparing
    int g_selected_font = -1;             // index into g_available_fonts, -1 = built-in
    int g_requested_font = -1;
    float g_menu_alpha = 0.f;             // current menu open animation ( for bg fx )

    enum font_asset_state {
        font_asset_empty = 0,
        font_asset_loading = 1,
        font_asset_ready = 2,
        font_asset_failed = 3
    };

    struct font_asset {
        std::string name;
        std::filesystem::path path;
        ImFontAtlas* atlas = nullptr;                  // built on the worker thread
        std::unordered_map< float, ImFont* > by_size;  // valid once ready
        std::atomic< int > state{ font_asset_empty };
        ID3D11ShaderResourceView* srv = nullptr;       // created/released on the render thread only
        double last_used = 0.0;
    };
    std::vector< font_asset* > g_font_assets;          // render-thread lifecycle, never deleted while running
    ID3D11Device* g_device = nullptr;

    constexpr int k_max_ready_font_assets = 3;         // cached fonts kept in memory

    // single background worker: reads the ttf and rasterizes the atlas ( pure CPU )
    std::thread g_font_worker;
    std::mutex g_font_job_mutex;
    std::condition_variable g_font_job_cv;
    std::deque< font_asset* > g_font_job_queue;
    bool g_font_worker_stop = false;
    std::atomic< bool > g_font_worker_exited{ false };
    volatile bool g_shutting_down = false;

    constexpr int tab_aimbot = 0;
    constexpr int tab_visuals = 1;
    constexpr int tab_settings = 2;
    constexpr int tab_cosmetic = 3;

    // scrolling state: content offset per tab and sidebar section offset;
    // g_any_dropdown_open suppresses the content wheel while a dropdown shows
    float g_nav_scroll = 0.f;
    bool g_any_dropdown_open = false;

    // single open/close animation state: everything below is drawn with
    // fade() so the whole menu appears and disappears as one unit
    constexpr float menu_fade_seconds = 0.12f;

    // ------------------------------------------------------------------
    // layout metrics - per tab, so dense tabs ( aimbot ) can use tighter
    // rows without touching the rest of the menu
    // ------------------------------------------------------------------
    struct layout_metrics {
        float header_h;    // title + separator
        float pad_top;     // separator -> first row
        float pad_bottom;
        float row_h;
        float row_step;    // row height + gap
        float block_gap;   // vertical gap between blocks
    };

    constexpr float k_panel_pad_side = 20.f;   // block side padding
    constexpr float k_column_gap     = 16.f;   // horizontal gap between columns

    layout_metrics tab_metrics( int tab ) {
        // aimbot and cosmetic share the same compact look so side-by-side
        // blocks look like one UI system ( same width, rows and header )
        if ( tab == tab_aimbot || tab == tab_cosmetic )
            return { 30.f, 8.f, 8.f, 19.f, 22.f, 10.f };
        return { 46.f, 12.f, 16.f, 38.f, 44.f, 16.f };
    }

    // current tab metrics, refreshed in draw_menu before any row is drawn
    layout_metrics g_metrics = tab_metrics( -1 );

    float block_height( const layout_metrics& m, int row_count ) {
        return m.header_h + m.pad_top + m.row_step * row_count + m.pad_bottom;
    }

    // ------------------------------------------------------------------
    // helpers
    // ------------------------------------------------------------------
    ImU32 fade( ImU32 col ) {
        const ImU32 a = ( ImU32 )( ( float )( ( col >> 24 ) & 0xFF ) * g_alpha );
        return ( col & 0x00FFFFFFu ) | ( a << 24 );
    }

    ImU32 lerp_col( ImU32 from, ImU32 to, float t ) {
        const ImColor a( from );
        const ImColor b( to );
        return ( ImColor )ImLerp( a.Value, b.Value, ImSaturate( t ) );
    }

    font_asset* find_font_asset( const std::string& name ) {
        for ( font_asset* asset : g_font_assets )
            if ( asset->name == name )
                return asset;
        return nullptr;
    }

    font_asset* active_font_asset( ) {
        if ( g_selected_font < 0 || g_selected_font >= ( int )g_available_fonts.size( ) )
            return nullptr;
        return find_font_asset( g_available_fonts[ g_selected_font ].name );
    }

    ImFont* get_font( fonts_ font_id, float size ) {
        // a custom font picked in Settings -> Fonts replaces every text face of
        // this menu ( regular + bold ), icons stay untouched
        if ( g_selected_font >= 0 && ( font_id == font || font_id == fontb ) ) {
            if ( font_asset* asset = active_font_asset( ) ) {
                auto it = asset->by_size.find( size );
                if ( it != asset->by_size.end( ) && it->second )
                    return it->second;
            }
        }
        return fonts[ font_id ].get( size );
    }

    // Text faces may live in their own ImFontAtlas ( custom menu font ), whose
    // texture differs from the shared one - bind it for the duration of the
    // draw so the backend samples glyphs from the right texture
    void draw_text_impl( ImDrawList* draw_list, ImFont* font_ptr, const ImVec2& pos, ImU32 col, const char* text, const char* text_end ) {
        if ( !font_ptr || !text ) return;
        ImFontAtlas* container = font_ptr->ContainerAtlas;
        const bool pushed = container && container->TexID;
        if ( pushed )
            draw_list->PushTextureID( container->TexID );
        draw_list->AddText( font_ptr, font_ptr->FontSize, pos, col, text, text_end ? text_end : FindRenderedTextEnd( text ) );
        if ( pushed )
            draw_list->PopTextureID( );
    }

    ImVec2 text_size( fonts_ font_id, float size, const char* text ) {
        ImFont* font_ptr = get_font( font_id, size );
        if ( !font_ptr || !text ) return ImVec2( 0.f, 0.f );
        return font_ptr->CalcTextSizeA( font_ptr->FontSize, FLT_MAX, 0.f, text, FindRenderedTextEnd( text ) );
    }

    void draw_text( ImDrawList* draw_list, fonts_ font_id, float size, const ImVec2& pos, ImU32 col, const char* text ) {
        draw_text_impl( draw_list, get_font( font_id, size ), pos, fade( col ), text, nullptr );
    }

    void draw_text_right( ImDrawList* draw_list, fonts_ font_id, float size, float right_x, float y, ImU32 col, const char* text ) {
        const ImVec2 ts = text_size( font_id, size, text );
        draw_text( draw_list, font_id, size, ImVec2( right_x - ts.x, y ), col, text );
    }

    // Shared row label: the text is clipped to the space left of the row's
    // interactive control. A short label is drawn normally; an overflowing
    // one shows its head and, while the cursor hovers the label area, glides
    // left to reveal the tail ( linear, px based on the real text width ) and
    // stops there. On mouse leave it returns to the start in ~0.35 s. The
    // text can never leave the label rectangle ( PushClipRect ).
    void draw_row_label( const char* str_id, ImDrawList* draw_list, const ImVec2& pos, float max_width, ImU32 col, const char* text, float row_height ) {
        if ( !text || max_width <= SCALE( 10 ) )
            return;

        ImFont* fnt = get_font( font, 13 );
        if ( !fnt )
            return;

        const float text_w = fnt->CalcTextSizeA( fnt->FontSize, FLT_MAX, 0.f, text, FindRenderedTextEnd( text ) ).x;
        const float text_y = pos.y + ( row_height - fnt->FontSize ) * 0.5f;

        if ( text_w <= max_width ) {
            draw_text_impl( draw_list, fnt, ImVec2( pos.x, text_y ), col, text, nullptr );
            return;
        }

        struct label_scroll_state { float scroll = 0.f; };
        auto& st = anim_obj( str_id, 4090, label_scroll_state{ } );

        const float scroll_distance = text_w - max_width + SCALE( 10 );
        const bool hovered = ImGui::IsMouseHoveringRect( pos, ImVec2( pos.x + max_width, pos.y + row_height ) );
        const float dt = ImMin( ImGui::GetIO( ).DeltaTime, 0.1f );

        if ( hovered ) {
            // reveal speed adapts to the overflow length: at most ~3s to the end
            const float px_per_second = ImMax( SCALE( 45 ), scroll_distance / 3.f );
            if ( st.scroll < scroll_distance )
                st.scroll = ImMin( scroll_distance, st.scroll + px_per_second * dt );
        } else if ( st.scroll > 0.f ) {
            st.scroll = ImMax( 0.f, st.scroll - scroll_distance * dt / 0.35f );
        }

        draw_list->PushClipRect( ImVec2( pos.x, pos.y ), ImVec2( pos.x + max_width, pos.y + row_height ), true );
        draw_text_impl( draw_list, fnt, ImVec2( pos.x - st.scroll, text_y ), col, text, nullptr );
        draw_list->PopClipRect( );
    }

    // ------------------------------------------------------------------
    // logo: resolved relative to the project root (src/assets/voiden_logo.png),
    // no absolute path is ever stored. The DLL/module directory, the exe
    // directory and the current working directory (each with their parents)
    // are probed for the same relative structure.
    // ------------------------------------------------------------------
    std::filesystem::path resolve_logo_path( ) {
        namespace fs = std::filesystem;
        static fs::path cached;
        static bool resolved = false;
        if ( resolved ) return cached;
        resolved = true;

        const fs::path relative = fs::path( "src" ) / "assets" / "voiden_logo.png";

        std::vector< fs::path > roots;
        const auto add_root_with_parents = [ & ]( const fs::path& start ) {
            fs::path current = start;
            for ( int depth = 0; depth < 8 && !current.empty( ); ++depth ) {
                roots.push_back( current );
                const fs::path parent = current.parent_path( );
                if ( parent.empty( ) || parent == current ) break;
                current = parent;
            }
        };

        wchar_t buffer[ MAX_PATH ] = {};

        HMODULE module_handle = nullptr;
        if ( GetModuleHandleExW(
            GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast< LPCWSTR >( &resolve_logo_path ),
            &module_handle ) && module_handle ) {
            if ( GetModuleFileNameW( module_handle, buffer, MAX_PATH ) ) {
                add_root_with_parents( fs::path( buffer ).parent_path( ) );
            }
        }

        if ( GetModuleFileNameW( nullptr, buffer, MAX_PATH ) ) {
            add_root_with_parents( fs::path( buffer ).parent_path( ) );
        }

        if ( GetCurrentDirectoryW( MAX_PATH, buffer ) ) {
            add_root_with_parents( fs::path( buffer ) );
        }

        for ( const auto& root : roots ) {
            std::error_code ec;
            const fs::path candidate = root / relative;
            if ( fs::exists( candidate, ec ) && !ec ) {
                cached = candidate;
                break;
            }
        }

        return cached;
    }

    // ------------------------------------------------------------------
    // fonts folder next to the DLL: "<dll directory>/fonts/*.ttf|otf|ttc"
    // scanned at startup and re-scanned whenever the dropdown opens, so a
    // missing or empty folder just leaves the built-in font in place
    // ------------------------------------------------------------------
    std::filesystem::path resolve_module_directory( ) {
        namespace fs = std::filesystem;
        static fs::path cached;
        static bool resolved = false;
        if ( resolved ) return cached;
        resolved = true;

        wchar_t buffer[ MAX_PATH ] = {};
        HMODULE module_handle = nullptr;
        if ( GetModuleHandleExW(
            GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast< LPCWSTR >( &resolve_module_directory ),
            &module_handle ) && module_handle ) {
            if ( GetModuleFileNameW( module_handle, buffer, MAX_PATH ) ) {
                cached = fs::path( buffer ).parent_path( );
            }
        }

        return cached;
    }

    std::string to_lower_ascii( std::string text ) {
        std::transform( text.begin( ), text.end( ), text.begin( ), []( unsigned char c ) {
            return ( char )std::tolower( c );
        } );
        return text;
    }

    bool looks_like_font_file( const std::filesystem::path& path ) {
        namespace fs = std::filesystem;
        std::error_code ec;
        if ( !fs::is_regular_file( path, ec ) || ec ) return false;
        if ( fs::file_size( path, ec ) < 128 || ec ) return false;

        // reject files that are not real sfnt fonts early - feeding garbage to
        // the atlas builder would trip ImGui asserts
        std::ifstream file( path, std::ios::binary );
        if ( !file ) return false;
        unsigned char header[ 4 ] = {};
        file.read( reinterpret_cast< char* >( header ), 4 );
        if ( file.gcount( ) != 4 ) return false;

        const unsigned long tag = ( ( unsigned long )header[ 0 ] << 24 ) | ( ( unsigned long )header[ 1 ] << 16 )
            | ( ( unsigned long )header[ 2 ] << 8 ) | ( unsigned long )header[ 3 ];
        return tag == 0x00010000ul  // ttf
            || tag == 0x74727565ul  // 'true'
            || tag == 0x74746366ul  // 'ttcf' collection
            || tag == 0x4F54544Ful; // 'OTTO' otf
    }

    void scan_available_fonts( ) {
        namespace fs = std::filesystem;
        g_available_fonts.clear( );

        const fs::path module_dir = resolve_module_directory( );
        if ( !module_dir.empty( ) ) {
            std::error_code ec;
            const fs::path fonts_dir = module_dir / "fonts";
            if ( fs::is_directory( fonts_dir, ec ) && !ec ) {
                std::vector< available_font > found;
                for ( const auto& entry : fs::directory_iterator( fonts_dir, ec ) ) {
                    if ( ec ) break;
                    if ( !entry.is_regular_file( ec ) || ec ) continue;
                    const fs::path path = entry.path( );
                    std::string ext = to_lower_ascii( path.extension( ).string( ) );
                    if ( ext != ".ttf" && ext != ".otf" && ext != ".ttc" ) continue;
                    if ( !looks_like_font_file( path ) ) continue;
                    found.push_back( { path.stem( ).string( ), path } );
                }

                std::sort( found.begin( ), found.end( ), []( const available_font& a, const available_font& b ) {
                    return to_lower_ascii( a.name ) < to_lower_ascii( b.name );
                } );
                g_available_fonts = std::move( found );
            }
        }

        // re-resolve the active / requested selections by name in case the
        // folder changed between scans ( files added/removed while running )
        g_selected_font = -1;
        g_requested_font = -1;
        for ( int i = 0; i < ( int )g_available_fonts.size( ); ++i ) {
            if ( g_available_fonts[ i ].name == g_selected_font_name ) g_selected_font = i;
            if ( g_available_fonts[ i ].name == g_requested_font_name ) g_requested_font = i;
        }
    }

    // ------------------------------------------------------------------
    // background preparation: the worker only does CPU work ( file read +
    // stb rasterization into a private ImFontAtlas ). ImGui atlas objects are
    // self-contained, the shared atlas and the renderer are not touched here.
    // ------------------------------------------------------------------
    // SEH-isolated atlas build: a broken font must never wedge the worker
    // thread inside stb ( an unhandled AV here would kill the unload chain );
    // kept free of C++ locals so __try is allowed
    static bool atlas_build_seh( ImFontAtlas* atlas ) {
        __try {
            return atlas->Build( ) ? true : false;
        }
        __except ( EXCEPTION_EXECUTE_HANDLER ) {
            return false;
        }
    }

    void prepare_font_asset( font_asset* asset ) {
        void* file_data = nullptr;
        int file_size = 0;
        if ( !c_font::read_font_file( asset->path, &file_data, &file_size ) ) {
            asset->state.store( font_asset_failed );
            return;
        }

        ImFontAtlas* atlas = IM_NEW( ImFontAtlas );
        const ImWchar* ranges = atlas->GetGlyphRangesCyrillic( );
        static const float sizes[] = { 9.f, 11.f, 12.f, 13.f, 16.f };

        bool ok = true;
        for ( int i = 0; i < ( int )( sizeof( sizes ) / sizeof( sizes[ 0 ] ) ); ++i ) {
            ImFontConfig config;
            config.PixelSnapH = true;
            // the first add hands the buffer over to the atlas ( freed on
            // destruction ), the rest make imgui copy it - the same buffer
            // must never be owned twice
            config.FontDataOwnedByAtlas = ( i == 0 );
            ImFont* font = atlas->AddFontFromMemoryTTF( file_data, file_size, sizes[ i ] * dpi_scale, &config, ranges );
            if ( font )
                asset->by_size[ sizes[ i ] ] = font;
            else {
                ok = false;
                break;
            }
        }

        if ( ok )
            ok = atlas_build_seh( atlas );

        if ( !ok ) {
            IM_DELETE( atlas );   // frees the owned file buffer and the copies
            asset->by_size.clear( );
            asset->state.store( font_asset_failed );
            return;
        }

        asset->atlas = atlas;
        asset->state.store( font_asset_ready );
    }

    void font_worker_loop( ) {
        for ( ;; ) {
            font_asset* job = nullptr;
            {
                std::unique_lock< std::mutex > lock( g_font_job_mutex );
                g_font_job_cv.wait( lock, [ ]( ) { return !g_font_job_queue.empty( ) || g_font_worker_stop; } );
                if ( g_font_job_queue.empty( ) ) {
                    if ( g_font_worker_stop ) {
                        g_font_worker_exited.store( true );
                        return;
                    }
                    continue;
                }
                job = g_font_job_queue.front( );
                g_font_job_queue.pop_front( );
            }

            if ( job->state.load( ) == font_asset_loading )
                prepare_font_asset( job );
        }
    }

    void ensure_font_worker( ) {
        if ( g_font_worker.joinable( ) )
            return;
        g_font_worker_stop = false;
        g_font_worker_exited.store( false );
        g_font_worker = std::thread( font_worker_loop );
    }

    void stop_font_worker( ) {
        {
            std::lock_guard< std::mutex > lock( g_font_job_mutex );
            g_font_worker_stop = true;
            g_font_job_queue.clear( );
        }
        g_font_job_cv.notify_all( );

        // bounded wait: normally the worker exits in microseconds; if a font
        // build somehow wedged, detach instead of blocking the game's unload
        // chain forever ( FreeLibraryAndExitThread would never be reached )
        for ( int i = 0; i < 60; ++i ) {
            if ( g_font_worker_exited.load( ) )
                break;
            Sleep( 50 );
        }

        if ( g_font_worker.joinable( ) ) {
            if ( g_font_worker_exited.load( ) )
                g_font_worker.join( );
            else
                g_font_worker.detach( );
        }
    }

    void enqueue_font_job( font_asset* asset ) {
        {
            std::lock_guard< std::mutex > lock( g_font_job_mutex );
            for ( font_asset* queued : g_font_job_queue )
                if ( queued == asset ) return;   // already queued
            g_font_job_queue.push_back( asset );
        }
        g_font_job_cv.notify_one( );
    }

    font_asset* get_or_create_font_asset( const std::string& name, const std::filesystem::path& path ) {
        if ( font_asset* existing = find_font_asset( name ) )
            return existing;

        font_asset* asset = new font_asset( );
        asset->name = name;
        asset->path = path;
        g_font_assets.push_back( asset );
        return asset;
    }

    void activate_font( int index ) {
        g_selected_font = index;
        g_selected_font_name = g_available_fonts[ index ].name;
        if ( font_asset* asset = find_font_asset( g_selected_font_name ) )
            asset->last_used = ImGui::GetTime( );
    }

    // Renders the custom atlas pixels into a private D3D11 texture. Runs on
    // the render thread only, one small texture per font, once per font.
    void upload_font_asset_texture( font_asset* asset ) {
        if ( !g_device || !asset->atlas ) {
            asset->state.store( font_asset_failed );
            return;
        }

        unsigned char* pixels = nullptr;
        int width = 0, height = 0, bytes_per_pixel = 0;
        asset->atlas->GetTexDataAsRGBA32( &pixels, &width, &height, &bytes_per_pixel );
        if ( !pixels || width <= 0 || height <= 0 ) {
            asset->state.store( font_asset_failed );
            return;
        }

        D3D11_TEXTURE2D_DESC desc{};
        desc.Width = ( UINT )width;
        desc.Height = ( UINT )height;
        desc.MipLevels = 1;
        desc.ArraySize = 1;
        desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.SampleDesc.Count = 1;
        desc.Usage = D3D11_USAGE_IMMUTABLE;
        desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

        D3D11_SUBRESOURCE_DATA init{};
        init.pSysMem = pixels;
        init.SysMemPitch = ( UINT )width * 4;

        ID3D11Texture2D* texture = nullptr;
        if ( FAILED( g_device->CreateTexture2D( &desc, &init, &texture ) ) || !texture ) {
            asset->state.store( font_asset_failed );
            return;
        }

        D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc{};
        srv_desc.Format = desc.Format;
        srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        srv_desc.Texture2D.MipLevels = 1;
        const HRESULT srv_result = g_device->CreateShaderResourceView( texture, &srv_desc, &asset->srv );
        texture->Release( );
        if ( FAILED( srv_result ) || !asset->srv ) {
            asset->state.store( font_asset_failed );
            return;
        }

        asset->atlas->SetTexID( ( ImTextureID )asset->srv );

        // glyphs + metrics live in the ImFont objects, the baked pixels are
        // not needed once the GPU texture exists - free a few MB per font
        asset->atlas->ClearTexData( );
    }

    void release_font_asset_gpu( font_asset* asset ) {
        if ( asset->srv ) {
            asset->srv->Release( );
            asset->srv = nullptr;
        }
        if ( asset->atlas ) {
            IM_DELETE( asset->atlas );
            asset->atlas = nullptr;
        }
        asset->by_size.clear( );
        asset->state.store( font_asset_empty );
    }

    bool is_font_asset_in_use( font_asset* asset ) {
        if ( g_selected_font >= 0 && g_selected_font < ( int )g_available_fonts.size( )
            && g_available_fonts[ g_selected_font ].name == asset->name ) return true;
        if ( g_requested_font >= 0 && g_requested_font < ( int )g_available_fonts.size( )
            && g_available_fonts[ g_requested_font ].name == asset->name ) return true;
        return false;
    }

    // keeps the resident-font cache bounded; assets being prepared are never
    // touched, eviction just downgrades the struct so the worker can rebuild
    // it later without any lifetime risk
    void evict_font_assets( ) {
        int ready_count = 0;
        for ( font_asset* asset : g_font_assets )
            if ( asset->state.load( ) == font_asset_ready && asset->srv )
                ++ready_count;

        if ( ready_count <= k_max_ready_font_assets )
            return;

        font_asset* victim = nullptr;
        for ( font_asset* asset : g_font_assets ) {
            if ( asset->state.load( ) != font_asset_ready || !asset->srv ) continue;
            if ( is_font_asset_in_use( asset ) ) continue;
            if ( !victim || asset->last_used < victim->last_used )
                victim = asset;
        }

        if ( victim )
            release_font_asset_gpu( victim );
    }

    // render-thread side of the pipeline: uploads textures for finished
    // assets, activates the requested font once it is usable, reverts on
    // failures and keeps the cache bounded
    void process_font_assets( ) {
        for ( font_asset* asset : g_font_assets ) {
            if ( asset->state.load( ) == font_asset_ready && !asset->srv )
                upload_font_asset_texture( asset );
        }

        if ( g_requested_font >= 0 && g_requested_font != g_selected_font
            && g_requested_font < ( int )g_available_fonts.size( ) ) {
            font_asset* asset = find_font_asset( g_available_fonts[ g_requested_font ].name );
            if ( asset ) {
                if ( asset->state.load( ) == font_asset_ready && asset->srv ) {
                    activate_font( g_requested_font );
                } else if ( asset->state.load( ) == font_asset_failed ) {
                    g_requested_font = g_selected_font;
                    g_requested_font_name = g_selected_font_name;
                    notify::add( "font failed to load", notify::notify_error );
                }
            } else {
                g_requested_font = g_selected_font;
                g_requested_font_name = g_selected_font_name;
            }
        }

        evict_font_assets( );
    }

    void request_menu_font( int index ) {
        if ( index >= ( int )g_available_fonts.size( ) ) index = -1;
        if ( index == g_requested_font ) return;

        if ( index < 0 ) {
            // back to the built-in font - just a pointer swap, instant
            g_requested_font = -1;
            g_requested_font_name.clear( );
            g_selected_font = -1;
            g_selected_font_name.clear( );
            config::update(
                std::string( ),
                std::string( "voiden" ), std::string( "menu_font" ), std::string( ) );
            return;
        }

        g_requested_font = index;
        g_requested_font_name = g_available_fonts[ index ].name;
        config::update(
            g_requested_font_name,
            std::string( "voiden" ), std::string( "menu_font" ), std::string( ) );

        font_asset* asset = get_or_create_font_asset( g_available_fonts[ index ].name, g_available_fonts[ index ].path );
        asset->path = g_available_fonts[ index ].path;

        const int state = asset->state.load( );
        if ( state == font_asset_ready && asset->srv ) {
            activate_font( index );   // cached - zero heavy work
        } else if ( state == font_asset_empty || state == font_asset_failed ) {
            asset->state.store( font_asset_loading );
            enqueue_font_job( asset );
        }
        // font_asset_loading: already queued or being prepared on the worker
    }

    // restores the font picked in a previous session; config can still be
    // empty during early init, so this retries until the value shows up.
    // runs through the same async pipeline, the menu is never blocked
    bool g_saved_font_resolved = false;
    void try_apply_saved_menu_font( ) {
        if ( g_saved_font_resolved ) return;

        const std::string saved_font = config::get( std::string( "voiden" ), std::string( "menu_font" ), std::string( ) );
        if ( saved_font.empty( ) ) return;

        g_saved_font_resolved = true;
        for ( int i = 0; i < ( int )g_available_fonts.size( ); ++i ) {
            if ( g_available_fonts[ i ].name == saved_font ) {
                request_menu_font( i );
                return;
            }
        }
    }

    bool load_texture_from_file( ID3D11Device* device, const std::filesystem::path& path, ID3D11ShaderResourceView** out_srv, int* out_w, int* out_h ) {
        *out_srv = nullptr;
        *out_w = 0;
        *out_h = 0;
        if ( !device ) return false;

        std::ifstream file( path, std::ios::binary | std::ios::ate );
        if ( !file ) return false;
        const std::streamsize file_size = file.tellg( );
        if ( file_size <= 0 ) return false;
        file.seekg( 0, std::ios::beg );
        std::vector< unsigned char > data( static_cast< size_t >( file_size ) );
        if ( !file.read( reinterpret_cast< char* >( data.data( ) ), file_size ) ) return false;

        IWICImagingFactory* factory = nullptr;
        IWICStream* stream = nullptr;
        IWICBitmapDecoder* decoder = nullptr;
        IWICBitmapFrameDecode* frame = nullptr;
        IWICFormatConverter* converter = nullptr;
        bool ok = false;

        if ( SUCCEEDED( CoCreateInstance( CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS( &factory ) ) ) && factory ) {
            do {
                if ( FAILED( factory->CreateStream( &stream ) ) || !stream ) break;
                if ( FAILED( stream->InitializeFromMemory( data.data( ), static_cast< DWORD >( data.size( ) ) ) ) ) break;
                if ( FAILED( factory->CreateDecoderFromStream( stream, nullptr, WICDecodeMetadataCacheOnDemand, &decoder ) ) || !decoder ) break;
                if ( FAILED( decoder->GetFrame( 0, &frame ) ) || !frame ) break;
                if ( FAILED( factory->CreateFormatConverter( &converter ) ) || !converter ) break;
                if ( FAILED( converter->Initialize( frame, GUID_WICPixelFormat32bppBGRA, WICBitmapDitherTypeNone, nullptr, 0.0, WICBitmapPaletteTypeCustom ) ) ) break;

                UINT width = 0;
                UINT height = 0;
                if ( FAILED( converter->GetSize( &width, &height ) ) || width == 0 || height == 0 ) break;

                const UINT row_pitch = width * 4;
                std::vector< unsigned char > pixels( static_cast< size_t >( row_pitch ) * height );
                if ( FAILED( converter->CopyPixels( nullptr, row_pitch, static_cast< UINT >( pixels.size( ) ), pixels.data( ) ) ) ) break;

                D3D11_TEXTURE2D_DESC desc{};
                desc.Width = width;
                desc.Height = height;
                desc.MipLevels = 1;
                desc.ArraySize = 1;
                desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
                desc.SampleDesc.Count = 1;
                desc.Usage = D3D11_USAGE_DEFAULT;
                desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

                D3D11_SUBRESOURCE_DATA init{};
                init.pSysMem = pixels.data( );
                init.SysMemPitch = row_pitch;

                ID3D11Texture2D* texture = nullptr;
                if ( FAILED( device->CreateTexture2D( &desc, &init, &texture ) ) || !texture ) break;

                D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc{};
                srv_desc.Format = desc.Format;
                srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
                srv_desc.Texture2D.MipLevels = 1;
                if ( FAILED( device->CreateShaderResourceView( texture, &srv_desc, out_srv ) ) ) {
                    texture->Release( );
                    break;
                }
                texture->Release( );

                *out_w = static_cast< int >( width );
                *out_h = static_cast< int >( height );
                ok = true;
            } while ( false );
        }

        if ( converter ) converter->Release( );
        if ( frame ) frame->Release( );
        if ( decoder ) decoder->Release( );
        if ( stream ) stream->Release( );
        if ( factory ) factory->Release( );
        return ok;
    }

    // ------------------------------------------------------------------
    // config helpers (same keys as the legacy menu)
    // ------------------------------------------------------------------
    bool cfg_bool( const char* category, const char* key, bool default_value ) {
        return config::get( category, key, default_value ? 1 : 0 ) != 0;
    }

    void set_cfg_bool( const char* category, const char* key, bool default_value, bool value ) {
        config::update( value ? 1 : 0, category, key, default_value ? 1 : 0 );
    }

    // ------------------------------------------------------------------
    // widgets
    // ------------------------------------------------------------------
    bool nav_item( const char* str_id, const char* icon_glyph, const char* label, bool selected, float width ) {
        ImGuiWindow* window = ImGui::GetCurrentWindow( );
        const ImGuiID id = window->GetID( str_id );
        const ImVec2 p = window->DC.CursorPos;
        const float height = SCALE( 42 );
        const ImRect bb( p, ImVec2( p.x + width, p.y + height ) );
        ImGui::ItemSize( bb, 0.f );
        ImGui::ItemAdd( bb, id );

        bool hovered = false;
        bool held = false;
        const bool pressed = ImGui::ButtonBehavior( bb, id, &hovered, &held );

        struct nav_anim { float sel = 0.f; float hover = 0.f; };
        auto& st = anim_obj( str_id, 4071, nav_anim{ } );
        st.sel = anim( st.sel, 0.f, 1.f, selected );
        st.hover = anim( st.hover, 0.f, 1.f, hovered );

        ImDrawList* draw_list = window->DrawList;
        if ( st.sel > 0.004f ) {
            draw_list->AddRectFilled( bb.Min, bb.Max, fade( lerp_col( 0, col_nav_active, st.sel ) ), SCALE( 8 ) );
            draw_list->AddRect( bb.Min, bb.Max, fade( lerp_col( 0, col_panel_border, st.sel ) ), SCALE( 8 ) );
        } else if ( st.hover > 0.004f ) {
            draw_list->AddRectFilled( bb.Min, bb.Max, fade( lerp_col( 0, col_nav_hover, st.hover ) ), SCALE( 8 ) );
        }

        const float content_left = bb.Min.x + SCALE( 14 );
        const float focus = ImMax( st.sel, st.hover );
        const ImU32 item_col = fade( lerp_col( col_text_dim, col_text, focus ) );

        if ( ImFont* icon_font = get_font( icons, 15 ) ) {
            const ImVec2 ts = icon_font->CalcTextSizeA( icon_font->FontSize, FLT_MAX, 0.f, icon_glyph );
            draw_list->AddText( icon_font, icon_font->FontSize, ImVec2( content_left, bb.GetCenter( ).y - ts.y * 0.5f ), item_col, icon_glyph );
        }
        if ( ImFont* fnt = get_font( fontb, 13 ) ) {
            draw_text_impl( draw_list, fnt, ImVec2( content_left + SCALE( 30 ), bb.GetCenter( ).y - fnt->FontSize * 0.5f ), item_col, label, nullptr );
        }

        return pressed;
    }

    // text color dimmed by the Enabled state of the block ( 0 = normal )
    ImU32 dim_text_col( ImU32 col, float dim ) {
        return fade( lerp_col( col, col_text_faint, ImSaturate( dim ) ) );
    }

    ImU32 dim_border_col( ImU32 col, float dim ) {
        return fade( lerp_col( col, col_panel_border, ImSaturate( dim ) ) );
    }

    bool row_toggle( const char* str_id, const char* label, bool* value, float width, float dim = 0.f, bool active = true, float label_indent = 0.f, float hit_inset_left = 0.f ) {
        ImGuiWindow* window = ImGui::GetCurrentWindow( );
        const ImGuiID id = window->GetID( str_id );
        const ImVec2 p = window->DC.CursorPos;
        const float height = SCALE( g_metrics.row_h );
        const ImRect bb( p, ImVec2( p.x + width, p.y + height ) );
        // hit_inset_left shrinks the interactive area: the gear icon of the
        // bindable Enable rows owns that strip - overlapping hit rects steal
        // each other's hover ( HoveredId check ) and block the clicks
        const ImRect hit_bb( bb.Min.x + hit_inset_left, bb.Min.y, bb.Max.x, bb.Max.y );
        ImGui::ItemSize( bb, 0.f );
        ImGui::ItemAdd( hit_bb, id );

        bool hovered = false;
        bool held = false;
        bool pressed = false;
        if ( active ) {
            pressed = ImGui::ButtonBehavior( hit_bb, id, &hovered, &held );
            if ( pressed ) *value = !*value;
        }

        struct toggle_anim { float t = 0.f; float hover = 0.f; };
        auto& st = anim_obj( str_id, 4072, toggle_anim{ } );
        st.t = anim( st.t, 0.f, 1.f, *value );
        st.hover = anim( st.hover, 0.f, 1.f, hovered );

        ImDrawList* draw_list = window->DrawList;
        // same color scheme as every other row label: dim base, hover highlight
        const ImU32 text_col = dim_text_col( lerp_col( col_text_dim, col_text, st.hover ), dim );

        const float sw_h = ImMin( SCALE( 20 ), SCALE( g_metrics.row_h - 4.f ) );
        const float sw_w = sw_h * 1.9f;
        draw_row_label( str_id, draw_list, ImVec2( bb.Min.x + label_indent, bb.Min.y ),
            ImMax( SCALE( 10 ), width - label_indent - sw_w - SCALE( 8 ) ), text_col, label, height );

        const ImVec2 smin( bb.Max.x - sw_w, bb.GetCenter( ).y - sw_h * 0.5f );
        const ImVec2 smax( bb.Max.x, smin.y + sw_h );
        const ImU32 track_off = dim_border_col( col_track_off, dim );
        draw_list->AddRectFilled( smin, smax, fade( lerp_col( track_off, col_track_on, st.t ) ), sw_h * 0.5f );

        const float knob_d = sw_h - SCALE( 6 );
        const float knob_x = ImLerp( smin.x + SCALE( 3 ), smax.x - SCALE( 3 ) - knob_d, st.t );
        draw_list->AddCircleFilled( ImVec2( knob_x + knob_d * 0.5f, smin.y + sw_h * 0.5f ), knob_d * 0.5f, fade( lerp_col( col_knob_off, col_knob_on, st.t ) ), 24 );

        return pressed;
    }

    // one shared slider row for every block: animated value display so the
    // bar and the number glide instead of jumping
    template < typename T >
    bool row_slider_impl( const char* str_id, const char* label, T* value, T min, T max, const char* format, float width, float dim, bool active ) {
        const float min_f = ( float )min;
        const float max_f = ( float )max;

        ImGuiWindow* window = ImGui::GetCurrentWindow( );
        const ImGuiID id = window->GetID( str_id );
        const ImVec2 p = window->DC.CursorPos;
        const float height = SCALE( g_metrics.row_h );
        const ImRect bb( p, ImVec2( p.x + width, p.y + height ) );

        const float track_w = ImMin( width * 0.42f, SCALE( 150 ) );
        const float value_gap = SCALE( 12 );
        const ImVec2 tmin( bb.Max.x - track_w, bb.GetCenter( ).y - SCALE( 1.5f ) );
        const ImVec2 tmax( bb.Max.x, tmin.y + SCALE( 3 ) );
        const ImRect track_bb( tmin - ImVec2( SCALE( 8 ), SCALE( 12 ) ), tmax + ImVec2( SCALE( 8 ), SCALE( 12 ) ) );

        ImGui::ItemSize( bb, 0.f );
        ImGui::ItemAdd( bb, id, &track_bb );

        bool hovered = false;
        bool held = false;
        if ( active )
            ImGui::ButtonBehavior( track_bb, id, &hovered, &held );
        if ( held && active && tmax.x > tmin.x ) {
            const float dragged = min_f + ( ImGui::GetIO( ).MousePos.x - tmin.x ) / ( tmax.x - tmin.x ) * ( max_f - min_f );
            *value = ( T )ImClamp( dragged, ImMin( min_f, max_f ), ImMax( min_f, max_f ) );
        }

        struct slider_anim { float hover = 0.f; float display = 0.f; bool init = false; };
        auto& st = anim_obj( str_id, 4073, slider_anim{ } );
        st.hover = anim( st.hover, 0.f, 1.f, hovered && active );

        // smooth value indicator: follows the real value fast while dragging,
        // glides when the value changes from elsewhere
        if ( !st.init ) {
            st.display = ( float )*value;
            st.init = true;
        }
        if ( held && active ) st.display = ( float )*value;
        else st.display = ImLerp( st.display, ( float )*value, ImSaturate( ImGui::GetIO( ).DeltaTime * 14.f ) );

        ImDrawList* draw_list = window->DrawList;

        char buf[ 32 ];
        ImFormatString( buf, sizeof( buf ), format, ( T )st.display );
        const ImU32 value_col = dim_text_col( lerp_col( col_text_dim, col_text, st.hover ), dim );

        // same color scheme as every other row label: dim base, hover highlight;
        // the label never reaches the value text or the track
        const ImU32 label_col = dim_text_col( lerp_col( col_text_dim, col_text, st.hover ), dim );
        float value_text_w = SCALE( 24 );
        if ( ImFont* fnt = get_font( fontb, 12 ) ) {
            value_text_w = fnt->CalcTextSizeA( fnt->FontSize, FLT_MAX, 0.f, buf ).x;
        }
        const float label_max = ImMax( SCALE( 10 ), ( tmin.x - value_gap - value_text_w - SCALE( 8 ) ) - bb.Min.x );
        draw_row_label( str_id, draw_list, ImVec2( bb.Min.x, bb.Min.y ), label_max, label_col, label, height );

        if ( ImFont* fnt = get_font( fontb, 12 ) ) {
            const ImVec2 ts = fnt->CalcTextSizeA( fnt->FontSize, FLT_MAX, 0.f, buf );
            draw_text_impl( draw_list, fnt, ImVec2( tmin.x - value_gap - ts.x, bb.GetCenter( ).y - ts.y * 0.5f ), value_col, buf, nullptr );
        }

        const ImU32 line_col = dim_border_col( col_slider_line, dim );
        draw_list->AddRectFilled( tmin, tmax, line_col, SCALE( 1.5f ) );
        const float t = ( max_f > min_f ) ? ImSaturate( ( st.display - min_f ) / ( max_f - min_f ) ) : 0.f;
        const float knob_x = tmin.x + ( tmax.x - tmin.x ) * t;
        const ImU32 fill_col = dim_border_col( col_slider_fill, dim );
        if ( knob_x > tmin.x ) {
            draw_list->AddRectFilled( tmin, ImVec2( knob_x, tmax.y ), fill_col, SCALE( 1.5f ) );
        }
        draw_list->AddCircleFilled( ImVec2( knob_x, ( tmin.y + tmax.y ) * 0.5f ), SCALE( 5.5f ), fill_col, 20 );

        return held;
    }

    bool row_slider_float( const char* str_id, const char* label, float* value, float min, float max, const char* format, float width, float dim = 0.f, bool active = true ) {
        return row_slider_impl< float >( str_id, label, value, min, max, format, width, dim, active );
    }

    bool row_slider_int( const char* str_id, const char* label, int* value, int min, int max, const char* format, float width, float dim = 0.f, bool active = true ) {
        return row_slider_impl< int >( str_id, label, value, min, max, format, width, dim, active );
    }

    // ------------------------------------------------------------------
    // shared dropdown machinery ( same visual language as Settings -> Fonts )
    // ------------------------------------------------------------------
    struct dropdown_row_state {
        bool open = false;
    };

    // label on the left + value box on the right; returns true when pressed
    bool dropdown_box( const char* str_id, const char* label, const char* value_text, float width, float dim, bool open, ImRect* out_box, bool active = true ) {
        ImGuiWindow* window = ImGui::GetCurrentWindow( );
        const ImGuiID id = window->GetID( str_id );
        const ImVec2 p = window->DC.CursorPos;
        const float height = SCALE( g_metrics.row_h );
        const ImRect bb( p, ImVec2( p.x + width, p.y + height ) );

        const float box_w = ImMin( SCALE( 150 ), width );
        const float box_h = ImMin( SCALE( 22 ), SCALE( g_metrics.row_h - 3.f ) );
        const ImVec2 box_min( bb.Max.x - box_w, bb.GetCenter( ).y - box_h * 0.5f );
        const ImVec2 box_max( bb.Max.x, box_min.y + box_h );
        if ( out_box ) *out_box = ImRect( box_min, box_max );

        ImGui::ItemSize( bb, 0.f );
        ImGui::ItemAdd( bb, id );

        bool box_hovered = false;
        bool box_held = false;
        bool pressed = false;
        if ( active )
            pressed = ImGui::ButtonBehavior( ImRect( box_min, box_max ), id, &box_hovered, &box_held );

        struct box_anim { float hover = 0.f; };
        auto& st = anim_obj( str_id, 4077, box_anim{ } );
        st.hover = anim( st.hover, 0.f, 1.f, active && ( box_hovered || open ) );

        const bool focused = st.hover > 0.01f;
        ImDrawList* draw_list = window->DrawList;

        const ImU32 label_col = dim_text_col( lerp_col( col_text_dim, col_text, focused ? 1.f : 0.f ), dim );
        draw_row_label( str_id, draw_list, ImVec2( bb.Min.x, bb.Min.y ),
            ImMax( SCALE( 10 ), ( box_min.x - SCALE( 8 ) ) - bb.Min.x ), label_col, label, height );

        draw_list->AddRectFilled( box_min, box_max, fade( lerp_col( col_panel_bg, col_nav_hover, focused ? 1.f : 0.f ) ), SCALE( 6 ) );
        draw_list->AddRect( box_min, box_max, dim_border_col( lerp_col( col_panel_border, col_text, focused ? 1.f : 0.f ), dim ), SCALE( 6 ) );

        draw_list->PushClipRect( box_min, box_max, true );
        if ( ImFont* fnt = get_font( font, 12 ) ) {
            draw_text_impl( draw_list, fnt, ImVec2( box_min.x + SCALE( 10 ), bb.GetCenter( ).y - fnt->FontSize * 0.5f ), dim_text_col( col_text, dim ), value_text, nullptr );
        }
        draw_list->PopClipRect( );

        {
            const ImU32 arrow_col = dim_text_col( lerp_col( col_text_dim, col_text, focused ? 1.f : 0.f ), dim );
            ui::rotate_start( );
            ui::arrow( { box_max.x - SCALE( 14 ), bb.GetCenter( ).y - SCALE( 2 ) }, arrow_col, ImGuiDir_Down, 5 );
            ui::rotate_end( open ? IM_PI : 0.f, ui::rotation_center( ) );
        }

        return pressed;
    }

    // animated dropdown list attached to a value box; returns the index of a
    // clicked item ( -1 when nothing was clicked this frame )
    int dropdown_popup( const char* popup_id, const ImRect& box, const char* const* items, int item_count,
                        const bool* checked, bool& open, float dim, bool close_on_pick, bool box_hovered ) {
        struct popup_anim { float anim = 0.f; };
        auto& st = anim_obj( popup_id, 4078, popup_anim{ } );
        st.anim = anim( st.anim, 0.f, 1.f, open );
        if ( st.anim > 0.01f )
            g_any_dropdown_open = true;

        if ( st.anim <= 0.01f )
            return -1;

        const float item_h = SCALE( 24 );
        const float item_gap = SCALE( 2 );
        const float pad = SCALE( 6 );
        const int visible_count = ImMin( item_count, 7 );
        const float list_h = visible_count * item_h + ( visible_count - 1 ) * item_gap + pad * 2.f;
        const float list_w = box.GetWidth( );

        const ImVec2 display = ImGui::GetIO( ).DisplaySize;
        ImVec2 list_min( box.Max.x - list_w, box.Max.y + SCALE( 6 ) + ( 1.f - st.anim ) * SCALE( 5 ) );
        if ( list_min.y + list_h > display.y - SCALE( 8 ) )
            list_min.y = box.Min.y - SCALE( 6 ) - list_h;
        list_min.x = ImClamp( list_min.x, SCALE( 8 ), ImMax( SCALE( 8 ), display.x - list_w - SCALE( 8 ) ) );

        ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, ImVec2( pad, pad ) );
        ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, ImVec2( 0, item_gap ) );
        ImGui::PushStyleVar( ImGuiStyleVar_WindowRounding, SCALE( 8 ) );
        ImGui::PushStyleVar( ImGuiStyleVar_ScrollbarSize, SCALE( 3 ) );
        ImGui::SetNextWindowPos( list_min );
        ImGui::SetNextWindowSize( ImVec2( list_w, list_h ) );
        ImGui::Begin( popup_id, nullptr,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBackground );

        ImGuiWindow* list_window = ImGui::GetCurrentWindow( );

        // fade( ) covers the menu alpha, this adds the open/close fade
        auto popup_fade = [ & ]( ImU32 col ) {
            col = fade( col );
            const ImU32 a = ( ImU32 )( ( float )( ( col >> 24 ) & 0xFF ) * st.anim );
            return ( col & 0x00FFFFFFu ) | ( a << 24 );
        };

        list_window->DrawList->AddRectFilled( list_window->Pos, list_window->Pos + list_window->Size,
            popup_fade( col_panel_bg ), SCALE( 8 ) );
        list_window->DrawList->AddRect( list_window->Pos, list_window->Pos + list_window->Size,
            popup_fade( col_panel_border ), SCALE( 8 ) );

        if ( ImGui::IsMouseClicked( 0 ) && !ImGui::IsWindowHovered( ) && !box_hovered )
            open = false;

        int picked = -1;
        const float list_width = list_window->Size.x - pad * 2.f;
        for ( int i = 0; i < item_count; ++i ) {
            const std::string item_id = std::string( popup_id ) + "_item_" + std::to_string( i );
            const ImGuiID item_hash = ImGui::GetID( item_id.c_str( ) );

            const ImVec2 item_pos = ImGui::GetCursorScreenPos( );
            const ImRect item_bb( item_pos, ImVec2( item_pos.x + list_width, item_pos.y + item_h ) );
            ImGui::ItemSize( item_bb, 0.f );
            ImGui::ItemAdd( item_bb, item_hash );

            bool item_hovered = false;
            bool item_held = false;
            const bool item_pressed = ImGui::ButtonBehavior( item_bb, item_hash, &item_hovered, &item_held );

            struct item_anim { float hover = 0.f; };
            auto& rst = anim_obj( item_id.c_str( ), 4079, item_anim{ } );
            rst.hover = anim( rst.hover, 0.f, 1.f, item_hovered );

            const bool is_checked = checked && checked[ i ];
            if ( is_checked ) {
                list_window->DrawList->AddRectFilled( item_bb.Min, item_bb.Max,
                    popup_fade( col_nav_active ), SCALE( 6 ) );
            } else if ( rst.hover > 0.004f ) {
                list_window->DrawList->AddRectFilled( item_bb.Min, item_bb.Max,
                    popup_fade( lerp_col( 0, col_nav_hover, rst.hover ) ), SCALE( 6 ) );
            }

            const ImU32 text_col = popup_fade( lerp_col( col_text_dim, col_text, is_checked ? 1.f : rst.hover ) );
            if ( ImFont* fnt = get_font( font, 12 ) ) {
                draw_text_impl( list_window->DrawList, fnt,
                    ImVec2( item_bb.Min.x + SCALE( 10 ), item_bb.GetCenter( ).y - fnt->FontSize * 0.5f ),
                    text_col, items[ i ], nullptr );
            }
            if ( ImFont* icon_font = get_font( icons, 12 ) ) {
                if ( is_checked ) {
                    const ImVec2 ts = icon_font->CalcTextSizeA( icon_font->FontSize, FLT_MAX, 0.f, check_line );
                    list_window->DrawList->AddText( icon_font, icon_font->FontSize,
                        ImVec2( item_bb.Max.x - SCALE( 10 ) - ts.x, item_bb.GetCenter( ).y - ts.y * 0.5f ),
                        popup_fade( col_text ), check_line );
                }
            }

            if ( item_pressed ) {
                picked = i;
                if ( close_on_pick )
                    open = false;
            }
        }

        ImGui::BringWindowToFocusFront( list_window );
        ImGui::BringWindowToDisplayFront( list_window );
        ImGui::End( );
        ImGui::PopStyleVar( 4 );

        ( void )dim;
        return picked;
    }

    // single-choice dropdown row ( Activation Mode, Selection, ... ).
    // option_values maps a display index to the stored config value
    // ( pass nullptr for identity mapping )
    bool row_choice( const char* str_id, const char* label, const char* const* items, int item_count, const int* option_values, int* value, float width, float dim = 0.f, bool active = true ) {
        auto& st = anim_obj( str_id, 4080, dropdown_row_state{ } );
        if ( !active )
            st.open = false;

        int current_index = 0;
        for ( int i = 0; i < item_count; ++i ) {
            const int stored = option_values ? option_values[ i ] : i;
            if ( stored == *value ) {
                current_index = i;
                break;
            }
        }

        ImRect box( 0, 0, 0, 0 );
        const bool pressed = dropdown_box( str_id, label, items[ current_index ], width, dim, st.open, &box, active );
        if ( pressed && active )
            st.open = !st.open;

        bool checked[ 32 ] = {};
        for ( int i = 0; i < item_count && i < 32; ++i )
            checked[ i ] = i == current_index;

        const std::string popup_id = std::string( str_id ) + "_popup";
        const int picked = dropdown_popup( popup_id.c_str( ), box, items, item_count, checked, st.open, dim, true, false );
        if ( picked >= 0 )
            *value = option_values ? option_values[ picked ] : picked;

        return picked >= 0;
    }

    // ------------------------------------------------------------------
    // key bind field: click -> "..." waiting state -> press any key/mouse
    // button; right click -> activation mode dropdown. Writes the very same
    // config keys the legacy menu used ( binds/... + legacy aimbot/... )
    // ------------------------------------------------------------------
    std::string g_key_listening;
    int g_key_listen_frames = 0;

    const char* key_display_name( int vk, char* buf, size_t buf_size ) {
        if ( vk <= 0 ) {
            strcpy_s( buf, buf_size, "none" );
            return buf;
        }
        if ( vk >= VK_LBUTTON && vk <= VK_XBUTTON2 ) {
            sprintf_s( buf, buf_size, "mouse %d", vk - VK_LBUTTON + 1 );
            return buf;
        }
        if ( vk >= '0' && vk <= '9' ) { buf[ 0 ] = ( char )vk; buf[ 1 ] = 0; return buf; }
        if ( vk >= 'A' && vk <= 'Z' ) { buf[ 0 ] = ( char )vk; buf[ 1 ] = 0; return buf; }
        if ( vk >= VK_F1 && vk <= VK_F12 ) { sprintf_s( buf, buf_size, "f%d", vk - VK_F1 + 1 ); return buf; }
        switch ( vk ) {
            case VK_SPACE: strcpy_s( buf, buf_size, "space" ); return buf;
            case VK_CONTROL: strcpy_s( buf, buf_size, "ctrl" ); return buf;
            case VK_MENU: strcpy_s( buf, buf_size, "alt" ); return buf;
            case VK_SHIFT: strcpy_s( buf, buf_size, "shift" ); return buf;
            case VK_TAB: strcpy_s( buf, buf_size, "tab" ); return buf;
            case VK_CAPITAL: strcpy_s( buf, buf_size, "caps" ); return buf;
            case VK_INSERT: strcpy_s( buf, buf_size, "ins" ); return buf;
            case VK_DELETE: strcpy_s( buf, buf_size, "del" ); return buf;
            case VK_HOME: strcpy_s( buf, buf_size, "home" ); return buf;
            case VK_END: strcpy_s( buf, buf_size, "end" ); return buf;
            case VK_PRIOR: strcpy_s( buf, buf_size, "pgup" ); return buf;
            case VK_NEXT: strcpy_s( buf, buf_size, "pgdn" ); return buf;
            case VK_LEFT: strcpy_s( buf, buf_size, "left" ); return buf;
            case VK_RIGHT: strcpy_s( buf, buf_size, "right" ); return buf;
            case VK_UP: strcpy_s( buf, buf_size, "up" ); return buf;
            case VK_DOWN: strcpy_s( buf, buf_size, "down" ); return buf;
            case VK_BACK: strcpy_s( buf, buf_size, "back" ); return buf;
            case VK_RETURN: strcpy_s( buf, buf_size, "enter" ); return buf;
            default: sprintf_s( buf, buf_size, "key 0x%02x", vk ); return buf;
        }
    }

    void save_bind_config( const char* binds_key, const char* binds_mode, const char* legacy_key, const char* legacy_mode, int key, int mode ) {
        config::update( key, std::string( "binds" ), std::string( binds_key ), 0 );
        config::update( mode, std::string( "binds" ), std::string( binds_mode ), 0 );
        config::update( key, std::string( "aimbot" ), std::string( legacy_key ), 0 );
        config::update( mode, std::string( "aimbot" ), std::string( legacy_mode ), 0 );
    }

    int read_bind_config_key( const char* binds_key, const char* legacy_key ) {
        int key = config::get( std::string( "binds" ), std::string( binds_key ), 0 );
        if ( key == 0 )
            key = config::get( std::string( "aimbot" ), std::string( legacy_key ), 0 );
        return key;
    }

    int read_bind_config_mode( const char* binds_mode, const char* legacy_mode ) {
        int mode = config::get( std::string( "binds" ), std::string( binds_mode ), 0 );
        if ( mode == 0 )
            mode = config::get( std::string( "aimbot" ), std::string( legacy_mode ), 0 );
        if ( mode < 0 || mode > 2 )
            mode = 0;
        return mode;
    }

    // mode names match the legacy storage: 0 = toggle, 1 = hold, 2 = always on
    const char* const k_activation_mode_items[ 3 ] = { "Hold", "Toggle", "Always On" };
    int activation_mode_to_index( int mode ) {
        if ( mode == 1 ) return 0;
        if ( mode == 2 ) return 2;
        return 1;
    }
    int activation_index_to_mode( int index ) {
        if ( index == 0 ) return 1;
        if ( index == 2 ) return 2;
        return 0;
    }

    bool row_key_bind( const char* str_id, const char* label, const char* binds_key, const char* binds_mode,
                       const char* legacy_key, const char* legacy_mode, float width, float dim = 0.f, bool allow_mode_menu = true, bool active = true ) {
        auto& st = anim_obj( str_id, 4080, dropdown_row_state{ } );
        if ( !active )
            st.open = false;

        int key = read_bind_config_key( binds_key, legacy_key );
        int mode = read_bind_config_mode( binds_mode, legacy_mode );

        const bool listening = g_key_listening == str_id;
        char key_buf[ 16 ];
        const char* value_text = listening ? "" : key_display_name( key, key_buf, sizeof( key_buf ) );

        ImRect box( 0, 0, 0, 0 );
        const bool pressed = dropdown_box( str_id, label, value_text, width, dim, false, &box, active );

        ImGuiWindow* window = ImGui::GetCurrentWindow( );
        bool box_hovered = false;
        if ( active && ImGui::IsMouseHoveringRect( box.Min, box.Max ) )
            box_hovered = true;

        // waiting state: three white dots instead of a key name
        if ( listening ) {
            ImDrawList* draw_list = window->DrawList;
            const float cy = ( box.Min.y + box.Max.y ) * 0.5f;
            const float cx = ( box.Min.x + box.Max.x ) * 0.5f - SCALE( 6 );
            for ( int i = 0; i < 3; ++i )
                draw_list->AddCircleFilled( ImVec2( cx + i * SCALE( 6 ), cy ), SCALE( 1.7f ), fade( col_text ), 12 );
        }

        if ( pressed && active ) {
            g_key_listening = str_id;
            g_key_listen_frames = 2;
            // drain the "pressed since last call" flag of every virtual key so
            // the click that opened this field can never be captured itself
            // ( otherwise a stale left-click bit reads back as Mouse 1 )
            for ( int vk = 1; vk < 255; ++vk )
                GetAsyncKeyState( vk );
        }

        // right click on the field opens the activation mode dropdown
        if ( allow_mode_menu && box_hovered && ImGui::IsMouseClicked( 1 ) && active )
            st.open = !st.open;

        if ( listening ) {
            if ( !active ) {
                // the block got disabled while waiting - drop the capture
                g_key_listening.clear( );
            } else if ( g_key_listen_frames > 0 ) {
                --g_key_listen_frames;
            } else {
                for ( int vk = 1; vk < 255; ++vk ) {
                    const SHORT state = GetAsyncKeyState( vk );
                    // a fresh press only: toggle bit set AND key still down -
                    // stale bits from earlier clicks are ignored, so exactly
                    // one physical press is captured exactly once
                    if ( ( state & 1 ) == 0 || ( state & 0x8000 ) == 0 )
                        continue;
                    const int captured = vk == VK_ESCAPE ? 0 : vk;
                    save_bind_config( binds_key, binds_mode, legacy_key, legacy_mode, captured, mode );
                    g_key_listening.clear( );
                    break;
                }
            }
        }

        if ( st.open && allow_mode_menu && active ) {
            static const int mode_values[ 3 ] = { 1, 0, 2 };   // Hold, Toggle, Always On
            int index = activation_mode_to_index( mode );
            bool checked[ 3 ] = { index == 0, index == 1, index == 2 };
            const std::string popup_id = std::string( str_id ) + "_mode";
            const int picked = dropdown_popup( popup_id.c_str( ), box, k_activation_mode_items, 3, checked, st.open, dim, true, box_hovered );
            if ( picked >= 0 ) {
                mode = mode_values[ picked ];
                save_bind_config( binds_key, binds_mode, legacy_key, legacy_mode, key, mode );
            }
        }

        return pressed;
    }

    // multi-choice dropdown row ( bone selection ); writes the same
    // map<string,string> config layout as the legacy multi-select
    bool row_bones( const char* str_id, const char* label, const char* category, const char* key, float width, float dim = 0.f, bool active = true ) {
        static const char* const k_bone_ids[ 5 ] = { "head", "neck", "chest", "spine", "pelvis" };
        static const char* const k_bone_names[ 5 ] = { "Head", "Neck", "Chest", "Spine", "Pelvis" };

        auto& st = anim_obj( str_id, 4080, dropdown_row_state{ } );
        if ( !active )
            st.open = false;

        std::map< std::string, std::string > saved = config::get( std::string( category ), std::string( key ), std::map< std::string, std::string >( ) );
        bool checked[ 5 ] = {};
        bool any = false;
        for ( int i = 0; i < 5; ++i ) {
            auto it = saved.find( k_bone_ids[ i ] );
            checked[ i ] = it != saved.end( ) && it->second == "1";
            any |= checked[ i ];
        }
        if ( !any )
            checked[ 0 ] = true;   // legacy default: head

        char value_buf[ 64 ] = {};
        int total = 0;
        for ( int i = 0; i < 5; ++i ) {
            if ( !checked[ i ] ) continue;
            if ( total > 0 ) strcat_s( value_buf, ", " );
            strcat_s( value_buf, k_bone_names[ i ] );
            ++total;
        }

        ImRect box( 0, 0, 0, 0 );
        const bool pressed = dropdown_box( str_id, label, value_buf, width, dim, st.open, &box, active );
        if ( pressed && active )
            st.open = !st.open;

        const std::string popup_id = std::string( str_id ) + "_popup";
        const int picked = dropdown_popup( popup_id.c_str( ), box, k_bone_names, 5, checked, st.open, dim, false, false );
        if ( picked >= 0 && active ) {
            std::map< std::string, std::string > updated;
            for ( int i = 0; i < 5; ++i ) {
                const bool now = ( i == picked ) ? !checked[ i ] : checked[ i ];
                if ( now )
                    updated[ k_bone_ids[ i ] ] = "1";
            }
            if ( updated.empty( ) )
                updated[ k_bone_ids[ picked ] ] = "1";   // never allow an empty bone set

            config::update( updated, std::string( category ), std::string( key ), std::map< std::string, std::string >( ) );
        }

        return pressed;
    }

    // generic multi-choice dropdown: stores a map< id, "1"/"0" > config like
    // the bone selector; an unset map means "everything on". used for the
    // watermark components ( name / fps / date / time )
    bool row_multi_choice( const char* str_id, const char* label, const char* category, const char* key,
                           const char* const* ids, const char* const* names, int count, float width, float dim = 0.f, bool active = true ) {
        auto& st = anim_obj( str_id, 4080, dropdown_row_state{ } );
        if ( !active )
            st.open = false;

        std::map< std::string, std::string > saved = config::get( std::string( category ), std::string( key ), std::map< std::string, std::string >( ) );
        bool checked[ 16 ] = {};
        for ( int i = 0; i < count && i < 16; ++i ) {
            auto it = saved.find( ids[ i ] );
            checked[ i ] = it == saved.end( ) || it->second == "1";   // default: all on
        }

        char value_buf[ 96 ] = {};
        int selected = 0;
        for ( int i = 0; i < count && i < 16; ++i ) {
            if ( !checked[ i ] ) continue;
            if ( selected > 0 ) strcat_s( value_buf, ", " );
            strcat_s( value_buf, names[ i ] );
            ++selected;
        }
        if ( selected == 0 )
            strcpy_s( value_buf, "none" );

        ImRect box( 0, 0, 0, 0 );
        const bool pressed = dropdown_box( str_id, label, value_buf, width, dim, st.open, &box, active );
        if ( pressed && active )
            st.open = !st.open;

        const std::string popup_id = std::string( str_id ) + "_popup";
        const int picked = dropdown_popup( popup_id.c_str( ), box, names, count, checked, st.open, dim, false, false );
        if ( picked >= 0 && active ) {
            // write the complete state so an all-off selection is remembered
            std::map< std::string, std::string > updated;
            for ( int i = 0; i < count && i < 16; ++i )
                updated[ ids[ i ] ] = ( i == picked ? !checked[ i ] : checked[ i ] ) ? "1" : "0";
            config::update( updated, std::string( category ), std::string( key ), std::map< std::string, std::string >( ) );
        }

        return pressed;
    }

    // ------------------------------------------------------------------
    // font selector row: label on the left, value box on the right, the
    // popup list reuses the voiden palette ( panel bg, borders, nav hover )
    // ------------------------------------------------------------------
    bool row_font_dropdown( const char* str_id, const char* label, float width ) {
        ImGuiWindow* window = ImGui::GetCurrentWindow( );
        const ImGuiID id = window->GetID( str_id );
        const ImVec2 p = window->DC.CursorPos;
        const float height = SCALE( g_metrics.row_h );
        const ImRect bb( p, ImVec2( p.x + width, p.y + height ) );

        const float box_w = ImMin( SCALE( 150 ), width );
        const float box_h = SCALE( 22 );
        const ImVec2 box_min( bb.Max.x - box_w, bb.GetCenter( ).y - box_h * 0.5f );
        const ImVec2 box_max( bb.Max.x, box_min.y + box_h );
        const ImRect box_bb( box_min, box_max );

        ImGui::ItemSize( bb, 0.f );
        ImGui::ItemAdd( bb, id, &box_bb );

        bool box_hovered = false;
        bool box_held = false;
        const bool pressed = ImGui::ButtonBehavior( box_bb, id, &box_hovered, &box_held );

        struct dropdown_state { bool open = false; float anim = 0.f; };
        auto& st = anim_obj( str_id, 4075, dropdown_state{ } );
        if ( pressed ) {
            st.open = !st.open;
            if ( st.open ) {
                scan_available_fonts( );
                try_apply_saved_menu_font( );
            }
        }

        st.anim = anim( st.anim, 0.f, 1.f, st.open );
        if ( st.anim > 0.01f )
            g_any_dropdown_open = true;
        const bool focused = box_hovered || st.open;

        ImDrawList* draw_list = window->DrawList;

        const ImU32 label_col = fade( lerp_col( col_text_dim, col_text, focused ? 1.f : 0.f ) );
        draw_row_label( str_id, draw_list, ImVec2( bb.Min.x, bb.Min.y ),
            ImMax( SCALE( 10 ), ( box_min.x - SCALE( 8 ) ) - bb.Min.x ), label_col, label, height );

        draw_list->AddRectFilled( box_min, box_max, fade( lerp_col( col_panel_bg, col_nav_hover, focused ? 1.f : 0.f ) ), SCALE( 6 ) );
        draw_list->AddRect( box_min, box_max, fade( lerp_col( col_panel_border, col_text, focused ? 1.f : 0.f ) ), SCALE( 6 ) );

        const char* value_text = ( g_requested_font >= 0 && g_requested_font < ( int )g_available_fonts.size( ) )
            ? g_available_fonts[ g_requested_font ].name.c_str( )
            : "Default";
        draw_list->PushClipRect( box_min, box_max, true );
        if ( ImFont* fnt = get_font( font, 12 ) ) {
            draw_text_impl( draw_list, fnt, ImVec2( box_min.x + SCALE( 10 ), bb.GetCenter( ).y - fnt->FontSize * 0.5f ), fade( col_text ), value_text, nullptr );
        }
        draw_list->PopClipRect( );

        {
            const ImU32 arrow_col = fade( lerp_col( col_text_dim, col_text, focused ? 1.f : 0.f ) );
            ui::rotate_start( );
            ui::arrow( { box_max.x - SCALE( 14 ), box_bb.GetCenter( ).y - SCALE( 2 ) }, arrow_col, ImGuiDir_Down, 5 );
            ui::rotate_end( st.open ? IM_PI : 0.f, ui::rotation_center( ) );
        }

        if ( st.anim <= 0.01f ) {
            return false;
        }

        // ---------------- popup list ----------------
        bool value_changed = false;
        // fade( ) covers the global menu alpha, this adds the open/close fade
        auto popup_fade = [ & ]( ImU32 col ) {
            col = fade( col );
            const ImU32 a = ( ImU32 )( ( float )( ( col >> 24 ) & 0xFF ) * st.anim );
            return ( col & 0x00FFFFFFu ) | ( a << 24 );
        };

        const float item_h = SCALE( 26 );
        const float item_gap = SCALE( 2 );
        const float pad = SCALE( 6 );
        const int item_count = 1 + ( int )g_available_fonts.size( ); // Default + scanned fonts
        const int visible_count = ImMin( item_count, 7 );
        const float list_h = visible_count * item_h + ( visible_count - 1 ) * item_gap + pad * 2.f;
        const float list_w = box_w;

        const ImVec2 display = ImGui::GetIO( ).DisplaySize;
        ImVec2 list_min( box_max.x - list_w, box_max.y + SCALE( 6 ) + ( 1.f - st.anim ) * SCALE( 5 ) );
        if ( list_min.y + list_h > display.y - SCALE( 8 ) ) {
            list_min.y = box_bb.Min.y - SCALE( 6 ) - list_h;
        }
        list_min.x = ImClamp( list_min.x, SCALE( 8 ), ImMax( SCALE( 8 ), display.x - list_w - SCALE( 8 ) ) );

        const std::string list_name = std::string( "##voiden_font_list_" ) + str_id;
        ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, ImVec2( pad, pad ) );
        ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, ImVec2( 0, item_gap ) );
        ImGui::PushStyleVar( ImGuiStyleVar_WindowRounding, SCALE( 8 ) );
        ImGui::PushStyleVar( ImGuiStyleVar_ScrollbarSize, SCALE( 3 ) );
        ImGui::SetNextWindowPos( list_min );
        ImGui::SetNextWindowSize( ImVec2( list_w, list_h ) );
        ImGui::Begin( list_name.c_str( ), nullptr,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBackground );

        ImGuiWindow* list_window = ImGui::GetCurrentWindow( );
        list_window->DrawList->AddRectFilled( list_window->Pos, list_window->Pos + list_window->Size,
            popup_fade( col_panel_bg ), SCALE( 8 ) );
        list_window->DrawList->AddRect( list_window->Pos, list_window->Pos + list_window->Size,
            popup_fade( col_panel_border ), SCALE( 8 ) );

        if ( ImGui::IsMouseClicked( 0 ) && !ImGui::IsWindowHovered( ) && !box_hovered ) {
            st.open = false;
        }

        const float list_width = list_window->Size.x - pad * 2.f;
        for ( int i = -1; i < ( int )g_available_fonts.size( ); ++i ) {
            const char* item_name = ( i < 0 ) ? "Default" : g_available_fonts[ i ].name.c_str( );
            const std::string item_id = list_name + "_item_" + std::to_string( i );
            const ImGuiID item_hash = ImGui::GetID( item_id.c_str( ) );

            const ImVec2 item_pos = ImGui::GetCursorScreenPos( );
            const ImRect item_bb( item_pos, ImVec2( item_pos.x + list_width, item_pos.y + item_h ) );
            ImGui::ItemSize( item_bb, 0.f );
            ImGui::ItemAdd( item_bb, item_hash );

            bool item_hovered = false;
            bool item_held = false;
            const bool item_pressed = ImGui::ButtonBehavior( item_bb, item_hash, &item_hovered, &item_held );

            struct item_anim { float hover = 0.f; };
            auto& rst = anim_obj( item_id.c_str( ), 4076, item_anim{ } );
            rst.hover = anim( rst.hover, 0.f, 1.f, item_hovered );

            const bool selected = i == g_requested_font;
            if ( selected ) {
                list_window->DrawList->AddRectFilled( item_bb.Min, item_bb.Max,
                    popup_fade( col_nav_active ), SCALE( 6 ) );
            } else if ( rst.hover > 0.004f ) {
                list_window->DrawList->AddRectFilled( item_bb.Min, item_bb.Max,
                    popup_fade( lerp_col( 0, col_nav_hover, rst.hover ) ), SCALE( 6 ) );
            }

            const ImU32 text_col = popup_fade( lerp_col( col_text_dim, col_text, selected ? 1.f : rst.hover ) );
            if ( ImFont* fnt = get_font( font, 12 ) ) {
                draw_text_impl( list_window->DrawList, fnt,
                    ImVec2( item_bb.Min.x + SCALE( 10 ), item_bb.GetCenter( ).y - fnt->FontSize * 0.5f ),
                    text_col, item_name, nullptr );
            }
            if ( ImFont* icon_font = get_font( icons, 12 ) ) {
                if ( selected ) {
                    const ImVec2 ts = icon_font->CalcTextSizeA( icon_font->FontSize, FLT_MAX, 0.f, check_line );
                    list_window->DrawList->AddText( icon_font, icon_font->FontSize,
                        ImVec2( item_bb.Max.x - SCALE( 10 ) - ts.x, item_bb.GetCenter( ).y - ts.y * 0.5f ),
                        popup_fade( col_text ), check_line );
                }
            }

            if ( item_pressed ) {
                request_menu_font( i );
                st.open = false;
                value_changed = true;
            }
        }

        ImGui::BringWindowToFocusFront( list_window );
        ImGui::BringWindowToDisplayFront( list_window );

        ImGui::End( );
        ImGui::PopStyleVar( 4 );

        return value_changed;
    }

    // ------------------------------------------------------------------
    // declarative layout: blocks are data, positions/sizes are computed
    // ------------------------------------------------------------------
    struct block_row {
        enum kind_t { kind_toggle, kind_slider, kind_slider_int, kind_button, kind_dropdown, kind_keybind, kind_choice, kind_bones, kind_color, kind_multi } kind = kind_toggle;
        const char* label = "";
        const char* key = "";           // config key, also unique row id inside the block
        const char* category = "";
        bool bool_default = false;
        float float_default = 0.f;
        int int_default = 0;
        float min_v = 0.f;
        float max_v = 0.f;
        const char* format = nullptr;
        const char* off_key1 = nullptr; // config keys zeroed when this toggle is switched ON
        const char* off_key2 = nullptr; // (mirrors the legacy menu mutual exclusion)
        void (*button_callback)() = nullptr; // callback for button kind
        const char* const* options = nullptr; // choice kind
        int option_count = 0;
        const int* option_values = nullptr;   // choice kind: display index -> stored value
        const char* key2 = nullptr;     // keybind kind: binds mode key / choice kind: binds mirror
        const char* legacy_key = nullptr;     // keybind kind: legacy category/key pair
        const char* legacy_mode_key = nullptr;
        const char* visible_key = nullptr;    // row is rendered only when category/visible_key == visible_value
        int visible_value = 0;
        const char* row_dim_key = nullptr;    // row is dimmed+locked while category/row_dim_key == 0
        const char* cr_key = nullptr;   // color kind: four float config keys
        const char* cg_key = nullptr;
        const char* cb_key = nullptr;
        const char* ca_key = nullptr;
        const char* const* multi_ids = nullptr;  // multi kind: item ids
    };

    // NOTE: every pointer member has an in-class initializer - blocks are
    // created as `menu_block name;` ( default init ), so without these the
    // unset fields would hold garbage and break the tab at random

    // compact keyboard menu attached to the block's Enable row: holds the very
    // same config pairs the removed Key Bind / Activation Mode rows used
    struct block_bind_cfg {
        const char* binds_key = nullptr;       // binds/<key> - virtual key
        const char* binds_mode = nullptr;      // binds/<key> - hold/toggle/always
        const char* legacy_key = nullptr;      // <category>/<key> mirror
        const char* legacy_mode_key = nullptr; // <category>/<key> mirror
    };

    struct menu_block {
        const char* id = "";
        const char* title = "";
        const char* icon = "";
        const char* dim_category = "";   // Enabled-dim source: config pair;
        const char* dim_key = nullptr;   // null dim_key = no dim source
        block_bind_cfg bind;             // keyboard menu on the Enable row ( has_bind )
        bool has_bind = false;
        std::vector< block_row > rows;
    };

    // attaches the keyboard-icon popup to a block's Enable row; the bind and
    // activation storage is identical to the rows it replaces
    void set_block_bind( menu_block& block, const char* binds_key, const char* binds_mode,
                         const char* legacy_key, const char* legacy_mode_key ) {
        block.bind = block_bind_cfg{ binds_key, binds_mode, legacy_key, legacy_mode_key };
        block.has_bind = true;
    }

    struct menu_column {
        std::vector< menu_block > blocks;
    };

    block_row make_toggle( const char* label, const char* category, const char* key, bool def, const char* off1 = nullptr, const char* off2 = nullptr ) {
        block_row r{ };
        r.kind = block_row::kind_toggle;
        r.label = label;
        r.key = key;
        r.category = category;
        r.bool_default = def;
        r.off_key1 = off1;
        r.off_key2 = off2;
        r.button_callback = nullptr;
        return r;
    }

    block_row make_slider( const char* label, const char* category, const char* key, float def, float min_v, float max_v, const char* format ) {
        block_row r{ };
        r.kind = block_row::kind_slider;
        r.label = label;
        r.key = key;
        r.category = category;
        r.float_default = def;
        r.min_v = min_v;
        r.max_v = max_v;
        r.format = format;
        r.button_callback = nullptr;
        return r;
    }

    block_row make_slider_int( const char* label, const char* category, const char* key, int def, int min_v, int max_v, const char* format ) {
        block_row r{ };
        r.kind = block_row::kind_slider_int;
        r.label = label;
        r.key = key;
        r.category = category;
        r.int_default = def;
        r.min_v = ( float )min_v;
        r.max_v = ( float )max_v;
        r.format = format;
        r.button_callback = nullptr;
        return r;
    }

block_row make_button( const char* label, void (*callback)() ) {
        block_row r{ };
        r.kind = block_row::kind_button;
        r.label = label;
        r.key = label;
        r.category = "";
        r.bool_default = false;
        r.float_default = 0.f;
        r.int_default = 0;
        r.min_v = 0.f;
        r.max_v = 0.f;
        r.format = nullptr;
        r.off_key1 = nullptr;
        r.off_key2 = nullptr;
        r.button_callback = callback;
        return r;
    }

    block_row make_dropdown( const char* label ) {
        block_row r{ };
        r.kind = block_row::kind_dropdown;
        r.label = label;
        r.key = label;
        r.category = "";
        r.bool_default = false;
        r.float_default = 0.f;
        r.int_default = 0;
        r.min_v = 0.f;
        r.max_v = 0.f;
        r.format = nullptr;
        r.off_key1 = nullptr;
        r.off_key2 = nullptr;
        r.button_callback = nullptr;
        return r;
    }

    // key bind field: binds/... keys + legacy category/... mirror keys
    block_row make_keybind( const char* label, const char* category, const char* binds_key, const char* binds_mode, const char* legacy_key, const char* legacy_mode_key ) {
        block_row r{ };
        r.kind = block_row::kind_keybind;
        r.label = label;
        r.key = binds_key;
        r.category = category;
        r.key2 = binds_mode;
        r.legacy_key = legacy_key;
        r.legacy_mode_key = legacy_mode_key;
        r.bool_default = false;
        r.float_default = 0.f;
        r.int_default = 0;
        r.button_callback = nullptr;
        return r;
    }

    // single choice dropdown stored as an int ( activation mode, selection, ... ).
    // binds_mirror_key optionally mirrors the value into the binds/ category so
    // the game-side bind_state reads the same mode the UI shows
    block_row make_choice( const char* label, const char* category, const char* key, int def, const char* const* options, int option_count, const int* option_values = nullptr, const char* binds_mirror_key = nullptr ) {
        block_row r{ };
        r.kind = block_row::kind_choice;
        r.label = label;
        r.key = key;
        r.category = category;
        r.int_default = def;
        r.options = options;
        r.option_count = option_count;
        r.option_values = option_values;
        r.key2 = binds_mirror_key;
        r.bool_default = false;
        r.float_default = 0.f;
        r.button_callback = nullptr;
        return r;
    }

    // multi-choice dropdown stored as a map ( bone selection )
    block_row make_bones( const char* label, const char* category, const char* key ) {
        block_row r{ };
        r.kind = block_row::kind_bones;
        r.label = label;
        r.key = key;
        r.category = category;
        r.bool_default = false;
        r.float_default = 0.f;
        r.int_default = 0;
        r.button_callback = nullptr;
        return r;
    }

    // RGBA color picker row stored as four float keys ( legacy picker reused )
    block_row make_color( const char* label, const char* category, const char* cr, const char* cg, const char* cb, const char* ca ) {
        block_row r{ };
        r.kind = block_row::kind_color;
        r.label = label;
        r.key = cr;
        r.category = category;
        r.cr_key = cr;
        r.cg_key = cg;
        r.cb_key = cb;
        r.ca_key = ca;
        r.bool_default = false;
        r.float_default = 0.f;
        r.int_default = 0;
        r.button_callback = nullptr;
        return r;
    }

    // row is rendered only while category/visible_key holds the given value
    block_row visible_when( block_row r, const char* key, int value ) {
        r.visible_key = key;
        r.visible_value = value;
        return r;
    }

    // row is dimmed and locked while category/dim_key is 0 ( the toggle that
    // owns this sub-setting is switched off )
    block_row dim_when_off( block_row r, const char* key ) {
        r.row_dim_key = key;
        return r;
    }

    // generic multi-choice dropdown row ( watermark components, ... )
    block_row make_multi( const char* label, const char* category, const char* key,
                          const char* const* names, const char* const* ids, int count ) {
        block_row r{ };
        r.kind = block_row::kind_multi;
        r.label = label;
        r.key = key;
        r.category = category;
        r.options = names;
        r.option_count = count;
        r.multi_ids = ids;
        return r;
    }

    // Block/table definitions for every tab. Adding new blocks or rows here
    // is enough - sizes and positions are derived automatically.
    static const char* const k_activation_items[ 3 ] = { "Hold", "Toggle", "Always On" };
    static const int k_activation_values[ 3 ] = { 1, 0, 2 };   // legacy storage: hold=1, toggle=0, always=2
    static const char* const k_selection_items[ 2 ] = { "nearest", "random" };
    static const char* const k_outline_mode_items[ 2 ] = { "Static", "Rainbow" };
    static const char* const k_watermark_component_names[ 4 ] = { "VOIDEN", "FPS", "Date", "Time" };
    static const char* const k_watermark_component_ids[ 4 ] = { "name", "fps", "date", "time" };
    static const char* const k_info_names[ 10 ] = { "Name", "Static", "Fraction", "Admin", "Tester", "Media", "AFK", "Dead", "Level", "Distance" };
    static const char* const k_info_ids[ 10 ] = { "name", "static", "fraction", "admin", "tester", "media", "afk", "dead", "level", "distance" };
    static const char* const k_keybinds_mode_items[ 2 ] = { "Always Show", "Show Active" };
    static const char* const k_keybinds_display_items[ 2 ] = { "Always", "Menu Open" };

    // ------------------------------------------------------------------
    // compact bind menu on the Enable rows: a small gear icon left of the
    // label opens a popup holding the Key Bind field and the Activation
    // Mode dropdown ( the rows removed from the blocks ). All storage and key
    // capture goes through the existing helpers ( save_bind_config,
    // read_bind_config_*, the shared g_key_listening state ) - there is no
    // second key handling path.
    // ------------------------------------------------------------------
    constexpr float k_bind_icon_indent = 20.f;   // uniform label shift for every bindable Enable

    struct bind_popup_state {
        bool open = false;
        bool mode_open = false;
        float open_anim = 0.f;
        float hover = 0.f;
        int opened_frame = 0;
    };

    // popup body: two rows ( Key Bind / Activation Mode ) in the shared popup
    // style; the activation list is drawn after End( ) as its own top level
    // window - never a nested Begin
    void draw_bind_popup( const char* popup_id, const block_bind_cfg& bind, const ImRect& anchor,
                          bind_popup_state& st, bool active ) {
        const float pad = SCALE( 7 );
        const float row_h = SCALE( 20 );
        const float box_w = SCALE( 88 );
        const float box_h = SCALE( 17 );
        const float row_gap = SCALE( 3 );
        const float pop_w = SCALE( 198 );
        const float pop_h = pad * 2.f + row_h * 2.f + row_gap;

        // below the icon, flipped above when it would leave the screen; x is
        // clamped the same way the dropdown popups clamp themselves
        const ImVec2 display = ImGui::GetIO( ).DisplaySize;
        ImVec2 pos( anchor.Min.x, anchor.Max.y + SCALE( 6 ) + ( 1.f - st.open_anim ) * SCALE( 5 ) );
        if ( pos.y + pop_h > display.y - SCALE( 8 ) )
            pos.y = anchor.Min.y - SCALE( 6 ) - pop_h;
        pos.x = ImClamp( pos.x, SCALE( 8 ), ImMax( SCALE( 8 ), display.x - pop_w - SCALE( 8 ) ) );

        ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, ImVec2( pad, pad ) );
        ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, ImVec2( 0, row_gap ) );
        ImGui::PushStyleVar( ImGuiStyleVar_WindowRounding, SCALE( 8 ) );
        ImGui::SetNextWindowPos( pos );
        ImGui::SetNextWindowSize( ImVec2( pop_w, pop_h ) );
        ImGui::Begin( popup_id, nullptr,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBackground );

        ImGuiWindow* window = ImGui::GetCurrentWindow( );

        // fade( ) covers the menu alpha, open_anim adds the open/close fade
        auto popup_fade = [ & ]( ImU32 col ) {
            col = fade( col );
            const ImU32 a = ( ImU32 )( ( float )( ( col >> 24 ) & 0xFF ) * st.open_anim );
            return ( col & 0x00FFFFFFu ) | ( a << 24 );
        };

        window->DrawList->AddRectFilled( window->Pos, window->Pos + window->Size, popup_fade( col_panel_bg ), SCALE( 8 ) );
        window->DrawList->AddRect( window->Pos, window->Pos + window->Size, popup_fade( col_panel_border ), SCALE( 8 ) );

        const ImRect pop_bb( window->Pos, window->Pos + window->Size );
        const ImVec2 mouse = ImGui::GetIO( ).MousePos;
        // click outside closes ( the icon itself is excluded - its click is the
        // open/close toggle ); the opening frame is always skipped
        if ( ImGui::IsMouseClicked( 0 ) && ImGui::GetFrameCount( ) != st.opened_frame
            && !pop_bb.Contains( mouse ) && !anchor.Contains( mouse ) ) {
            st.open = false;
            st.mode_open = false;
        }

        int key = read_bind_config_key( bind.binds_key, bind.legacy_key );
        const int mode = read_bind_config_mode( bind.binds_mode, bind.legacy_mode_key );
        const std::string listen_id = std::string( popup_id ) + "_listen";
        const bool listening = g_key_listening == listen_id;

        const float row_w = pop_w - pad * 2.f;
        const float label_w = row_w - box_w - SCALE( 8 );

        struct popup_row_anim { float hover = 0.f; };

        // ---- Key Bind: click -> waiting dots -> any key / mouse button ----
        ImGui::SetCursorScreenPos( window->Pos + ImVec2( pad, pad ) );
        const std::string key_row_id = std::string( popup_id ) + "_key";
        const ImGuiID key_hash = ImGui::GetID( key_row_id.c_str( ) );
        const ImVec2 key_pos = ImGui::GetCursorScreenPos( );
        const ImRect key_bb( key_pos, ImVec2( key_pos.x + row_w, key_pos.y + row_h ) );
        ImGui::ItemSize( key_bb, 0.f );
        ImGui::ItemAdd( key_bb, key_hash );

        bool key_hovered = false;
        bool key_held = false;
        const bool key_pressed = active && ImGui::ButtonBehavior( key_bb, key_hash, &key_hovered, &key_held );

        auto& kst = anim_obj( key_row_id.c_str( ), 4086, popup_row_anim{ } );
        kst.hover = anim( kst.hover, 0.f, 1.f, key_hovered );

        draw_row_label( key_row_id.c_str( ), window->DrawList, key_bb.Min, label_w,
            popup_fade( lerp_col( col_text_dim, col_text, kst.hover ) ), "Key Bind", row_h );

        const ImVec2 kbox_min( key_bb.Max.x - box_w, key_bb.GetCenter( ).y - box_h * 0.5f );
        const ImVec2 kbox_max( key_bb.Max.x, kbox_min.y + box_h );
        window->DrawList->AddRectFilled( kbox_min, kbox_max, popup_fade( lerp_col( col_panel_bg, col_nav_hover, kst.hover ) ), SCALE( 6 ) );
        window->DrawList->AddRect( kbox_min, kbox_max, popup_fade( lerp_col( col_panel_border, col_text, kst.hover > 0.01f ? 1.f : 0.f ) ), SCALE( 6 ) );

        if ( listening ) {
            // same waiting dots the key bind field used inside the blocks
            const float cy = ( kbox_min.y + kbox_max.y ) * 0.5f;
            const float cx = ( kbox_min.x + kbox_max.x ) * 0.5f - SCALE( 6 );
            for ( int i = 0; i < 3; ++i )
                window->DrawList->AddCircleFilled( ImVec2( cx + i * SCALE( 6 ), cy ), SCALE( 1.7f ), popup_fade( col_text ), 12 );
        } else {
            char key_buf[ 16 ];
            const char* key_text = key_display_name( key, key_buf, sizeof( key_buf ) );
            if ( ImFont* fnt = get_font( font, 12 ) ) {
                const ImVec2 ts = fnt->CalcTextSizeA( fnt->FontSize, FLT_MAX, 0.f, key_text );
                window->DrawList->AddText( fnt, fnt->FontSize,
                    ImVec2( kbox_min.x + ( box_w - ts.x ) * 0.5f, ( kbox_min.y + kbox_max.y ) * 0.5f - ts.y * 0.5f ),
                    popup_fade( col_text ), key_text );
            }
        }

        if ( key_pressed ) {
            g_key_listening = listen_id;
            g_key_listen_frames = 2;
            // drain the "pressed since last call" flag of every virtual key so
            // the click that opened this field can never be captured itself
            for ( int vk = 1; vk < 255; ++vk )
                GetAsyncKeyState( vk );
        }

        if ( listening ) {
            if ( !active ) {
                g_key_listening.clear( );
            } else if ( g_key_listen_frames > 0 ) {
                --g_key_listen_frames;
            } else {
                for ( int vk = 1; vk < 255; ++vk ) {
                    const SHORT state = GetAsyncKeyState( vk );
                    // a fresh press only: toggle bit set AND key still down
                    if ( ( state & 1 ) == 0 || ( state & 0x8000 ) == 0 )
                        continue;
                    const int captured = vk == VK_ESCAPE ? 0 : vk;
                    save_bind_config( bind.binds_key, bind.binds_mode, bind.legacy_key, bind.legacy_mode_key, captured, mode );
                    g_key_listening.clear( );
                    break;
                }
            }
        }

        // ---- Activation Mode: Hold / Toggle / Always On ----
        ImGui::SetCursorScreenPos( ImVec2( key_bb.Min.x, key_bb.Max.y + row_gap ) );
        const std::string mode_row_id = std::string( popup_id ) + "_mode_row";
        const ImGuiID mode_hash = ImGui::GetID( mode_row_id.c_str( ) );
        const ImVec2 mode_pos = ImGui::GetCursorScreenPos( );
        const ImRect mode_bb( mode_pos, ImVec2( mode_pos.x + row_w, mode_pos.y + row_h ) );
        ImGui::ItemSize( mode_bb, 0.f );
        ImGui::ItemAdd( mode_bb, mode_hash );

        bool mode_hovered = false;
        bool mode_held = false;
        const bool mode_pressed = active && ImGui::ButtonBehavior( mode_bb, mode_hash, &mode_hovered, &mode_held );

        auto& mst = anim_obj( mode_row_id.c_str( ), 4086, popup_row_anim{ } );
        mst.hover = anim( mst.hover, 0.f, 1.f, mode_hovered );

        draw_row_label( mode_row_id.c_str( ), window->DrawList, mode_bb.Min, label_w,
            popup_fade( lerp_col( col_text_dim, col_text, mst.hover ) ), "Activation Mode", row_h );

        const int mode_index = activation_mode_to_index( mode );
        const bool mode_focused = mst.hover > 0.01f || st.mode_open;
        const ImVec2 mbox_min( mode_bb.Max.x - box_w, mode_bb.GetCenter( ).y - box_h * 0.5f );
        const ImVec2 mbox_max( mode_bb.Max.x, mbox_min.y + box_h );
        window->DrawList->AddRectFilled( mbox_min, mbox_max, popup_fade( lerp_col( col_panel_bg, col_nav_hover, mode_focused ? 1.f : 0.f ) ), SCALE( 6 ) );
        window->DrawList->AddRect( mbox_min, mbox_max, popup_fade( lerp_col( col_panel_border, col_text, mode_focused ? 1.f : 0.f ) ), SCALE( 6 ) );

        if ( ImFont* fnt = get_font( font, 12 ) ) {
            window->DrawList->AddText( fnt, fnt->FontSize,
                ImVec2( mbox_min.x + SCALE( 8 ), ( mbox_min.y + mbox_max.y ) * 0.5f - fnt->FontSize * 0.5f ),
                popup_fade( col_text ), k_activation_items[ mode_index ] );
        }
        {
            const ImU32 arrow_col = popup_fade( lerp_col( col_text_dim, col_text, mode_focused ? 1.f : 0.f ) );
            ui::rotate_start( );
            ui::arrow( { mbox_max.x - SCALE( 12 ), ( mbox_min.y + mbox_max.y ) * 0.5f - SCALE( 2 ) }, arrow_col, ImGuiDir_Down, 4 );
            ui::rotate_end( st.mode_open ? IM_PI : 0.f, ui::rotation_center( ) );
        }

        if ( mode_pressed )
            st.mode_open = !st.mode_open;

        ImGui::End( );
        ImGui::PopStyleVar( 3 );

        // the activation list is a regular top level dropdown attached to the
        // mode box; passing the real mode box hover suppresses the same-frame
        // outside-click close when the click that opened it landed on the box
        if ( st.mode_open && st.open_anim > 0.01f ) {
            bool checked[ 3 ] = { mode_index == 0, mode_index == 1, mode_index == 2 };
            const std::string mode_popup_id = std::string( popup_id ) + "_mode_popup";
            const ImRect mode_box( mbox_min, mbox_max );
            const int picked = dropdown_popup( mode_popup_id.c_str( ), mode_box, k_activation_items, 3, checked, st.mode_open, 0.f, true,
                mode_bb.Contains( mouse ) );
            if ( picked >= 0 )
                save_bind_config( bind.binds_key, bind.binds_mode, bind.legacy_key, bind.legacy_mode_key, key, k_activation_values[ picked ] );
        }
    }

    // Enable row with the gear icon: the toggle behaves exactly like a
    // plain toggle, the icon only opens/closes the bind popup
    bool row_toggle_bindable( const char* str_id, const char* label, bool* value, float width, float dim, bool active,
                              const menu_block& block ) {
        ImGuiWindow* window = ImGui::GetCurrentWindow( );
        const ImVec2 p = window->DC.CursorPos;
        const float row_h = SCALE( g_metrics.row_h );
        const ImRect row_bb( p, ImVec2( p.x + width, p.y + row_h ) );

        const std::string popup_id = std::string( str_id ) + "_bindpopup";
        auto& st = anim_obj( popup_id.c_str( ), 4085, bind_popup_state{ } );
        if ( !active ) {
            st.open = false;
            st.mode_open = false;
            if ( g_key_listening == popup_id + "_listen" )
                g_key_listening.clear( );
        }

        const float gutter = SCALE( k_bind_icon_indent );
        const ImRect icon_bb( row_bb.Min, ImVec2( row_bb.Min.x + gutter, row_bb.Max.y ) );
        const ImGuiID icon_id = window->GetID( ( popup_id + "_kbd" ).c_str( ) );
        // registered as its own item: the toggle's hit rect excludes this
        // strip, so icon and switch never fight over the hover state
        ImGui::ItemAdd( icon_bb, icon_id );
        bool icon_hovered = false;
        bool icon_held = false;
        const bool icon_pressed = active && ImGui::ButtonBehavior( icon_bb, icon_id, &icon_hovered, &icon_held );

        st.hover = anim( st.hover, 0.f, 1.f, icon_hovered || st.open );
        if ( icon_pressed ) {
            st.open = !st.open;
            st.mode_open = false;
            if ( st.open ) {
                st.opened_frame = ImGui::GetFrameCount( );
                // drain stale key bits so the opening click can never be captured
                for ( int vk = 1; vk < 255; ++vk )
                    GetAsyncKeyState( vk );
            } else if ( g_key_listening == popup_id + "_listen" ) {
                g_key_listening.clear( );
            }
        }

        ImDrawList* draw_list = window->DrawList;
        if ( st.hover > 0.004f ) {
            const ImRect plate( icon_bb.Min + ImVec2( SCALE( 1 ), SCALE( 2 ) ), icon_bb.Max - ImVec2( SCALE( 1 ), SCALE( 2 ) ) );
            draw_list->AddRectFilled( plate.Min, plate.Max, fade( lerp_col( 0, col_nav_hover, st.hover ) ), SCALE( 5 ) );
        }
        if ( ImFont* icon_font = get_font( icons, 12 ) ) {
            const ImVec2 ts = icon_font->CalcTextSizeA( icon_font->FontSize, FLT_MAX, 0.f, settings_3_line );
            const ImU32 icon_col = dim_text_col( lerp_col( col_text_dim, col_text, st.hover ), dim );
            draw_list->AddText( icon_font, icon_font->FontSize,
                ImVec2( icon_bb.Min.x + ( gutter - SCALE( 2 ) - ts.x ) * 0.5f, icon_bb.GetCenter( ).y - ts.y * 0.5f ),
                icon_col, settings_3_line );
        }

        // one click must not both flip the switch and open the menu - only the
        // icon click frame suppresses the toggle behavior
        const bool changed = row_toggle( str_id, label, value, width, dim, active && !icon_pressed, SCALE( k_bind_icon_indent ), gutter );

        st.open_anim = anim( st.open_anim, 0.f, 1.f, st.open );
        if ( st.open_anim > 0.01f ) {
            g_any_dropdown_open = true;
            draw_bind_popup( popup_id.c_str( ), block.bind, icon_bb, st, active );
        }

        return changed;
    }

    std::vector< menu_column > build_layout( int tab ) {
        std::vector< menu_column > columns;

        if ( tab == tab_aimbot ) {
            menu_block aimbot;
            aimbot.id = "aimbot";
            aimbot.title = "AIMBOT";
            aimbot.icon = aiming_line;
            aimbot.dim_category = "aimbot";
            aimbot.dim_key = "vector_enable";
            aimbot.rows.push_back( make_toggle( "Enabled", "aimbot", "vector_enable", false, "silent_enable" ) );
            set_block_bind( aimbot, "vector_aim_key", "vector_aim_mode", "aim_key", "aim_key_mode" );
            aimbot.rows.push_back( make_bones( "Bone", "aimbot", "vector_aim_bones" ) );
            aimbot.rows.push_back( make_toggle( "Draw FOV", "aimbot", "vector_show_radius", false ) );
            aimbot.rows.push_back( make_slider( "FOV Radius", "aimbot", "vector_radius", 50.f, 0.f, 500.f, "%.0f" ) );
            aimbot.rows.push_back( make_slider( "Smooth", "aimbot", "vector_smoothness", 50.f, 0.f, 100.f, "%.0f%%" ) );

            menu_block damager;
            damager.id = "damager";
            damager.title = "DAMAGER";
            damager.icon = aiming_2_line;
            damager.dim_category = "aimbot";
            damager.dim_key = "damager";
            damager.rows.push_back( make_toggle( "Enabled", "aimbot", "damager", false, "silent_enable" ) );
            set_block_bind( damager, "damager_key", "damager_mode", "damager_key", "damager_key_mode" );
            damager.rows.push_back( make_slider_int( "Rate", "aimbot", "damager_rate", 100, 1, 2000, "%d" ) );

            menu_block triggerbot;
            triggerbot.id = "triggerbot";
            triggerbot.title = "TRIGGER BOT";
            triggerbot.icon = flash_circle_line;
            triggerbot.dim_category = "aimbot";
            triggerbot.dim_key = "triggerbot";
            triggerbot.rows.push_back( make_toggle( "Enabled", "aimbot", "triggerbot", false ) );
            set_block_bind( triggerbot, "triggerbot_key", "triggerbot_mode", "triggerbot_key", "triggerbot_key_mode" );
            triggerbot.rows.push_back( make_toggle( "Draw FOV", "aimbot", "triggerbot_show_radius", false ) );
            triggerbot.rows.push_back( make_slider( "FOV Radius", "aimbot", "triggerbot_fov", 90.f, 1.f, 500.f, "%.0f" ) );

            menu_block silent;
            silent.id = "silent";
            silent.title = "SILENT AIM";
            silent.icon = flash_line;
            silent.dim_category = "aimbot";
            silent.dim_key = "silent_enable";
            silent.rows.push_back( make_toggle( "Enabled", "aimbot", "silent_enable", false, "vector_enable", "damager" ) );
            set_block_bind( silent, "silent_aim_key", "silent_aim_mode", "silent_aim_key", "silent_aim_key_mode" );
            silent.rows.push_back( make_bones( "Bone", "aimbot", "silent_aim_bones" ) );
            silent.rows.push_back( make_choice( "Selection", "aimbot", "silent_bone_selection", 0, k_selection_items, 2 ) );
            silent.rows.push_back( make_slider( "FOV", "aimbot", "silent_radius", 50.f, 0.f, 500.f, "%.0f" ) );
            silent.rows.push_back( make_slider( "Hit Chance", "aimbot", "hit_chance_value", 0.f, 0.f, 100.f, "%.0f%%" ) );
            silent.rows.push_back( make_toggle( "Draw FOV", "aimbot", "silent_show_radius", false ) );
            silent.rows.push_back( make_toggle( "Show Tracers", "aimbot", "silent_show_tracers", false ) );
            silent.rows.push_back( make_toggle( "Silent Line", "aimbot", "silent_line", false ) );
            silent.rows.push_back( make_toggle( "Magic Bullet", "aimbot", "magic_bullet", false ) );

            menu_column left;
            left.blocks.push_back( std::move( aimbot ) );
            left.blocks.push_back( std::move( damager ) );
            menu_column right;
            right.blocks.push_back( std::move( triggerbot ) );
            right.blocks.push_back( std::move( silent ) );
            columns.push_back( std::move( left ) );
            columns.push_back( std::move( right ) );
        } else if ( tab == tab_cosmetic ) {
            menu_block menu;
            menu.id = "menu";
            menu.title = "MENU";
            menu.icon = menu_line;
            menu.rows.push_back( make_slider( "Transparency", "cosmetic", "menu_transparency", 25.f, 0.f, 90.f, "%.0f" ) );
            menu.rows.push_back( make_toggle( "Watermark", "hack", "watermark", true ) );
            menu.rows.push_back( dim_when_off( make_multi( "Components", "cosmetic", "watermark_components",
                k_watermark_component_names, k_watermark_component_ids, 4 ), "watermark" ) );

            menu_block outline;
            outline.id = "outline";
            outline.title = "MENU OUTLINE";
            outline.icon = brush_line;
            outline.dim_category = "cosmetic";
            outline.dim_key = "menu_outline";
            outline.rows.push_back( make_toggle( "Enabled", "cosmetic", "menu_outline", false ) );
            outline.rows.push_back( make_choice( "Mode", "cosmetic", "menu_outline_mode", 0, k_outline_mode_items, 2 ) );
            outline.rows.push_back( visible_when(
                make_color( "Color", "cosmetic", "menu_outline_r", "menu_outline_g", "menu_outline_b", "menu_outline_a" ),
                "menu_outline_mode", 0 ) );
            outline.rows.push_back( visible_when(
                make_slider( "Speed", "cosmetic", "menu_outline_speed", 3.f, 1.f, 10.f, "%.1f" ),
                "menu_outline_mode", 1 ) );

            menu_block background;
            background.id = "background";
            background.title = "BACKGROUND";
            background.icon = background_line;
            background.rows.push_back( make_toggle( "Snow", "cosmetic", "background_snow", false ) );
            background.rows.push_back( visible_when( make_slider( "Snow Intensity", "cosmetic", "background_snow_intensity", 50.f, 0.f, 100.f, "%.0f" ), "background_snow", 1 ) );
            background.rows.push_back( make_toggle( "Particles", "cosmetic", "background_particles", false ) );
            background.rows.push_back( visible_when( make_slider( "Particle Count", "cosmetic", "background_particles_count", 110.f, 20.f, 200.f, "%.0f" ), "background_particles", 1 ) );
            background.rows.push_back( visible_when( make_slider( "Speed", "cosmetic", "background_particles_speed", 3.f, 1.f, 10.f, "%.1f" ), "background_particles", 1 ) );
            background.rows.push_back( visible_when( make_slider( "Connection Distance", "cosmetic", "background_particles_distance", 130.f, 60.f, 300.f, "%.0f" ), "background_particles", 1 ) );

            menu_column left;
            left.blocks.push_back( std::move( menu ) );
            left.blocks.push_back( std::move( outline ) );
            menu_column right;
            right.blocks.push_back( std::move( background ) );
            columns.push_back( std::move( left ) );
            columns.push_back( std::move( right ) );
        } else if ( tab == tab_visuals ) {
            menu_block thermal;
            thermal.id = "thermal";
            thermal.title = "THERMAL";
            thermal.icon = eye_line;
            thermal.rows.push_back( make_toggle( "Enabled", "visual", "thermal_enable", false ) );
            thermal.rows.push_back( dim_when_off( make_color( "World Tint", "visual", "thermal_r", "thermal_g", "thermal_b", "thermal_a" ), "thermal_enable" ) );
            thermal.rows.push_back( dim_when_off( make_slider( "Glow Intensity", "visual", "thermal_intensity", 1.f, 0.f, 5.f, "%.1f" ), "thermal_enable" ) );

            menu_block visuals;
            visuals.id = "visuals";
            visuals.title = "PLAYER";
            visuals.icon = eye_line;
            visuals.rows.push_back( make_toggle( "Enabled", "visual", "enable", false ) );
            visuals.rows.push_back( make_toggle( "Box", "visual", "draw_box", false ) );
            visuals.rows.push_back( make_toggle( "Skeletons", "visual", "draw_skeleton", false ) );
            visuals.rows.push_back( dim_when_off( make_toggle( "Information", "visual", "info_enable", false ), "enable" ) );
            visuals.rows.push_back( dim_when_off( make_multi( "Show", "visual", "info_flags",
                k_info_names, k_info_ids, 10 ), "info_enable" ) );
            visuals.rows.push_back( make_toggle( "Debug", "visual", "debug_panel", false ) );

            menu_block keybinds;
            keybinds.id = "keybinds";
            keybinds.title = "KEYBINDS";
            keybinds.icon = settings_3_line;
            keybinds.rows.push_back( make_toggle( "Enabled", "visual", "keybinds_enable", true ) );
            keybinds.rows.push_back( dim_when_off( make_choice( "Mode", "visual", "keybinds_mode", 0, k_keybinds_mode_items, 2 ), "keybinds_enable" ) );
            keybinds.rows.push_back( dim_when_off( make_choice( "Display", "visual", "keybinds_display", 0, k_keybinds_display_items, 2 ), "keybinds_enable" ) );

            menu_column left;
            left.blocks.push_back( std::move( thermal ) );
            left.blocks.push_back( std::move( keybinds ) );
            menu_column right;
            right.blocks.push_back( std::move( visuals ) );
            columns.push_back( std::move( left ) );
            columns.push_back( std::move( right ) );
        } else {
            menu_block unload;
            unload.id = "unload";
            unload.title = "UNLOAD";
            unload.icon = settings_3_line;
            unload.rows.push_back( make_button( "Unload", []() {
                ws_server::unload_user_scripts_for_shutdown();
                hacks::disable_panic_features();
                runtime_session::request_unload();
            } ) );

            menu_block fonts_block;
            fonts_block.id = "fonts";
            fonts_block.title = "FONTS";
            fonts_block.icon = settings_3_line;
            // lists every *.ttf/*.otf/*.ttc found in "<dll directory>/fonts"
            fonts_block.rows.push_back( make_dropdown( "Font" ) );

            menu_column left;
            left.blocks.push_back( std::move( unload ) );
            menu_column right;
            right.blocks.push_back( std::move( fonts_block ) );
            columns.push_back( std::move( left ) );
            columns.push_back( std::move( right ) );
        }

        return columns;
    }

    void draw_block_row( const menu_block& block, const block_row& row, const ImVec2& pos, float width, float dim, bool active ) {
        ImGui::SetCursorScreenPos( pos );
        const std::string str_id = std::string( block.id ) + "_" + row.key;

        if ( row.kind == block_row::kind_toggle ) {
            bool value = cfg_bool( row.category, row.key, row.bool_default );
            // the Enable row of a block with the compact keyboard menu carries
            // the icon + bind popup instead of the removed Key Bind rows
            const bool is_bind_enable = block.has_bind && block.dim_key && row.key
                && strcmp( row.key, block.dim_key ) == 0;
            bool changed = false;
            if ( is_bind_enable )
                changed = row_toggle_bindable( str_id.c_str( ), row.label, &value, width, dim, active, block );
            else
                changed = row_toggle( str_id.c_str( ), row.label, &value, width, dim, active );
            if ( changed ) {
                set_cfg_bool( row.category, row.key, row.bool_default, value );
                if ( value && row.off_key1 ) config::update( 0, row.category, row.off_key1, 0 );
                if ( value && row.off_key2 ) config::update( 0, row.category, row.off_key2, 0 );
            }
        } else if ( row.kind == block_row::kind_slider ) {
            float value = config::get( row.category, row.key, row.float_default );
            if ( row_slider_float( str_id.c_str( ), row.label, &value, row.min_v, row.max_v, row.format, width, dim, active ) ) {
                config::update( value, row.category, row.key, row.float_default );
            }
        } else if ( row.kind == block_row::kind_slider_int ) {
            int value = config::get( row.category, row.key, row.int_default );
            if ( row_slider_int( str_id.c_str( ), row.label, &value, ( int )row.min_v, ( int )row.max_v, row.format, width, dim, active ) ) {
                config::update( value, row.category, row.key, row.int_default );
            }
        } else if ( row.kind == block_row::kind_keybind ) {
            row_key_bind( str_id.c_str( ), row.label, row.key, row.key2, row.legacy_key, row.legacy_mode_key, width, dim, true, active );
        } else if ( row.kind == block_row::kind_choice ) {
            int value = config::get( row.category, row.key, row.int_default );
            row_choice( str_id.c_str( ), row.label, row.options, row.option_count, row.option_values, &value, width, dim, active );
            if ( active ) {
                config::update( value, row.category, row.key, row.int_default );
                // mirror activation modes into the binds/ category - bind_state
                // reads binds first and only falls back to the legacy key,
                // without the mirror "always on" would never reach the game
                if ( row.key2 )
                    config::update( value, std::string( "binds" ), std::string( row.key2 ), 0 );
            }
        } else if ( row.kind == block_row::kind_bones ) {
            row_bones( str_id.c_str( ), row.label, row.category, row.key, width, dim, active );
        } else if ( row.kind == block_row::kind_multi ) {
            row_multi_choice( str_id.c_str( ), row.label, row.category, row.key,
                row.multi_ids, row.options, row.option_count, width, dim, active );
        } else if ( row.kind == block_row::kind_color ) {
            ImGuiWindow* window = ImGui::GetCurrentWindow( );
            const ImGuiID id = window->GetID( str_id.c_str() );
            const float height = SCALE( g_metrics.row_h );
            const ImVec2 p = window->DC.CursorPos;
            const ImRect bb( p, ImVec2( p.x + width, p.y + height ) );
            ImGui::ItemSize( bb, 0.f );
            ImGui::ItemAdd( bb, id );

            const bool row_hovered = active && ImGui::IsMouseHoveringRect( bb.Min, bb.Max );

            float col_value[ 4 ] = {
                config::get( row.category, row.cr_key, 1.f ),
                config::get( row.category, row.cg_key, 1.f ),
                config::get( row.category, row.cb_key, 1.f ),
                config::get( row.category, row.ca_key, 1.f ),
            };

            // label with the shared long-text handling, swatch sits on the right
            ImDrawList* row_draw_list = window->DrawList;
            const float sw = SCALE( 14 );
            const ImU32 text_col = dim_text_col( lerp_col( col_text_dim, col_text, row_hovered ? 1.f : 0.f ), dim );
            draw_row_label( str_id.c_str( ), row_draw_list, ImVec2( bb.Min.x, bb.Min.y ),
                ImMax( SCALE( 10 ), width - sw - SCALE( 8 ) ), text_col, row.label, height );
            if ( active ) {
                window->DC.CursorPos = ImVec2( bb.Max.x - sw, bb.GetCenter( ).y - sw * 0.5f );
                const bool changed = ui::color_btn( ( str_id + "_color" ).c_str( ), col_value, ImVec2( sw, sw ), true );

                const bool moved =
                    col_value[ 0 ] != config::get( row.category, row.cr_key, 1.f ) ||
                    col_value[ 1 ] != config::get( row.category, row.cg_key, 1.f ) ||
                    col_value[ 2 ] != config::get( row.category, row.cb_key, 1.f ) ||
                    col_value[ 3 ] != config::get( row.category, row.ca_key, 1.f );
                if ( changed || moved ) {
                    config::update( col_value[ 0 ], row.category, row.cr_key, 1.f );
                    config::update( col_value[ 1 ], row.category, row.cg_key, 1.f );
                    config::update( col_value[ 2 ], row.category, row.cb_key, 1.f );
                    config::update( col_value[ 3 ], row.category, row.ca_key, 1.f );
                }
            } else {
                // inactive: static dimmed swatch, no picker
                const ImVec2 smin( bb.Max.x - sw, bb.GetCenter( ).y - sw * 0.5f );
                row_draw_list->AddRectFilled( smin, smin + ImVec2( sw, sw ), dim_border_col( IM_COL32( 90, 90, 96, 255 ), dim ), SCALE( 3 ) );
                row_draw_list->AddRect( smin, smin + ImVec2( sw, sw ), dim_border_col( col_panel_border, dim ), SCALE( 3 ) );
            }
        } else if ( row.kind == block_row::kind_dropdown ) {
            row_font_dropdown( str_id.c_str( ), row.label, width );
        } else if ( row.kind == block_row::kind_button ) {
            ImGuiWindow* window = ImGui::GetCurrentWindow( );
            const ImGuiID id = window->GetID( str_id.c_str() );
            const ImVec2 p = window->DC.CursorPos;
            const float height = SCALE( g_metrics.row_h );
            const ImRect bb( p, ImVec2( p.x + width, p.y + height ) );
            ImGui::ItemSize( bb, 0.f );
            ImGui::ItemAdd( bb, id );

            bool hovered = false;
            bool held = false;
            const bool pressed = active && ImGui::ButtonBehavior( bb, id, &hovered, &held );

            struct button_anim { float hover = 0.f; float held = 0.f; };
            auto& st = anim_obj( str_id.c_str(), 4074, button_anim{ } );
            st.hover = anim( st.hover, 0.f, 1.f, hovered );
            st.held = anim( st.held, 0.f, 1.f, held );

            ImDrawList* draw_list = window->DrawList;
            const ImU32 bg_col = fade( lerp_col( col_panel_bg, col_nav_hover, st.hover ) );
            const ImU32 border_col = dim_border_col( lerp_col( col_panel_border, col_text, st.hover ), dim );
            const ImU32 text_col = dim_text_col( lerp_col( col_text_dim, col_text, st.hover ), dim );

            draw_list->AddRectFilled( bb.Min, bb.Max, bg_col, SCALE( 6 ) );
            draw_list->AddRect( bb.Min, bb.Max, border_col, SCALE( 6 ) );

            if ( ImFont* fnt = get_font( fontb, 13 ) ) {
                const ImVec2 ts = fnt->CalcTextSizeA( fnt->FontSize, FLT_MAX, 0.f, row.label );
                draw_text_impl( draw_list, fnt, ImVec2( bb.GetCenter().x - ts.x * 0.5f, bb.GetCenter().y - ts.y * 0.5f ), text_col, row.label, nullptr );
            }

            if ( pressed && row.button_callback ) {
                row.button_callback();
            }
        }
    }

    void draw_block_frame( ImDrawList* draw_list, const menu_block& block, const ImVec2& bmin, const ImVec2& bmax ) {
        draw_list->AddRectFilled( bmin, bmax, fade( col_panel_bg ), SCALE( 10 ) );
        draw_list->AddRect( bmin, bmax, fade( col_panel_border ), SCALE( 10 ) );

        if ( ImFont* icon_font = get_font( icons, 14 ) ) {
            const ImVec2 ts = icon_font->CalcTextSizeA( icon_font->FontSize, FLT_MAX, 0.f, block.icon );
            draw_list->AddText( icon_font, icon_font->FontSize, ImVec2( bmin.x + SCALE( k_panel_pad_side ), bmin.y + SCALE( g_metrics.header_h ) * 0.5f - ts.y * 0.5f ), fade( col_text ), block.icon );
        }
        draw_text( draw_list, fontb, 13, ImVec2( bmin.x + SCALE( k_panel_pad_side ) + SCALE( 26 ), bmin.y + ( g_metrics.header_h - 13.f ) * 0.5f ), col_text, block.title );
        draw_list->AddLine(
            ImVec2( bmin.x + SCALE( k_panel_pad_side ), bmin.y + SCALE( g_metrics.header_h - 2 ) ),
            ImVec2( bmax.x - SCALE( k_panel_pad_side ), bmin.y + SCALE( g_metrics.header_h - 2 ) ),
            fade( col_separator ) );
    }

    // Draws one column of blocks top-down. Every block is sized from its row
    // count and the next block starts right after the previous one plus the
    // fixed gap, so blocks can never overlap and adding/removing rows
    // reflows everything below automatically.
    // Draws one column of blocks top-down. Every block is sized from its row
    // count and the next block starts right after the previous one plus the
    // fixed gap, so blocks can never overlap and adding/removing rows
    // reflows everything below automatically. The whole column is shifted by
    // `scroll` and clipped by the caller - blocks past the visible bottom are
    // reached by scrolling instead of being dropped.
    void draw_column( ImDrawList* draw_list, const menu_column& column, float x, float width, float top, float scroll, float* out_content_bottom ) {
        const auto row_visible = [ & ]( const block_row& r ) {
            return !r.visible_key
                || config::get( r.category ? r.category : "", r.visible_key, 0 ) == r.visible_value;
        };

        float y = top;
        float bottom = top;
        for ( const menu_block& block : column.blocks ) {
            int visible_rows = 0;
            for ( const block_row& row : block.rows )
                if ( row_visible( row ) )
                    ++visible_rows;

            const float height = block_height( g_metrics, visible_rows );

            const ImVec2 bmin( x, y );
            const ImVec2 bmax( x + width, y + height );
            const float row_w = width - SCALE( k_panel_pad_side ) * 2.f;

            // Enabled-state visual: when the block's dim key is off, every row
            // below the first one is rendered dimmed ( purely visual )
            const bool block_enabled = !block.dim_key
                || config::get( block.dim_category ? block.dim_category : "", block.dim_key, 0 ) != 0;

            draw_block_frame( draw_list, block, bmin, bmax );

            const ImVec2 first_row(
                bmin.x + SCALE( k_panel_pad_side ),
                bmin.y + SCALE( g_metrics.header_h + g_metrics.pad_top ) );

            int draw_index = 0;
            for ( const block_row& row : block.rows ) {
                if ( !row_visible( row ) )
                    continue;

                const ImVec2 row_pos(
                    first_row.x,
                    first_row.y + SCALE( g_metrics.row_step ) * draw_index );

                // rows with their own dim source ( sub-settings like Blur
                // Intensity or watermark Components ) are locked by their
                // toggle; the toggle's own default is used so the locked state
                // always matches what the toggle row displays. the rest follow
                // the block Enabled state ( the first row always stays active )
                bool row_active;
                if ( row.row_dim_key ) {
                    bool dim_source_on = true;
                    for ( const block_row& source : block.rows ) {
                        if ( source.key && row.row_dim_key && strcmp( source.key, row.row_dim_key ) == 0 ) {
                            dim_source_on = cfg_bool( source.category, source.key, source.bool_default );
                            break;
                        }
                    }
                    row_active = dim_source_on;
                } else {
                    row_active = draw_index == 0 || block_enabled;
                }
                const float dim = row_active ? 0.f : 0.55f;
                draw_block_row( block, row, row_pos, row_w, dim, row_active );
                ++draw_index;
            }

            y = bmax.y + SCALE( g_metrics.block_gap );
            bottom = bmax.y;
        }

        if ( out_content_bottom )
            *out_content_bottom = bottom;
    }

    // ------------------------------------------------------------------
    // style (minimal set, also keeps notify::draw() correct while the
    // legacy menu is not rendered)
    // ------------------------------------------------------------------
    void apply_style( ) {
        ImVec4* c = ImGui::GetStyle( ).Colors;
        c[ ImGuiCol_Text ]          = ImVec4( 0.91f, 0.91f, 0.92f, 1.00f );
        c[ ImGuiCol_TextHovered ]   = ImVec4( 0.70f, 0.70f, 0.72f, 1.00f );
        c[ ImGuiCol_TextDisabled ]  = ImVec4( 0.42f, 0.42f, 0.44f, 1.00f );
        c[ ImGuiCol_WindowBg ]      = ImVec4( 0.035f, 0.035f, 0.040f, 1.00f );
        c[ ImGuiCol_ChildBg ]       = ImVec4( 0.050f, 0.050f, 0.060f, 1.00f );
        c[ ImGuiCol_PopupBg ]       = ImVec4( 0.050f, 0.050f, 0.060f, 1.00f );
        c[ ImGuiCol_Border ]        = ImVec4( 0.120f, 0.120f, 0.130f, 1.00f );
        c[ ImGuiCol_FrameBg ]       = ImVec4( 0.060f, 0.060f, 0.070f, 1.00f );
        c[ ImGuiCol_ScrollbarBg ]   = ImVec4( 0.030f, 0.030f, 0.035f, 1.00f );
        c[ ImGuiCol_ScrollbarGrab ] = ImVec4( 0.160f, 0.160f, 0.170f, 1.00f );
    }

    // light rotating-arc spinner, no extra libraries
    void draw_spinner( ImDrawList* draw_list, const ImVec2& center, float radius, ImU32 col ) {
        const float time = ( float )ImGui::GetTime( );
        const int segments = 22;
        const float arc = IM_PI * 1.35f;
        const float start = time * 4.2f;

        draw_list->PathClear( );
        for ( int i = 0; i <= segments; ++i ) {
            const float a = start + ( arc * i ) / segments;
            draw_list->PathLineTo( ImVec2( center.x + ImCos( a ) * radius, center.y + ImSin( a ) * radius ) );
        }
        draw_list->PathStroke( col, 0, SCALE( 2.5f ) );
    }

    // drawn while a freshly requested font is still being prepared on the
    // worker thread: menu keeps running, just dimmed with a small indicator
    void draw_font_loading_overlay( ImDrawList* draw_list, const ImVec2& wpos, const ImVec2& wsize ) {
        if ( g_requested_font < 0 || g_requested_font == g_selected_font
            || g_requested_font >= ( int )g_available_fonts.size( ) )
            return;

        font_asset* asset = find_font_asset( g_available_fonts[ g_requested_font ].name );
        if ( !asset || asset->state.load( ) != font_asset_loading )
            return;

        draw_list->AddRectFilled( wpos, wpos + wsize, fade( IM_COL32( 5, 5, 7, 165 ) ), SCALE( 10.f ) );

        const ImVec2 center( wpos.x + wsize.x * 0.5f, wpos.y + wsize.y * 0.5f );
        draw_spinner( draw_list, ImVec2( center.x, center.y - SCALE( 8 ) ), SCALE( 14 ), fade( col_text ) );

        const char* label = "loading font...";
        const ImVec2 ts = text_size( font, 11, label );
        draw_text( draw_list, font, 11, ImVec2( center.x - ts.x * 0.5f, center.y + SCALE( 18 ) ), col_text_dim, label );
    }

    // ------------------------------------------------------------------
    // drawing
    // ------------------------------------------------------------------
    void draw_menu( ) {
        g_any_dropdown_open = false;
        ImDrawList* draw_list = ImGui::GetWindowDrawList( );
        const ImVec2 wpos = ImGui::GetWindowPos( );
        const ImVec2 wsize = ImGui::GetWindowSize( );

        const float sidebar_w = SCALE( 224 );
        const float pad = SCALE( 26 );

        // ---------------- sidebar ----------------
        // round the left corners to match the window rounding - a square fill
        // would overpaint the window's rounded corners and the outline
        draw_list->AddRectFilled( wpos, ImVec2( wpos.x + sidebar_w, wpos.y + wsize.y ), fade( col_sidebar_bg ), SCALE( 10.f ), ImDrawFlags_RoundCornersLeft );
        draw_list->AddLine( ImVec2( wpos.x + sidebar_w, wpos.y ), ImVec2( wpos.x + sidebar_w, wpos.y + wsize.y ), fade( col_sidebar_line ) );

        // logo: centered in the sidebar, aspect preserved, compact footprint.
        const float logo_box_w = SCALE( 132 );
        const float logo_box_h = SCALE( 168 );
        float logo_draw_w = SCALE( 64 );
        float logo_draw_h = SCALE( 64 );
        if ( g_logo && g_logo_w > 0 && g_logo_h > 0 ) {
            const float aspect = ( float )g_logo_w / ( float )g_logo_h;
            logo_draw_w = logo_box_w;
            logo_draw_h = logo_box_w / aspect;
            if ( logo_draw_h > logo_box_h ) {
                logo_draw_h = logo_box_h;
                logo_draw_w = logo_box_h * aspect;
            }
        }
        const ImVec2 logo_min( wpos.x + ( sidebar_w - logo_draw_w ) * 0.5f, wpos.y + SCALE( 22 ) );

        if ( g_logo && g_logo_w > 0 && g_logo_h > 0 ) {
            draw_list->AddImage( ( ImTextureID )( intptr_t )g_logo, logo_min, logo_min + ImVec2( logo_draw_w, logo_draw_h ), ImVec2( 0.f, 0.f ), ImVec2( 1.f, 1.f ), fade( IM_COL32( 255, 255, 255, 255 ) ) );
        } else {
            draw_list->AddCircle( logo_min + ImVec2( logo_draw_w, logo_draw_h ) * 0.5f, logo_draw_w * 0.5f, fade( col_panel_border ), 48, SCALE( 1.5f ) );
        }

        const float logo_bottom = logo_min.y + logo_draw_h;
        const ImVec2 caption_ts = text_size( font, 9, "PRIVATE SOFTWARE" );
        draw_text( draw_list, font, 9, ImVec2( wpos.x + ( sidebar_w - caption_ts.x ) * 0.5f, logo_bottom + SCALE( 6 ) ), col_text_faint, "PRIVATE SOFTWARE" );

        // nav (below the logo, vertical) - the section list scrolls when it
        // outgrows the sidebar strip between the logo and the footer
        const float nav_width = sidebar_w - pad * 2.f;
        const float nav_y = logo_bottom + SCALE( 40 );
        const float nav_limit_y = wpos.y + wsize.y - SCALE( 70 );
        const float nav_avail_h = ImMax( SCALE( 20 ), nav_limit_y - nav_y );
        const int nav_count = 4;
        const float nav_content_h = ( nav_count - 1 ) * SCALE( 50 ) + SCALE( 42 );
        const float nav_max_scroll = ImMax( 0.f, nav_content_h - nav_avail_h );
        if ( ImGui::IsMouseHoveringRect( ImVec2( wpos.x, nav_y - SCALE( 6 ) ), ImVec2( wpos.x + sidebar_w, nav_limit_y ) ) && !g_any_dropdown_open ) {
            const float wheel = ImGui::GetIO( ).MouseWheel;
            if ( wheel != 0.f )
                g_nav_scroll = ImClamp( g_nav_scroll - wheel * SCALE( 48.f ), 0.f, nav_max_scroll );
        }
        g_nav_scroll = ImClamp( g_nav_scroll, 0.f, nav_max_scroll );
        const float nav_offset = -g_nav_scroll;

        ImGui::PushClipRect( ImVec2( wpos.x, nav_y - SCALE( 4 ) ), ImVec2( wpos.x + sidebar_w, nav_limit_y ), true );
        ImGui::SetCursorScreenPos( ImVec2( wpos.x + pad, nav_y + nav_offset ) );
        if ( nav_item( "voiden_nav_aimbot", aiming_line, "AIMBOT", g_active_tab == tab_aimbot, nav_width ) ) {
            g_active_tab = tab_aimbot;
        }
        ImGui::SetCursorScreenPos( ImVec2( wpos.x + pad, nav_y + SCALE( 50 ) + nav_offset ) );
        if ( nav_item( "voiden_nav_visuals", eye_line, "VISUALS", g_active_tab == tab_visuals, nav_width ) ) {
            g_active_tab = tab_visuals;
        }
        ImGui::SetCursorScreenPos( ImVec2( wpos.x + pad, nav_y + SCALE( 100 ) + nav_offset ) );
        if ( nav_item( "voiden_nav_settings", settings_3_line, "SETTINGS", g_active_tab == tab_settings, nav_width ) ) {
            g_active_tab = tab_settings;
        }
        ImGui::SetCursorScreenPos( ImVec2( wpos.x + pad, nav_y + SCALE( 150 ) + nav_offset ) );
        if ( nav_item( "voiden_nav_cosmetic", brush_line, "COSMETIC", g_active_tab == tab_cosmetic, nav_width ) ) {
            g_active_tab = tab_cosmetic;
        }
        ImGui::PopClipRect( );
        // sidebar footer
        draw_text( draw_list, fontb, 12, ImVec2( wpos.x + pad, wpos.y + wsize.y - SCALE( 58 ) ), col_text_dim, "v1.0.0" );
        draw_text( draw_list, font, 9, ImVec2( wpos.x + pad, wpos.y + wsize.y - SCALE( 36 ) ), col_text_faint, "BUILD: " __DATE__ );

        // ---------------- content header ----------------
        const float content_x = wpos.x + sidebar_w + SCALE( 30 );
        const float content_w = wsize.x - sidebar_w - SCALE( 60 );

        const char* title = ( g_active_tab == tab_aimbot ) ? "AIMBOT" : ( g_active_tab == tab_visuals ? "VISUALS" : ( g_active_tab == tab_cosmetic ? "COSMETIC" : "SETTINGS" ) );
        const char* subtitle = ( g_active_tab == tab_aimbot ) ? "aim assistance module" : ( g_active_tab == tab_visuals ? "player overlay module" : ( g_active_tab == tab_cosmetic ? "menu customization" : "configuration and unload" ) );
        const char* icon = ( g_active_tab == tab_aimbot ) ? aiming_line : ( g_active_tab == tab_visuals ? eye_line : ( g_active_tab == tab_cosmetic ? brush_line : settings_3_line ) );

        if ( ImFont* icon_font = get_font( icons, 16 ) ) {
            const ImVec2 ts = icon_font->CalcTextSizeA( icon_font->FontSize, FLT_MAX, 0.f, icon );
            draw_list->AddText( icon_font, icon_font->FontSize, ImVec2( content_x, wpos.y + SCALE( 28 ) + ( SCALE( 18 ) - ts.y ) * 0.5f ), fade( col_text ), icon );
        }
        draw_text( draw_list, fontb, 16, ImVec2( content_x + SCALE( 30 ), wpos.y + SCALE( 24 ) ), col_text, title );
        draw_text( draw_list, font, 11, ImVec2( content_x + SCALE( 30 ), wpos.y + SCALE( 50 ) ), col_text_dim, subtitle );

        // ---------------- settings area: automatic block layout ----------------
        std::vector< menu_column > columns = build_layout( g_active_tab );
        if ( columns.empty( ) ) {
            return;
        }

        const float area_top = wpos.y + SCALE( 88 );
        const float area_bottom = wpos.y + wsize.y - SCALE( 28 );
        const float total_gap = SCALE( k_column_gap ) * ( static_cast< float >( columns.size( ) ) - 1.f );
        const float column_w = ( content_w - total_gap ) / static_cast< float >( columns.size( ) );
        const float area_height = ImMax( SCALE( 10 ), area_bottom - area_top );

        g_metrics = tab_metrics( g_active_tab );

        // per-tab content scrolling: the wheel moves the offset, blocks are
        // drawn shifted and clipped to the work area. no visual scrollbar -
        // wheel input is the only control. the offset is applied in the same
        // frame the wheel is handled, and the clamp uses the content height
        // measured on the previous frame, so the list can always be scrolled
        // exactly to its bottom edge
        static float g_content_scroll[ 4 ] = {};
        static float g_content_max[ 4 ] = {};
        float& content_scroll = g_content_scroll[ ImClamp( g_active_tab, 0, 3 ) ];
        float& content_max = g_content_max[ ImClamp( g_active_tab, 0, 3 ) ];

        if ( ImGui::IsMouseHoveringRect( ImVec2( content_x, area_top ), ImVec2( content_x + content_w, area_bottom ) ) && !g_any_dropdown_open ) {
            const float wheel = ImGui::GetIO( ).MouseWheel;
            if ( wheel != 0.f )
                content_scroll -= wheel * SCALE( 56.f );
        }
        content_scroll = ImClamp( content_scroll, 0.f, content_max );

        ImGui::PushClipRect( ImVec2( content_x, area_top ), ImVec2( content_x + content_w, area_bottom ), true );

        float content_bottom = area_top;
        float column_x = content_x;
        for ( const menu_column& column : columns ) {
            float column_bottom = area_top;
            draw_column( draw_list, column, column_x, column_w, area_top - content_scroll, content_scroll, &column_bottom );
            content_bottom = ImMax( content_bottom, column_bottom );
            column_x += column_w + SCALE( k_column_gap );
        }

        ImGui::PopClipRect( );

        // content_bottom is measured in the scrolled ( shifted ) space - convert
        // it back to the un-scrolled content height before computing the range.
        // Subtracting the shifted value directly made max scroll shrink while
        // scrolling, so the bottom half of tall tabs ( Silent Aim ) was
        // unreachable; the height is dynamic, so growing/shrinking blocks
        // recompute the range automatically. No visual scrollbar by design.
        const float content_height = ImMax( 0.f, content_bottom - ( area_top - content_scroll ) );
        content_max = ImMax( 0.f, content_height - area_height );
        content_scroll = ImClamp( content_scroll, 0.f, content_max );

        // menu outline ( Cosmetic -> Menu Outline ): thin border on the very
        // edge of the menu window, static color or animated rainbow
        if ( config::get( "cosmetic", "menu_outline", 0 ) != 0 ) {
            ImU32 outline_col = 0;
            if ( config::get( "cosmetic", "menu_outline_mode", 0 ) == 1 ) {
                const float speed = ImMax( 0.1f, config::get( "cosmetic", "menu_outline_speed", 3.f ) );
                const float hue = ImFmod( ( float )ImGui::GetTime( ) * 0.06f * speed, 1.f );
                outline_col = fade( ( ImU32 )( ImColor )ImColor::HSV( hue, 0.75f, 1.f ) );
            } else {
                const ImColor c(
                    config::get( "cosmetic", "menu_outline_r", 0.71f ),
                    config::get( "cosmetic", "menu_outline_g", 0.65f ),
                    config::get( "cosmetic", "menu_outline_b", 0.96f ),
                    config::get( "cosmetic", "menu_outline_a", 1.f ) );
                outline_col = fade( c );
            }
            draw_list->AddRect( wpos, wpos + wsize, outline_col, SCALE( 10.f ), 0, SCALE( 1.5f ) );
        }

        draw_font_loading_overlay( draw_list, wpos, wsize );
    }

void initialize( ID3D11Device* device ) {
        if ( g_initialized ) {
            return;
        }
  
        if ( device && !g_logo ) {
            const std::filesystem::path logo_path = resolve_logo_path( );
            if ( !logo_path.empty( ) ) {
                load_texture_from_file( device, logo_path, &g_logo, &g_logo_w, &g_logo_h );
            }
        }

        // fonts next to the DLL + restore the font picked in a previous session
        // ( prepared on the worker thread, the menu is never blocked )
        g_device = device;
        scan_available_fonts( );
        ensure_font_worker( );
        try_apply_saved_menu_font( );

        g_initialized = true;
    }
  
    void shutdown( ) {
        // render/worker stop FIRST: nothing may touch fonts, assets or the
        // worker while the unload chain releases them
        g_shutting_down = true;

        if ( g_logo ) {
            g_logo->Release( );
            g_logo = nullptr;
        }
        g_logo_w = 0;
        g_logo_h = 0;

        // stop the worker first so it can not touch the assets being freed
        stop_font_worker( );
        for ( font_asset* asset : g_font_assets ) {
            release_font_asset_gpu( asset );
            delete asset;
        }
        g_font_assets.clear( );
        g_device = nullptr;

        g_available_fonts.clear( );
        g_selected_font_name.clear( );
        g_requested_font_name.clear( );
        g_selected_font = -1;
        g_requested_font = -1;
        g_saved_font_resolved = false;

        g_initialized = false;
    }
 
    // ------------------------------------------------------------------
    // Cosmetic -> Background effects ( drawn behind the menu, on top of the
    // blurred game image, before any window of the interface )
    // ------------------------------------------------------------------
    struct snow_flake { float x, y, speed, drift, phase, size; };

    static void draw_snow_effect( ImDrawList* draw_list, float dt, float screen_w, float screen_h, float intensity ) {
        static snow_flake flakes[ 220 ] = {};
        static bool seeded = false;
        const int max_flakes = ( int )( sizeof( flakes ) / sizeof( flakes[ 0 ] ) );
        if ( !seeded ) {
            unsigned int seed = 20260829u;
            for ( int i = 0; i < max_flakes; ++i ) {
                snow_flake& f = flakes[ i ];
                seed = seed * 1664525u + 1013904223u;
                f.x = ( float )( seed % 10000u ) * 0.0001f * screen_w;
                seed = seed * 1664525u + 1013904223u;
                f.y = ( float )( seed % 10000u ) * 0.0001f * screen_h;
                seed = seed * 1664525u + 1013904223u;
                f.speed = 32.f + ( float )( seed % 1000u ) * 0.08f;
                seed = seed * 1664525u + 1013904223u;
                f.drift = 0.5f + ( float )( seed % 1000u ) * 0.0012f;
                seed = seed * 1664525u + 1013904223u;
                f.phase = ( float )( seed % 628u ) * 0.01f;
                seed = seed * 1664525u + 1013904223u;
                f.size = 1.4f + ( float )( seed % 1000u ) * 0.0021f;
            }
            seeded = true;
        }

        // intensity = density: 0 -> few flakes, 100 -> the whole pool
        const int count = ( int )ImClamp( 25.f + intensity * 1.95f, 25.f, ( float )max_flakes );
        const float t = ( float )ImGui::GetTime( );
        for ( int i = 0; i < count; ++i ) {
            snow_flake& f = flakes[ i ];
            f.y += f.speed * dt;
            f.x += ImSin( t * f.drift + f.phase ) * 16.f * dt;
            if ( f.y > screen_h + 4.f ) {
                f.y -= screen_h + 8.f;
                f.x = ImFmod( f.x + 37.f, screen_w + 8.f );
            }
            if ( f.x < -4.f ) f.x += screen_w + 8.f;
            else if ( f.x > screen_w + 4.f ) f.x -= screen_w + 8.f;

            const int alpha = 110 + ( int )( ( f.size - 1.4f ) * 38.f );
            draw_list->AddCircleFilled( ImVec2( f.x, f.y ), f.size, fade( IM_COL32( 240, 245, 255, alpha ) ), 10 );
        }
    }

    struct bg_particle { float x, y, vx, vy; };

    static void draw_particles_effect( ImDrawList* draw_list, float dt, float screen_w, float screen_h,
                                       float count_f, float speed, float connection ) {
        const int count = ( int )ImClamp( count_f, 10.f, 200.f );
        static bg_particle pts[ 200 ] = {};
        static bool seeded = false;
        if ( !seeded ) {
            unsigned int seed = 987654321u;
            for ( int i = 0; i < 200; ++i ) {
                bg_particle& p = pts[ i ];
                seed = seed * 1664525u + 1013904223u;
                p.x = ( float )( seed % 10000u ) * 0.0001f * screen_w;
                seed = seed * 1664525u + 1013904223u;
                p.y = ( float )( seed % 10000u ) * 0.0001f * screen_h;
                seed = seed * 1664525u + 1013904223u;
                const float angle = ( float )( seed % 628u ) * 0.01f;
                seed = seed * 1664525u + 1013904223u;
                const float v = 12.f + ( float )( seed % 1000u ) * 0.02f;
                p.vx = ImCos( angle ) * v;
                p.vy = ImSin( angle ) * v;
            }
            seeded = true;
        }

        const float sp = ImClamp( speed, 0.1f, 10.f );
        const float conn = ImClamp( connection, 20.f, 400.f );
        const float conn_sq = conn * conn;

        for ( int i = 0; i < count; ++i ) {
            bg_particle& p = pts[ i ];
            p.x += p.vx * sp * dt;
            p.y += p.vy * sp * dt;
            if ( p.x < 0.f ) { p.x = 0.f; p.vx = -p.vx; }
            else if ( p.x > screen_w ) { p.x = screen_w; p.vx = -p.vx; }
            if ( p.y < 0.f ) { p.y = 0.f; p.vy = -p.vy; }
            else if ( p.y > screen_h ) { p.y = screen_h; p.vy = -p.vy; }
        }

        // links between close particles: the closer they are, the stronger
        // the line ( fades out smoothly towards Connection Distance )
        for ( int i = 0; i < count; ++i ) {
            for ( int j = i + 1; j < count; ++j ) {
                const float dx = pts[ i ].x - pts[ j ].x;
                const float dy = pts[ i ].y - pts[ j ].y;
                const float d2 = dx * dx + dy * dy;
                if ( d2 > conn_sq ) continue;
                const int alpha = ( int )( ( 1.f - ImSqrt( d2 ) / conn ) * 150.f );
                draw_list->AddLine( ImVec2( pts[ i ].x, pts[ i ].y ), ImVec2( pts[ j ].x, pts[ j ].y ),
                    fade( IM_COL32( 150, 160, 255, alpha ) ), 1.f );
            }
        }

        // the cursor acts as one more particle
        const ImVec2 mouse = ImGui::GetIO( ).MousePos;
        for ( int i = 0; i < count; ++i ) {
            const float dx = pts[ i ].x - mouse.x;
            const float dy = pts[ i ].y - mouse.y;
            const float d2 = dx * dx + dy * dy;
            if ( d2 > conn_sq ) continue;
            const int alpha = ( int )( ( 1.f - ImSqrt( d2 ) / conn ) * 200.f );
            draw_list->AddLine( ImVec2( mouse.x, mouse.y ), ImVec2( pts[ i ].x, pts[ i ].y ),
                fade( IM_COL32( 170, 180, 255, alpha ) ), 1.2f );
        }

        for ( int i = 0; i < count; ++i )
            draw_list->AddCircleFilled( ImVec2( pts[ i ].x, pts[ i ].y ), 2.6f, fade( IM_COL32( 205, 215, 255, 190 ) ), 12 );
    }

    void render( bool menu_open ) {
        if ( !g_initialized || !ImGui::GetCurrentContext( ) || g_shutting_down ) {
            return;
        }

        // pick up worker results / upload textures / apply the requested font;
        // runs every frame even with the menu closed so the font is ready
        // by the time it is opened again
        process_font_assets( );

        apply_style( );

        // single linear open/close animation shared by every element:
        // nothing is drawn without fade(), so the whole menu (background,
        // logo, borders, controls) appears and disappears simultaneously
        static float menu_alpha = 0.f;
        const float target_alpha = menu_open ? 1.f : 0.f;
        const float step = ImGui::GetIO( ).DeltaTime / menu_fade_seconds;
        if ( menu_alpha < target_alpha ) menu_alpha = ImMin( menu_alpha + step, target_alpha );
        else if ( menu_alpha > target_alpha ) menu_alpha = ImMax( menu_alpha - step, target_alpha );
        g_menu_alpha = menu_alpha;

        if ( menu_alpha <= 0.f ) {
            menu_alpha = 0.f;
            notify::draw( );
            return;
        }

        // Cosmetic -> Background: snow + particles end up in the background
        // draw list, which renders after the blur blit and before every
        // window - so they stay behind the interface but over the game
        {
            ImDrawList* background_list = ImGui::GetBackgroundDrawList( );
            const float effect_dt = ImMin( ImGui::GetIO( ).DeltaTime, 0.1f );
            const float screen_w = ImGui::GetIO( ).DisplaySize.x;
            const float screen_h = ImGui::GetIO( ).DisplaySize.y;
            if ( config::get( "cosmetic", "background_snow", 0 ) != 0 )
                draw_snow_effect( background_list, effect_dt, screen_w, screen_h,
                    config::get( "cosmetic", "background_snow_intensity", 50.f ) );
            if ( config::get( "cosmetic", "background_particles", 0 ) != 0 )
                draw_particles_effect( background_list, effect_dt, screen_w, screen_h,
                    config::get( "cosmetic", "background_particles_count", 110.f ),
                    config::get( "cosmetic", "background_particles_speed", 3.f ),
                    config::get( "cosmetic", "background_particles_distance", 130.f ) );
        }

        // menu opacity ( Cosmetic -> Menu -> Transparency ) scales the whole
        // menu uniformly: background, sidebar, texts and controls
        const float menu_opacity = 1.f - ImClamp(
            config::get( "cosmetic", "menu_transparency", 25.f ), 0.f, 90.f ) / 100.f;
        g_alpha = menu_alpha * menu_opacity;

        ImGui::SetNextWindowSize( SCALE( 880.f, 560.f ), ImGuiCond_Once );
        ImGui::SetNextWindowPos(
            ImVec2( ImGui::GetIO( ).DisplaySize.x * 0.5f, ImGui::GetIO( ).DisplaySize.y * 0.5f ),
            ImGuiCond_Once,
            ImVec2( 0.5f, 0.5f )
        );

        ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, ImVec2( 0.f, 0.f ) );
        ImGui::PushStyleVar( ImGuiStyleVar_WindowRounding, SCALE( 10.f ) );
        ImGui::PushStyleVar( ImGuiStyleVar_WindowBorderSize, 0.f );
        ImGui::PushStyleColor( ImGuiCol_WindowBg, ImGui::ColorConvertU32ToFloat4( fade( col_window_bg ) ) );

        if ( ImGui::Begin(
            "##voiden_menu",
            nullptr,
            ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoSavedSettings |
            ImGuiWindowFlags_NoBringToFrontOnFocus |
            ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_NoScrollWithMouse ) ) {
            const int window_stack_base = ImGui::GetCurrentContext( )->CurrentWindowStack.Size;
            try {
                draw_menu( );
            } catch ( ... ) {
                // a menu exception unwinds past popup End( ) calls - recovered below
            }
            // close anything a mid-menu early-out left open, so EndFrame's
            // Begin/End sanity check can never fire for menu-internal windows
            while ( ImGui::GetCurrentContext( )->CurrentWindowStack.Size > window_stack_base )
                ImGui::End( );
        }
        ImGui::End( );

        ImGui::PopStyleColor( );
        ImGui::PopStyleVar( 3 );

        g_alpha = 1.f;

        notify::draw( );
    }
}
