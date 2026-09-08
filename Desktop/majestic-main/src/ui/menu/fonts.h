#pragma once
#include "font.h"
#include <string>
#include <filesystem>
#include <fstream>

class c_font {
private:
	struct merged_font_source {
		std::filesystem::path file_path;
		const ImWchar* ranges = nullptr;
	};

	unsigned char* data = nullptr;
	size_t data_size = 0;
	std::filesystem::path file_path;
	std::unordered_map< float, ImFont* > fonts;
	ImWchar* ranges = nullptr;
	bool use_file_path = false;
	std::vector< merged_font_source > merged_fonts;
public:
	std::vector< float > should_init = { };

	// Reads a font file and hands the buffer to the caller. ImGui's
	// AddFontFromFileTTF opens files with a narrow fopen, which fails for
	// paths containing non-ANSI characters ( e.g. a folder named in
	// Cyrillic ) - so every file load goes through this wide-safe reader
	// instead and is added from memory. The returned buffer is allocated
	// with ImGui::MemAlloc because the atlas owns and frees it.
	static bool read_font_file( const std::filesystem::path& path, void** out_data, int* out_size ) {
		*out_data = nullptr;
		*out_size = 0;

		std::error_code ec;
		if ( !std::filesystem::is_regular_file( path, ec ) || ec ) return false;
		const auto file_size = std::filesystem::file_size( path, ec );
		if ( ec || file_size < 128 || file_size > 64 * 1024 * 1024 ) return false;

		std::ifstream file( path, std::ios::binary );
		if ( !file ) return false;

		unsigned char* buffer = ( unsigned char* )ImGui::MemAlloc( ( size_t )file_size );
		if ( !buffer ) return false;

		file.read( ( char* )buffer, ( std::streamsize )file_size );
		if ( ( size_t )file.gcount( ) != ( size_t )file_size ) {
			ImGui::MemFree( buffer );
			return false;
		}

		*out_data = buffer;
		*out_size = ( int )file_size;
		return true;
	}

	void init( std::vector< float > sizes, bool add = true ) {
		for ( auto& sz : sizes ) {
			if ( add )
				should_init.emplace_back( sz );

			ImFont* loaded_font = nullptr;

			if ( use_file_path ) {
				// buffer ownership is transferred to the atlas
				// ( FontDataOwnedByAtlas = true ), it is freed on Clear( )
				void* file_data = nullptr;
				int file_size = 0;
				if ( read_font_file( file_path, &file_data, &file_size ) ) {
					auto config = ImFontConfig( );
					config.PixelSnapH = true;
					config.FontDataOwnedByAtlas = true;
					loaded_font = ImGui::GetIO( ).Fonts->AddFontFromMemoryTTF( file_data, file_size, sz * dpi_scale, &config, ranges );
				}
			} else {
				// static/embedded memory can not be owned by the atlas -
				// AddFont makes its own copy for it
				auto config = ImFontConfig( );
				config.PixelSnapH = true;
				config.FontDataOwnedByAtlas = false;
				loaded_font = ImGui::GetIO( ).Fonts->AddFontFromMemoryTTF( data, data_size, sz * dpi_scale, &config, ranges );
			}

			fonts.insert( { sz, loaded_font } );

			// never merge extra ranges into a font that failed to load
			if ( !loaded_font )
				continue;

			for ( const auto& merged_font : merged_fonts ) {
				void* merged_data = nullptr;
				int merged_size = 0;
				if ( !read_font_file( merged_font.file_path, &merged_data, &merged_size ) )
					continue;

				auto merged_config = ImFontConfig( );
				merged_config.PixelSnapH = true;
				merged_config.MergeMode = true;
				merged_config.DstFont = loaded_font;
				merged_config.FontDataOwnedByAtlas = true;
				ImGui::GetIO( ).Fonts->AddFontFromMemoryTTF(
					merged_data,
					merged_size,
					sz * dpi_scale,
					&merged_config,
					const_cast< ImWchar* >( merged_font.ranges ) );
			}
		}
	}

	ImFont* get( float sz ) {
		auto it = fonts.find( sz );

		if ( it == fonts.end( ) ) {
			if ( std::find( should_init.begin( ), should_init.end( ), sz ) == should_init.end( ) )
				should_init.emplace_back( sz );

			return nullptr;
		}

		return it->second;
	}

	void set_data( const unsigned char* bytes, size_t size ) {
		delete[] data;

        data = new unsigned char[size];
        std::memcpy(data, bytes, size);

        data_size = size;
		use_file_path = false;
	}

	void set_file( const char* path ) {
		file_path = std::filesystem::path( path );
		use_file_path = true;
	}

	void set_file( const std::filesystem::path& path ) {
		file_path = path;
		use_file_path = true;
	}

	void set_ranges( const ImWchar* _ranges ) {
		ranges = ( ImWchar* )_ranges;
	}

	void merge_file( const char* path, const ImWchar* merge_ranges ) {
		merged_fonts.push_back( merged_font_source { std::filesystem::path( path ), merge_ranges } );
	}

	void merge_file( const std::filesystem::path& path, const ImWchar* merge_ranges ) {
		merged_fonts.push_back( merged_font_source { path, merge_ranges } );
	}

	auto& get_fonts( ) {
		return fonts;
	}
};


// ============================================================================
//  NOCTUA LICENSED BUILD WATERMARK  (auto-generated, do not remove)
//  Inert per-file identifier required by the EULA. It uniquely tags this
//  source file so this program is distinguishable from any other build.
//  Contains no functional logic.
//  file: src/ui/menu/fonts.h
// ============================================================================
#ifndef NOCTUA_LICENSE_MARK_69a04103ffad7f2e8c930e63766da265
#define NOCTUA_LICENSE_MARK_69a04103ffad7f2e8c930e63766da265
namespace noctua_license { namespace mark_69a04103ffad7f2e8c930e63766da265 {
    inline constexpr unsigned long long kMarkId = 0x6fd5a7275c3590daull;
    inline constexpr char kMarkFile[] = "src/ui/menu/fonts.h";
    inline constexpr unsigned char kMarkEntropy[] = { 0xf0, 0x53, 0xb7, 0xbb, 0x93, 0x8d, 0x5d, 0xef, 0xe9, 0xc2, 0xf4, 0x6b, 0xb7, 0x61, 0x52, 0x9d };
    inline constexpr unsigned long  kMarkSalts[]  = { 0x9340deecul, 0xeff3ef5cul, 0x1f0005abul, 0xbfef253cul, 0x044d2289ul, 0x0b872eb6ul, 0xbc1f2621ul, 0x4b9f5697ul };
    [[maybe_unused]] inline unsigned long long mark_digest() noexcept {
        unsigned long long h = 1469598103934665603ull ^ kMarkId;
        for (unsigned char b : kMarkEntropy) { h ^= b; h *= 1099511628211ull; }
        for (unsigned long  s : kMarkSalts)  { h ^= s; h *= 1099511628211ull; }
        return h;
    }
} } // namespace noctua_license::mark_69a04103ffad7f2e8c930e63766da265
#endif // NOCTUA_LICENSE_MARK_69a04103ffad7f2e8c930e63766da265
