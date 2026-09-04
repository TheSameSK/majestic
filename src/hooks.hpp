#pragma once
#include "core/imports.h"
#include "kiero.h"

IMGUI_IMPL_API LRESULT  ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

using D3D11PresentHook = HRESULT(__stdcall*)(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags);
using D3D11ResizeBuffersHook = HRESULT(__stdcall*)(IDXGISwapChain* pSwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags);

inline D3D11PresentHook phookD3D11Present = nullptr;
inline D3D11ResizeBuffersHook phookD3D11ResizeBuffers = nullptr;

using namespace std::chrono_literals;
namespace hook {

	std::chrono::time_point<std::chrono::high_resolution_clock> resize_counter = std::chrono::high_resolution_clock::now();
	bool present_setup = false;
	static bool display_transition = false;

	inline void set_render_ready(bool ready) {
		Game.renderReady = ready;
		Game.renderReadySince = ready ? GetTickCount64() : 0;
	}

	inline void release_render_target_for_display_change(const char* reason) {
		set_render_ready(false);
		display_transition = true;

		if (renderer.pContext) {
			renderer.pContext->OMSetRenderTargets(0, nullptr, nullptr);
		}

		if (renderer.RenderTargetView) {
			renderer.RenderTargetView->Release();
			renderer.RenderTargetView = nullptr;
		}

		if (ImGui::GetCurrentContext()) {
			ImGui_ImplDX11_InvalidateDeviceObjects();
		}

		if (renderer.pContext) {
			renderer.pContext->Flush();
		}
	}

	inline int d3d_exception_filter(const char* tag, EXCEPTION_POINTERS* exception) {
		DWORD code = exception && exception->ExceptionRecord ? exception->ExceptionRecord->ExceptionCode : 0;
		NOCTUA_RUNTIME_LOG("SEH: %s 0x%08X", tag, code);
		crash_logger::log_exception(tag, exception);
		return EXCEPTION_EXECUTE_HANDLER;
	}

	typedef LRESULT(__stdcall* WINPROC) (_In_ HWND hWnd,
		_In_ UINT Msg,
		_In_ WPARAM wParam,
		_In_ LPARAM lParam);

	LRESULT CALLBACK hook_WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	inline LRESULT wndproc_inner(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	inline LRESULT call_original_wndproc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
		WNDPROC original = Game.originalWndProc;
		if (!original || original == hook_WndProc) {
			return DefWindowProc(hWnd, uMsg, wParam, lParam);
		}
		return CallWindowProc(original, hWnd, uMsg, wParam, lParam);
	}

	inline bool submit_keyboard_layout_char(HWND hWnd, WPARAM wParam) {
		if (IsWindowUnicode(hWnd)) return false;
		if (wParam < 0x80 || wParam > 0xFF) return false;

		UINT code_page = GetACP();
		char code_page_buf[16]{};
		const LANGID lang_id = LOWORD(reinterpret_cast<uintptr_t>(GetKeyboardLayout(0)));
		if (GetLocaleInfoA(MAKELCID(lang_id, SORT_DEFAULT), LOCALE_IDEFAULTANSICODEPAGE, code_page_buf, sizeof(code_page_buf)) > 0) {
			const int parsed_code_page = atoi(code_page_buf);
			if (parsed_code_page > 0) code_page = static_cast<UINT>(parsed_code_page);
		}

		const char input = static_cast<char>(wParam & 0xFF);
		wchar_t wide_char = 0;
		if (MultiByteToWideChar(code_page, MB_PRECOMPOSED, &input, 1, &wide_char, 1) <= 0 || wide_char == 0) {
			return false;
		}

		ImGui::GetIO().AddInputCharacter(static_cast<unsigned int>(wide_char));
		return true;
	}

	inline constexpr int kMaxWndProcDepth = 4;

	LRESULT CALLBACK hook_WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
		static thread_local int wndproc_depth = 0;
		if (wndproc_depth >= kMaxWndProcDepth) {
			return DefWindowProc(hWnd, uMsg, wParam, lParam);
		}

		++wndproc_depth;
		LRESULT result = 0;
		__try {
			result = wndproc_inner(hWnd, uMsg, wParam, lParam);
		}
		__except(d3d_exception_filter("wndproc", GetExceptionInformation())) {
			--wndproc_depth;
			return call_original_wndproc(hWnd, uMsg, wParam, lParam);
		}
		--wndproc_depth;
		return result;
	}

	inline LRESULT wndproc_inner(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
		static bool callOnce = false;
		if (!callOnce) {
			callOnce = true;
		}

		if (!Game.originalWndProc) {
			return DefWindowProc(hWnd, uMsg, wParam, lParam);
		}

		if (uMsg == WM_SYSKEYDOWN && wParam == VK_RETURN && (HIWORD(lParam) & KF_ALTDOWN)) {
			if (renderer.ready()) {
				release_render_target_for_display_change("alt_enter");
			}
			display_transition = true;
			return call_original_wndproc(hWnd, uMsg, wParam, lParam);
		}

		if (!Game.renderReady) {
			return call_original_wndproc(hWnd, uMsg, wParam, lParam);
		}

		if (uMsg == WM_KEYUP || uMsg == WM_LBUTTONUP || uMsg == WM_RBUTTONUP || uMsg == WM_XBUTTONUP || uMsg == WM_MBUTTONUP) {
			int param = 0;
			switch (uMsg)
			{
			case WM_KEYUP:
				param = static_cast<int>(wParam);
				break;
			case WM_LBUTTONUP:
				param = 1;
				break;
			case WM_RBUTTONUP:
				param = 2;
				break;
			case WM_MBUTTONUP:
				param = 4;
				break;
			case WM_XBUTTONUP:
				param = 4 + GET_XBUTTON_WPARAM(wParam);
				break;
			}
			if (param == config::get("menu", "menu_key", 35)) {
				Game.menuOpen = !Game.menuOpen;
			}
		}

		if (present_setup && Game.window && Game.renderReady && renderer.ready()) {
			ImGuiContext* ctx = ImGui::GetCurrentContext();
			if (ctx != 0) {
				POINT mPos;
				if (GetCursorPos(&mPos)) {
					ScreenToClient(Game.window, &mPos);
					ImGui::GetIO().MousePos.x = static_cast<float>(mPos.x);
					ImGui::GetIO().MousePos.y = static_cast<float>(mPos.y);
				}

				if (uMsg == WM_KEYUP) {
					if (wParam < 256) {
						Game.keyStates[wParam] = false;
					}
				}
				else if (uMsg == WM_KEYDOWN) {
					if (wParam < 256) {
						Game.keyStates[wParam] = true;
					}
				}
				else if (uMsg == WM_LBUTTONUP)   { Game.keyStates[1] = false; }
				else if (uMsg == WM_LBUTTONDOWN) { Game.keyStates[1] = true; }
				else if (uMsg == WM_RBUTTONUP)   { Game.keyStates[2] = false; }
				else if (uMsg == WM_RBUTTONDOWN) { Game.keyStates[2] = true; }
				else if (uMsg == WM_MBUTTONUP)   { Game.keyStates[4] = false; }
				else if (uMsg == WM_MBUTTONDOWN) { Game.keyStates[4] = true; }
				else if (uMsg == WM_XBUTTONUP) {
					int xb = GET_XBUTTON_WPARAM(wParam);
					Game.keyStates[4 + xb] = false;
				}
				else if (uMsg == WM_XBUTTONDOWN) {
					int xb = GET_XBUTTON_WPARAM(wParam);
					Game.keyStates[4 + xb] = true;
				}

				if (Game.menuOpen) {
					if (uMsg == WM_CHAR && submit_keyboard_layout_char(hWnd, wParam)) {
						return TRUE;
					}

					if (ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam)) {
						return TRUE;
					}

					switch (uMsg) {
					case WM_KEYDOWN:
					case WM_KEYUP:
					case WM_SYSKEYDOWN:
					case WM_SYSKEYUP:
					case WM_CHAR:
					case WM_UNICHAR:
					case WM_SYSCHAR:
					case WM_DEADCHAR:
					case WM_SYSDEADCHAR:
					case WM_LBUTTONDOWN:
					case WM_LBUTTONUP:
					case WM_LBUTTONDBLCLK:
					case WM_RBUTTONDOWN:
					case WM_RBUTTONUP:
					case WM_RBUTTONDBLCLK:
					case WM_MBUTTONDOWN:
					case WM_MBUTTONUP:
					case WM_MBUTTONDBLCLK:
					case WM_XBUTTONDOWN:
					case WM_XBUTTONUP:
					case WM_XBUTTONDBLCLK:
					case WM_MOUSEMOVE:
					case WM_MOUSEWHEEL:
					case WM_MOUSEHWHEEL:
					case WM_INPUT:
						return 0;
					default:
						break;
					}
				}
			}
		}

		return call_original_wndproc(hWnd, uMsg, wParam, lParam);
	}

	HRESULT __stdcall hookedD3D11ResizeBuffers(IDXGISwapChain* pSwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags) {
		if (!renderer.ready()) {
			return phookD3D11ResizeBuffers(pSwapChain, BufferCount, Width, Height, NewFormat, SwapChainFlags);
		}

		release_render_target_for_display_change("resize_buffers");
		add_log("d3d11 resize: invalidate device objects");

		const HRESULT hr = phookD3D11ResizeBuffers(pSwapChain, BufferCount, Width, Height, NewFormat, SwapChainFlags);
		if (FAILED(hr)) {
			add_log("d3d11 resize: ResizeBuffers failed");

			renderer.pSwapChain = pSwapChain;
			renderer.reset(0, 0);
			if (ImGui::GetCurrentContext()) {
				ImGui_ImplDX11_CreateDeviceObjects();
			}
			set_render_ready(renderer.ready() && renderer.RenderTargetView != 0);
			display_transition = false;
			present_setup = Game.renderReady;

			if (hr == DXGI_ERROR_INVALID_CALL && Game.renderReady) {
				return S_OK;
			}

			return hr;
		}

		renderer.pSwapChain = pSwapChain;
		renderer.reset(Width, Height);

		DXGI_SWAP_CHAIN_DESC desc{};
		if (SUCCEEDED(pSwapChain->GetDesc(&desc))) {
			Game.window = desc.OutputWindow;
		}

		if (ImGui::GetCurrentContext()) {
			ImGui_ImplDX11_CreateDeviceObjects();
		}

		set_render_ready(renderer.ready() && renderer.RenderTargetView != 0);
		display_transition = false;
		if (Game.renderReady) {
			add_log("d3d11 resize: device objects restored");
		} else {
			add_log("d3d11 resize: render target restore failed");
		}
		return hr;
	}
	int64_t reload_time = 2000;

	static ImGuiContext* g_main_ctx = nullptr;

	inline void render_frame() {
		bool frame_started = false;
		__try {
			if (!ImGui::GetCurrentContext()) return;
			if (Game.menuOpen) {
				ImGui::GetIO().WantCaptureMouse = true;
				ImGui::GetIO().ConfigFlags &= ~ImGuiConfigFlags_NoMouse;
			} else {
				ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NoMouse;
			}
			ImGui::GetIO().MouseDrawCursor = Game.menuOpen;

			runtime_debug::last_section = "render_begin_scene";
			if (!renderer.BeginScene()) {
				set_render_ready(false);
				return;
			}
			frame_started = true;
			runtime_debug::last_section = "game_render";
			game::game_render();
			runtime_debug::last_section = "render_end_scene";
			renderer.EndScene();
			frame_started = false;
			runtime_debug::last_section = "render_draw_data";
			if (!renderer.Render()) {
				set_render_ready(false);
				return;
			}
			runtime_debug::last_section = "render_frame_done";
		}
		__except(d3d_exception_filter("render_frame", GetExceptionInformation())) {
			if (frame_started) renderer.AbortScene();
			set_render_ready(false);
		}
	}

	HRESULT __stdcall hookedD3D11Present(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags) {
		if (!present_setup) {

			auto now = std::chrono::high_resolution_clock::now();

			auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(now - resize_counter).count();
			if (dur > reload_time) {
				present_setup = true;

				DXGI_SWAP_CHAIN_DESC desc;

				if (SUCCEEDED(pSwapChain->GetDesc(&desc))) {
					Game.window = desc.OutputWindow;

					try {
						renderer.SetResourceLoad(load_image_assets);
						renderer.Initialize(Game.window, pSwapChain);
						add_log("renderer initialized");

						{
							ID3D11Device* _hud_dev = nullptr;
							pSwapChain->GetDevice(__uuidof(ID3D11Device), (void**)&_hud_dev);
							if (_hud_dev) {
								hud::s_device = _hud_dev;
								_hud_dev->Release();
							}
						}

						g_main_ctx = ImGui::GetCurrentContext();
						set_render_ready(true);
						display_transition = false;
					}
					catch (const std::exception&) {
					}
					catch (...) {
						add_log("Unknown exception in renderer init");
					}
				}

				resize_counter = std::chrono::high_resolution_clock::now();
			}
		}
		if (display_transition) {
			return phookD3D11Present(pSwapChain, SyncInterval, Flags);
		}

		if (!Game.renderReady && present_setup) {
			resize_counter = std::chrono::high_resolution_clock::now();
			reload_time = 5000;
			add_log("waiting for init");
			present_setup = false;
		}
		if (!Game.renderReady) return phookD3D11Present(pSwapChain, SyncInterval, Flags);

		WINDOWINFO info;
		info.cbSize = sizeof(WINDOWINFO);
		if (!GetWindowInfo(Game.window, &info)) {
			return phookD3D11Present(pSwapChain, SyncInterval, Flags);
		}
		Game.screen.x = ((info.rcClient.right) - (info.rcClient.left));
		Game.screen.y = ((info.rcClient.bottom) - (info.rcClient.top));

		render_frame();
		return phookD3D11Present(pSwapChain, SyncInterval, Flags);
	}

	void call_thread(game_thread* this_ptr, int ops_to_execute) {
		if ((
			(this_ptr->m_context.m_script_hash == 0x26FB4AB9 || this_ptr->m_context.m_script_hash == 1104607124 || this_ptr->m_context.m_script_hash == 3381724246) ||
			(this_ptr->m_context.m_script_hash == 2003913879 || this_ptr->m_context.m_script_hash == 3522812357)
			)) {
			auto& thread = game_thread::get();
			auto orig_thread = thread;
			thread = this_ptr;
			game_fiber->on_tick();
			thread = orig_thread;
		}
	}

	void on_native_thread() {

		static uint64_t    last = 0;
		uint64_t        cur = *(uint64_t*)(FrameCount);
		if (last != cur) {
			last = cur;

			game::game_tick();

			tick::pnative.on_tick();
		}

		game_fiber->wait(0);
	}

	void call_fiber() {
		add_log("init fiber");
		game_fiber = std::make_unique<fiber::fiber_task>(static_cast<HMODULE>(nullptr), on_native_thread);
	}

	uintptr_t hooked_native_thread(game_thread* this_ptr, int ops_to_execute) {
		call_thread(this_ptr, ops_to_execute);
		return original_native_thread(this_ptr, ops_to_execute);
	}

	uintptr_t original_set_shoot_at_coords = 0;
	void on_shoot_at_coords(native::nsdk::NativeContext& context) {
		add_log("on_shoot_at_coords");
		reinterpret_cast<decltype(&on_shoot_at_coords)>(original_set_shoot_at_coords)(context);
	}
	uintptr_t original_task_shoot_at_coord = 0;
	void on_task_shoot_at_coord(native::nsdk::NativeContext& context) {
		add_log("task_shoot_at_coord");
		reinterpret_cast<decltype(&on_task_shoot_at_coord)>(original_task_shoot_at_coord)(context);
	}

	uintptr_t original_get_is_task_active = 0;
	void on_get_is_task_active(native::nsdk::NativeContext& context) {
		int32_t task = context.GetArgument<int32_t>(1);
		reinterpret_cast<decltype(&on_get_is_task_active)>(original_get_is_task_active)(context);
	}

	uintptr_t original_set_vehicle_engine_on = 0;
	void on_set_vehicle_engine_on(native::nsdk::NativeContext& context) {
		reinterpret_cast<decltype(&on_set_vehicle_engine_on)>(original_set_vehicle_engine_on)(context);
	}
	uintptr_t original_task_leave_vehicle = 0;
	void on_task_leave_vehicle(native::nsdk::NativeContext& context) {
		if (config::get("misc", "no_force_exit", 0)) {
			auto ped = hacks::local_ped_handle();
			if (context.GetArgument<int32_t>(0) == ped)
				return;
		}
		reinterpret_cast<decltype(&on_task_leave_vehicle)>(original_task_leave_vehicle)(context);
	}

	uintptr_t original_get_player_invincible = 0;
	void on_get_player_invincible(native::nsdk::NativeContext& context) {
		if (config::get("misc", "spoof_godmode_checks", 0)) {
			auto ped = hacks::local_ped_handle();
			if (context.GetArgument<int32_t>(0) == ped) {
				context.SetResult<bool>(0, false);
			}
		}
		reinterpret_cast<decltype(&on_get_player_invincible)>(original_get_player_invincible)(context);
	}

	uintptr_t original_create_ped = 0;
	void on_create_ped(native::nsdk::NativeContext& context) {
		if (config::get("misc", "hide_anticheat_npcs", 0)) {
			context.SetArgument<bool>(2, 0.f);
			context.SetArgument<bool>(3, 0.f);
			context.SetArgument<bool>(4, 0.f);
		}
		reinterpret_cast<decltype(&on_create_ped)>(original_create_ped)(context);
	}

	uintptr_t hooked_gta_gun_shot(
		__int64 a1,
		__int64 a2,
		float* a3,
		float* a4,
		unsigned int a5,
		unsigned int a6,
		unsigned int a7,
		int a8,
		char a9,
		int a10,
		char a11,
		char a12) {
		;
		return original_gta_event_gun_shot(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12);
	}

	uintptr_t original_clear_ped_tasks_immediately = 0;
	void on_clear_ped_tasks_immediately(native::nsdk::NativeContext& context) {
		if (config::get("misc", "spoof_clear_ped_tasks", 0)) {
			auto ped = hacks::local_ped_handle();
			if (context.GetArgument<int32_t>(0) == ped) {
				if (Game.allowClearTasks) {
					Game.allowClearTasks = false;
				}
				else {
					return;
				}
			}
		}
		reinterpret_cast<decltype(&on_clear_ped_tasks_immediately)>(original_clear_ped_tasks_immediately)(context);
	}

	bool hook_game() {
		

		call_fiber();
		MH_CreateHook(gta_script_thread_tick, hooked_native_thread, reinterpret_cast<void**>(&original_native_thread));
		if (gta_script_thread_tick) {
			if (MH_EnableHook(gta_script_thread_tick) == MH_OK) {
			}
		}
		auto set_vehicle_engine_on = native::invoker::find_native_handler(0x2497c4717c8b881e);
		if (set_vehicle_engine_on) {
			MH_CreateHook(set_vehicle_engine_on, on_set_vehicle_engine_on, reinterpret_cast<void**>(&original_set_vehicle_engine_on));
			if (MH_EnableHook(set_vehicle_engine_on) == MH_OK) {
			}
		}

		auto task_leave_vehicle = native::invoker::find_native_handler(0xd3dbce61a490be02);
		if (task_leave_vehicle) {
			MH_CreateHook(task_leave_vehicle, on_task_leave_vehicle, reinterpret_cast<void**>(&original_task_leave_vehicle));
			if (MH_EnableHook(task_leave_vehicle) == MH_OK) {
				add_log("task_leave_vehicle ok");
			}
		}

		auto get_player_invincible = native::invoker::find_native_handler(0xb721981b2b939e07);
		if (get_player_invincible) {
			MH_CreateHook(get_player_invincible, on_get_player_invincible, reinterpret_cast<void**>(&original_get_player_invincible));
			if (MH_EnableHook(get_player_invincible) == MH_OK) {
				add_log("get_player_invincible ok");
			}
		}

		auto create_ped = native::invoker::find_native_handler(0xd49f9b0955c367de);
		if (create_ped) {
			MH_CreateHook(create_ped, on_create_ped, reinterpret_cast<void**>(&original_create_ped));
			if (MH_EnableHook(create_ped) == MH_OK) {
				add_log("create_ped ok");
			}
		}

	

		

		

		

		add_log("game hooks done");
		return true;
	}
	
	struct wndproc_hook_state {
		bool installed = false;
		HWND hwnd = nullptr;
	};

	inline wndproc_hook_state g_wndproc_state{};

	inline void wndproc_uninstall_from(HWND hwnd) {
		if (!hwnd || !IsWindow(hwnd)) return;
		if (GetWindowLongPtr(hwnd, GWLP_WNDPROC) != (LONG_PTR)hook_WndProc) return;
		if (!Game.originalWndProc) return;
		SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)Game.originalWndProc);
	}

	void patch_wndproc() {
		if (!present_setup) return;
		if (!Game.window) return;
		if (!Game.renderReady) return;

		if (g_wndproc_state.installed && g_wndproc_state.hwnd == Game.window) {
			return;
		}

		if (!IsWindow(Game.window)) {
			add_log("window is no longer valid");
			return;
		}

		LONG_PTR currentWndProc = GetWindowLongPtr(Game.window, GWLP_WNDPROC);
		if (currentWndProc == 0) {
			add_log("failed to get current wndproc");
			return;
		}

		if (currentWndProc == (LONG_PTR)hook_WndProc) {
			g_wndproc_state.installed = true;
			g_wndproc_state.hwnd = Game.window;
			add_log("wndproc hook already installed");
			return;
		}

		const bool migrating = g_wndproc_state.installed && g_wndproc_state.hwnd != Game.window;

		SetLastError(0);

		WNDPROC previous = (WNDPROC)SetWindowLongPtr(Game.window, GWLP_WNDPROC, (LONG_PTR)hook_WndProc);

		DWORD err = GetLastError();
		if ((uintptr_t)previous == 0 && err != 0) {
			Game.originalWndProc = 0;
			add_log("wndproc hook install failed");
			return;
		}

		if (migrating) {
			wndproc_uninstall_from(g_wndproc_state.hwnd);
		}

		if ((uintptr_t)previous != 0 && previous != hook_WndProc) {
			Game.originalWndProc = previous;
		}

		g_wndproc_state.installed = true;
		g_wndproc_state.hwnd = Game.window;
		add_log("wndproc hook installed successfully");
	}

	bool hook_renderer() {
		add_log("hook renderer");
		logs::logSystemInfo();

		if (kiero::init(kiero::RenderType::D3D11) != kiero::Status::Success) {
			add_log("d3d11 init failed");
			return false;
		}

		if (kiero::bind(8, reinterpret_cast<void**>(&phookD3D11Present), reinterpret_cast<void*>(hookedD3D11Present)) != kiero::Status::Success) {
			add_log("present hook failed");
			return false;
		}

		if (kiero::bind(13, reinterpret_cast<void**>(&phookD3D11ResizeBuffers), reinterpret_cast<void*>(hookedD3D11ResizeBuffers)) != kiero::Status::Success) {
			add_log("resize hook failed");
			return false;
		}

		Game.renderMethod = 2;
		add_log("d3d11 hooked");
		return true;
	}

bool disable() {
        add_log("Unloading");

        if (g_wndproc_state.installed && g_wndproc_state.hwnd && IsWindow(g_wndproc_state.hwnd)) {
            if (GetWindowLongPtr(g_wndproc_state.hwnd, GWLP_WNDPROC) == (LONG_PTR)hook_WndProc) {
                if (Game.originalWndProc) {
                    SetWindowLongPtr(g_wndproc_state.hwnd, GWLP_WNDPROC, (LONG_PTR)Game.originalWndProc);
                }
            }
        }
        Game.originalWndProc = 0;
        g_wndproc_state = {};

        Game.tick = nullptr;
        Game.window = nullptr;
        Game.renderReady = false;
        Game.menuOpen = false;

        present_setup = false;
        display_transition = false;
        resize_counter = std::chrono::high_resolution_clock::now();
        reload_time = 2000;
        g_main_ctx = nullptr;

        renderer.release();

        kiero::shutdown();
        MH_DisableHook(MH_ALL_HOOKS);
        MH_RemoveHook(MH_ALL_HOOKS);
        MH_Uninitialize();

        return true;
    }
	bool Ready() {
		if ((GetModuleHandleA("d3d11.dll") != NULL) || (GetModuleHandleA("dxgi.dll") != NULL)) return true;
		return false;
	}

	bool enable() {
		add_log("hook enable");
		while (Game.running == false) {
			if (Ready()) {
				add_log("d3d detected");
				if (MH_Initialize() != MH_OK) {
					add_log("minhook init failed");
					return false;
				}
				add_log("minhook ok");
				do {
					std::this_thread::sleep_for(std::chrono::milliseconds(1000));
					add_log("waiting for game");
				} while (*Game.gta_game_state != game_state_t::playing);
				add_log("game playing");

				bool rendererResult = hook_renderer();

				bool gameResult = hook_game();

				Game.tick = game::game_tick;

				Game.menuOpen = config::get("menu", "openOnLoad", 1);

				return rendererResult && gameResult;
			}
			std::this_thread::sleep_for(1000ms);
		}
		return false;
	}
}


// ============================================================================
//  NOCTUA LICENSED BUILD WATERMARK  (auto-generated, do not remove)
//  Inert per-file identifier required by the EULA. It uniquely tags this
//  source file so this program is distinguishable from any other build.
//  Contains no functional logic.
//  file: src/hooks.hpp
// ============================================================================
#ifndef NOCTUA_LICENSE_MARK_91eeee5e7d4aaacbc3be7d732d7aee79
#define NOCTUA_LICENSE_MARK_91eeee5e7d4aaacbc3be7d732d7aee79
namespace noctua_license { namespace mark_91eeee5e7d4aaacbc3be7d732d7aee79 {
    inline constexpr unsigned long long kMarkId = 0xb2623a5c840ebbddull;
    inline constexpr char kMarkFile[] = "src/hooks.hpp";
    inline constexpr unsigned char kMarkEntropy[] = { 0x37, 0x1b, 0x86, 0x01, 0x9b, 0x5b, 0x40, 0x3d, 0x53, 0x84, 0x23, 0xca, 0x1c, 0xa5, 0x75, 0x85 };
    inline constexpr unsigned long  kMarkSalts[]  = { 0x39f97d69ul, 0x05fc42eful, 0xfc0cc4ccul, 0x6b4d36dcul, 0x1310a60cul, 0xbcc53cf2ul, 0x0adf2b71ul, 0xc9166d0aul };
    [[maybe_unused]] inline unsigned long long mark_digest() noexcept {
        unsigned long long h = 1469598103934665603ull ^ kMarkId;
        for (unsigned char b : kMarkEntropy) { h ^= b; h *= 1099511628211ull; }
        for (unsigned long  s : kMarkSalts)  { h ^= s; h *= 1099511628211ull; }
        return h;
    }
} } // namespace noctua_license::mark_91eeee5e7d4aaacbc3be7d732d7aee79
#endif // NOCTUA_LICENSE_MARK_91eeee5e7d4aaacbc3be7d732d7aee79
