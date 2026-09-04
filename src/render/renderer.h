#pragma once
#include <fstream>
#include <iostream>
#include <ctime>

#include <dwmapi.h> 
#include <TlHelp32.h>
#include <d3d11.h>
#pragma comment(lib, "d3d11.lib")
#include "../imgui/imgui.h"

	

#include "../imgui/imgui.h"
#include "../imgui/imgui_impl_dx11.h"
#include "../imgui/imgui_impl_win32.h"
#include "../imgui/imgui_internal.h"

struct RGBA {
	RGBA(int r, int g, int b, int a) :
		r(r), g(g), b(b), a(a) { }

	int r ;
	int g ;
	int b ;
	int a ;
};

class imgui_render {
public:
	ImGuiIO io;

	HRESULT hres;
	D3D11_VIEWPORT viewport;
	IDXGISwapChain* pSwapChain = 0;
	ID3D11Device* pDevice = 0;
	ID3D11DeviceContext* pContext = 0;
	ID3D11RenderTargetView* RenderTargetView = 0;
	typedef void (*vResourceLoadCall)(ID3D11Device*);

	vResourceLoadCall loadCall = 0;

	void SetResourceLoad(vResourceLoadCall funct2);

	void Initialize(HWND targetWindow, IDXGISwapChain* pSwapchain);
	
	bool BeginScene();
	void EndScene();
	void AbortScene();

	bool Render();
	bool ready();
	void release();
	void reset(UINT Width, UINT Height);

	float RenderText(const std::string& text, const ImVec2& position, float size, RGBA color, bool center,bool outine = false);
	void RenderLine(const ImVec2& from, const ImVec2& to,  RGBA color, float thickness = 1.0f);
	void RenderCircle(const ImVec2& position, float radius, RGBA color, float thickness = 1.0f, uint32_t segments = 16);
	void RenderCircleFilled(const ImVec2& position, float radius, RGBA color, uint32_t segments = 16);
	void RenderRect(const ImVec2& from, const ImVec2& to, RGBA color, float rounding = 0.0f, uint32_t roundingCornersFlags = ImDrawFlags_RoundCornersAll, float thickness = 1.0f);
	void RenderDot(const ImVec2& from, const ImVec2& to, RGBA color, float thickness = 1.0f);
	void RenderRectFilled(const ImVec2& from, const ImVec2& to,RGBA color, float rounding = 0.0f, uint32_t roundingCornersFlags = ImDrawFlags_RoundCornersAll);

	ImVec2 RenderMenuRow(const ImVec2& from,float width);
	void RenderMenuSwitch(const ImVec2& from,std::string value);
	void RenderMenuValue(const ImVec2& from,float value);
	ImFont* EspBaseFont(float desiredSize, float* renderSize = nullptr);
	ImFont* EspNameFont(float desiredSize, float* renderSize = nullptr);
	ImFont* EspSmallFont(float desiredSize, float* renderSize = nullptr);
	ImFont* EspSmallFont(float* renderSize = nullptr);
	ImFont* EspWeaponIconFont(float desiredSize, float* renderSize = nullptr);
private:

	bool finishedInit = false;
	bool init_fonts = false;
	ImFont* imFont;
public:
	ImFont* espFont = nullptr;
	ImFont* smallFont = nullptr;
	ImFont* tahomaBoldFont = nullptr;
	ImFont* verdanaBoldFont = nullptr;
	ImFont* calibriFont = nullptr;
	ImFont* calibriIndicatorFont = nullptr;
	ImFont* hudFont = nullptr;
};


// ============================================================================
//  NOCTUA LICENSED BUILD WATERMARK  (auto-generated, do not remove)
//  Inert per-file identifier required by the EULA. It uniquely tags this
//  source file so this program is distinguishable from any other build.
//  Contains no functional logic.
//  file: src/render/renderer.h
// ============================================================================
#ifndef NOCTUA_LICENSE_MARK_578fad527c767102a3187dad346c99e3
#define NOCTUA_LICENSE_MARK_578fad527c767102a3187dad346c99e3
namespace noctua_license { namespace mark_578fad527c767102a3187dad346c99e3 {
    inline constexpr unsigned long long kMarkId = 0x6ab091184bab1cc1ull;
    inline constexpr char kMarkFile[] = "src/render/renderer.h";
    inline constexpr unsigned char kMarkEntropy[] = { 0x70, 0x1b, 0x58, 0x68, 0x51, 0x8d, 0xe7, 0x62, 0x95, 0x68, 0x81, 0xc7, 0x6c, 0x91, 0xad, 0xc9 };
    inline constexpr unsigned long  kMarkSalts[]  = { 0xc5db9694ul, 0x9a9d16aful, 0x284860aful, 0x6436ba1eul, 0x4f5a4857ul, 0x5277b49ful, 0x57322c4bul, 0x39bf022aul };
    [[maybe_unused]] inline unsigned long long mark_digest() noexcept {
        unsigned long long h = 1469598103934665603ull ^ kMarkId;
        for (unsigned char b : kMarkEntropy) { h ^= b; h *= 1099511628211ull; }
        for (unsigned long  s : kMarkSalts)  { h ^= s; h *= 1099511628211ull; }
        return h;
    }
} } // namespace noctua_license::mark_578fad527c767102a3187dad346c99e3
#endif // NOCTUA_LICENSE_MARK_578fad527c767102a3187dad346c99e3
