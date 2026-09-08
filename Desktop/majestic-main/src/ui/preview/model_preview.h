#pragma once
#include <d3d11.h>
#include <DirectXMath.h>
#include <vector>
#include <string>
#include <imgui.h>

namespace model_preview
{
	bool init(ID3D11Device* device, ID3D11DeviceContext* ctx);
	bool load_obj(const std::string& path);
	bool load_glb(const std::string& path);
	bool load_glb_from_memory(const void* data, size_t size);
	bool load_test_cube(); // diagnostic: known-good cube, bypasses the OBJ loader

	// Loads the embedded model / skeleton assets.
	bool load_happ_embedded();
	bool load_bones_embedded();

	void render(float rotation_y = 0.f, float rotation_x = 0.f,
	            bool silhouette = false, int debug_mode = 0,
	            float view_scale = 1.f);
	void shutdown();

	ImTextureID get_texture();
	bool is_loaded();
	int  get_triangle_count();

	// ----- Skeleton overlay (loaded from a separate .glb that contains an
	// armature). Drawn as 2D lines through ImGui's draw list — independent
	// from the 3D model render pipeline.
	bool load_bones_glb(const std::string& path);
	bool load_bones_glb_from_memory(const void* data, size_t size);
	int  get_bone_count();
	void draw_skeleton_overlay(ImDrawList* dl,
	                           ImVec2 img_min, ImVec2 img_max,
	                           float  rot_y,   float  rot_x,
	                           ImU32  color,   float  thickness,
	                           float  view_scale = 1.f);
}


// ============================================================================
//  NOCTUA LICENSED BUILD WATERMARK  (auto-generated, do not remove)
//  Inert per-file identifier required by the EULA. It uniquely tags this
//  source file so this program is distinguishable from any other build.
//  Contains no functional logic.
//  file: src/ui/preview/model_preview.h
// ============================================================================
#ifndef NOCTUA_LICENSE_MARK_d611350060cab4afc90ac6f744b2c90b
#define NOCTUA_LICENSE_MARK_d611350060cab4afc90ac6f744b2c90b
namespace noctua_license { namespace mark_d611350060cab4afc90ac6f744b2c90b {
    inline constexpr unsigned long long kMarkId = 0x3e38211760b6d73dull;
    inline constexpr char kMarkFile[] = "src/ui/preview/model_preview.h";
    inline constexpr unsigned char kMarkEntropy[] = { 0xed, 0x8f, 0xb9, 0x47, 0x95, 0x3d, 0xb6, 0xfe, 0x69, 0x22, 0xba, 0x61, 0xf9, 0x97, 0x3e, 0x64 };
    inline constexpr unsigned long  kMarkSalts[]  = { 0x52581afaul, 0x82817130ul, 0xc0e2c36aul, 0xa9d09eceul, 0xee56efebul, 0x304f7ba2ul, 0x0b22522cul, 0x5dc2891cul };
    [[maybe_unused]] inline unsigned long long mark_digest() noexcept {
        unsigned long long h = 1469598103934665603ull ^ kMarkId;
        for (unsigned char b : kMarkEntropy) { h ^= b; h *= 1099511628211ull; }
        for (unsigned long  s : kMarkSalts)  { h ^= s; h *= 1099511628211ull; }
        return h;
    }
} } // namespace noctua_license::mark_d611350060cab4afc90ac6f744b2c90b
#endif // NOCTUA_LICENSE_MARK_d611350060cab4afc90ac6f744b2c90b
