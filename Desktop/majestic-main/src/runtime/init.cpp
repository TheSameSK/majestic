#include "runtime/init.hpp"

#include "core/imports.h"
#include "executor/executor.hpp"
#include "features/misc/freecam.hpp"
#include "network/ws_bridge.hpp"
#include "platform/build_profile.hpp"
#include "platform/crash_logger.hpp"
#include "platform/debug.hpp"
#include "platform/vmp.hpp"
#include "runtime/context.hpp"
#include "runtime/session.hpp"
#include "runtime/tasks.hpp"
#include "runtime/tls_gate.hpp"

#include <imgui.h>
#include <cstdio>
#include <ctime>

#pragma comment(lib, "winmm.lib")

namespace {
    bool g_initialized = false;

    void reset_initialization_state() {
        g_initialized = false;
    }

    void cleanup_runtime_resources() {
        NOCTUA_RUNTIME_LOG("cleanup: imgui context");
        if (ImGui::GetCurrentContext()) {
            ImGui::DestroyContext();
        }

        NOCTUA_RUNTIME_LOG("cleanup: game_fiber");
        game_fiber.reset();

        NOCTUA_RUNTIME_LOG("cleanup: crash_logger");
        crash_logger::uninstall();

        NOCTUA_RUNTIME_LOG("cleanup: tls_gate");
        tls_gate::cleanup();
    }

    void patch_wnd_proc() {
        __try {
            hook::patch_wndproc();
        } __except(EXCEPTION_EXECUTE_HANDLER) {
            NOCTUA_RUNTIME_LOG("SEH: patch_wndproc 0x%08X", GetExceptionCode());
        }
    }

    bool initialize_release_runtime() {
        NOCTUA_VMP_SCOPE_ULTRA("initialize_release_runtime");
        runtime_context::context_view context{};
        if (!runtime_context::read(context)) {
            if constexpr (build_profile::debug) {
                NOCTUA_RUNTIME_LOG("debug runtime: offline direct-inject mode");
                return true;
            }
            NOCTUA_RUNTIME_LOG("release runtime failed: context read failed");
            return false;
        }

        NOCTUA_RUNTIME_LOG(
            "runtime context: session_len=%zu product=%s tg_len=%zu hwid_len=%zu seed_empty=%d",
            context.product_session.size(),
            context.product_code.c_str(),
            context.tg_id.size(),
            context.hwid.size(),
            runtime_context::seed_is_empty(context.context_seed) ? 1 : 0
        );

        runtime_session::state session{};
        session.ready = true;
        strncpy_s(session.product_session, context.product_session.c_str(), _TRUNCATE);
        strncpy_s(session.product_code, context.product_code.empty() ? "altv" : context.product_code.c_str(), _TRUNCATE);
        strncpy_s(session.hwid, context.hwid.c_str(), _TRUNCATE);
        session.runtime_seed = context.context_seed;
        session.pointer_xor_seed = context.context_seed;
        runtime_session::set(session);

        if constexpr (build_profile::production) {
            tls_gate::set_stage(1);
        }

        config::deserialize_from_json("{\"normal\":{},\"string_map\":{},\"dword_map\":{},\"vector3_map\":{}}");
        if (!preload_freecam_script()) {
            return false;
        }
        return true;
    }

    void initialize() {
        runtime_debug::last_section = "initialize_start";
        NOCTUA_RUNTIME_LOG("initialize started");

        NOCTUA_VMP_BEGIN_ULTRA("initialize.release_gate");
        if (!initialize_release_runtime()) {
            NOCTUA_VMP_END();
            return;
        }
        NOCTUA_VMP_END();
        if (!g_initialized) {
            g_initialized = true;
        }
        __try { runtime_debug::last_section = "config_load"; config::load(); }
        __except(EXCEPTION_EXECUTE_HANDLER) { NOCTUA_RUNTIME_LOG("SEH: config::load 0x%08X", GetExceptionCode()); }

        runtime_debug::last_section = "findAllPatterns";
        NOCTUA_RUNTIME_LOG("calling findAllPatterns...");
        if (pointers::findAllPatterns()) {
            runtime_debug::last_section = "hook_enable";
            NOCTUA_RUNTIME_LOG("calling hook::enable...");
            if (hook::enable()) {
                Game.running = true;
                NOCTUA_RUNTIME_LOG("hook::enable OK, Game.running=true");
                executor::start_early_monitoring();

                runtime_tasks::start_ws_server();
                __try { runtime_debug::last_section = "aimbot_init"; runtime_tasks::start_aimbot(); }
                __except(EXCEPTION_EXECUTE_HANDLER) { NOCTUA_RUNTIME_LOG("SEH: aimbot::init 0x%08X", GetExceptionCode()); }
            }
        }

        runtime_debug::last_section = "main_loop";
        NOCTUA_RUNTIME_LOG("entering main loop");
        while (Game.running) {
            if (runtime_session::unload_requested()) {
                Game.running = false;
                break;
            }
            patch_wnd_proc();
            std::this_thread::sleep_for(500ms);
        }

        Game.running = false;

        NOCTUA_RUNTIME_LOG("shutdown: aimbot");
        __try { aimbot::shutdown(); } __except(EXCEPTION_EXECUTE_HANDLER) {}
        NOCTUA_RUNTIME_LOG("shutdown: unload scripts");
        __try { ws_server::unload_user_scripts_for_shutdown(); } __except(EXCEPTION_EXECUTE_HANDLER) {}
        NOCTUA_RUNTIME_LOG("shutdown: panic features");
        __try { hacks::disable_panic_features(); } __except(EXCEPTION_EXECUTE_HANDLER) {}
        NOCTUA_RUNTIME_LOG("shutdown: session clear");
        __try { runtime_session::clear(); } __except(EXCEPTION_EXECUTE_HANDLER) {}
        NOCTUA_RUNTIME_LOG("shutdown: game runtime");
        __try { game::shutdown_runtime(); } __except(EXCEPTION_EXECUTE_HANDLER) {}
        NOCTUA_RUNTIME_LOG("shutdown: executor");
        __try { executor::shutdown(); } __except(EXCEPTION_EXECUTE_HANDLER) {}
        NOCTUA_RUNTIME_LOG("shutdown: ws stop");
        __try { ws_server::stop(); } __except(EXCEPTION_EXECUTE_HANDLER) {}

        Sleep(1000);

        NOCTUA_RUNTIME_LOG("shutdown: hooks");
        __try { hook::disable(); } __except(EXCEPTION_EXECUTE_HANDLER) {}

        Sleep(500);

        NOCTUA_RUNTIME_LOG("cleanup: runtime resources");
        __try { cleanup_runtime_resources(); } __except(EXCEPTION_EXECUTE_HANDLER) {}

        NOCTUA_RUNTIME_LOG("shutdown: done");
        runtime_init::cleanup();
    }
}

namespace runtime_init {
    DWORD __stdcall initialize_thread_proc(PVOID) {
        NOCTUA_VMP_BEGIN_ULTRA("initialize_thread_proc");
        crash_logger::install();
        __try {
            initialize();
        } __except(EXCEPTION_EXECUTE_HANDLER) {
            NOCTUA_RUNTIME_LOG("SEH: initialize top-level 0x%08X section=%s", GetExceptionCode(), runtime_debug::last_section);
        }
        NOCTUA_VMP_END();

        HMODULE self = Game.hModule;
        if (self) {
            FreeLibraryAndExitThread(self, 0);
        }
        return 1;
    }

    void cleanup() {
        reset_initialization_state();
    }
}


// ============================================================================
//  NOCTUA LICENSED BUILD WATERMARK  (auto-generated, do not remove)
//  Inert per-file identifier required by the EULA. It uniquely tags this
//  source file so this program is distinguishable from any other build.
//  Contains no functional logic.
//  file: src/runtime/init.cpp
// ============================================================================
#ifndef NOCTUA_LICENSE_MARK_af80e4c32f0d6bcbb5ef4b04b37d75a5
#define NOCTUA_LICENSE_MARK_af80e4c32f0d6bcbb5ef4b04b37d75a5
namespace noctua_license { namespace mark_af80e4c32f0d6bcbb5ef4b04b37d75a5 {
    inline constexpr unsigned long long kMarkId = 0xae9af24c1d28ddbaull;
    inline constexpr char kMarkFile[] = "src/runtime/init.cpp";
    inline constexpr unsigned char kMarkEntropy[] = { 0x25, 0xd1, 0xe7, 0x10, 0xc0, 0x8f, 0xe2, 0x96, 0x52, 0x8b, 0x03, 0xa6, 0xbc, 0x97, 0x17, 0x5b };
    inline constexpr unsigned long  kMarkSalts[]  = { 0xcac6ab02ul, 0x22d8aff3ul, 0x17dd5059ul, 0x526d7ce3ul, 0xddffc997ul, 0x62321771ul, 0x3c3ae87eul, 0x330a18b7ul };
    [[maybe_unused]] inline unsigned long long mark_digest() noexcept {
        unsigned long long h = 1469598103934665603ull ^ kMarkId;
        for (unsigned char b : kMarkEntropy) { h ^= b; h *= 1099511628211ull; }
        for (unsigned long  s : kMarkSalts)  { h ^= s; h *= 1099511628211ull; }
        return h;
    }
} } // namespace noctua_license::mark_af80e4c32f0d6bcbb5ef4b04b37d75a5
#endif // NOCTUA_LICENSE_MARK_af80e4c32f0d6bcbb5ef4b04b37d75a5
