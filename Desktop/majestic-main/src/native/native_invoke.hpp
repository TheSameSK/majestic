#pragma once

#include <cstdint>
#include <cstring>
#include <type_traits>
#include <Windows.h>
#include <Psapi.h>
#include "platform/debug.hpp"

#pragma comment(lib, "psapi.lib")

namespace native_invoke {

    struct scrVector { float x; int _pad0; float y; int _pad1; float z; int _pad2; };

    struct scrNativeCallContext {
        void* m_return;
        uint32_t m_arg_count;
        void* m_args;
        uint32_t m_data_count;
        alignas(16) uint8_t m_vector_space[192];
    };

    using scrNativeHash = uint64_t;
    using scrNativeHandler = void(*)(scrNativeCallContext*);
    using GetNativeHandlerFn = scrNativeHandler(*)(void* table, scrNativeHash hash);

    inline void* g_native_table = nullptr;
    inline GetNativeHandlerFn g_get_handler = nullptr;
    inline void(*g_fix_vectors)(scrNativeCallContext*) = nullptr;
    inline bool g_initialized = false;

    static uintptr_t find_pattern(uintptr_t base, size_t size, const char* pattern) {
        auto pat2bytes = [](const char* pat, uint8_t* bytes, bool* mask, int& len) {
            len = 0;
            for (const char* p = pat; *p; p++) {
                if (*p == ' ') continue;
                if (*p == '?') {
                    bytes[len] = 0; mask[len] = true; len++;
                    if (p[1] == '?') p++;
                } else {
                    char hex[3] = { p[0], p[1], 0 };
                    bytes[len] = (uint8_t)strtoul(hex, nullptr, 16);
                    mask[len] = false; len++;
                    p++;
                }
            }
        };

        uint8_t bytes[256]; bool mask[256]; int len = 0;
        pat2bytes(pattern, bytes, mask, len);

        const uint8_t* mem = (const uint8_t*)base;
        for (size_t i = 0; i + len <= size; i++) {
            bool found = true;
            for (int j = 0; j < len; j++) {
                if (!mask[j] && mem[i + j] != bytes[j]) { found = false; break; }
            }
            if (found) return base + i;
        }
        return 0;
    }

    static uintptr_t rip_rel(uintptr_t addr, int offset) {
        int32_t disp = *(int32_t*)(addr + offset);
        return addr + offset + 4 + disp;
    }

    struct NativeContext : scrNativeCallContext {
        uint64_t m_storage[32];

        NativeContext() {
            memset(this, 0, sizeof(*this));
            m_return = &m_storage[0];
            m_args = &m_storage[0];
        }

        template<typename T>
        void push(T value) {
            static_assert(sizeof(T) <= 8);
            *(T*)((uint64_t*)m_args + m_arg_count) = value;
            m_arg_count++;
        }

        template<typename T>
        T get_result() {
            return *(T*)m_storage;
        }
    };

    template<typename Ret>
    static Ret default_result() {
        if constexpr (!std::is_void_v<Ret>) {
            return Ret{};
        }
    }

    static bool init() {
        if (g_initialized) return true;

        HMODULE gta = GetModuleHandleA(NULL);
        if (!gta) return false;

        MODULEINFO mi;
        GetModuleInformation(GetCurrentProcess(), gta, &mi, sizeof(mi));
        uintptr_t base = (uintptr_t)mi.lpBaseOfDll;
        size_t size = mi.SizeOfImage;

        uintptr_t addr = find_pattern(base, size, "48 8D 0D ? ? ? ? 48 8B 14 FA E8 ? ? ? ? 48 85 C0 75 0A");
        if (!addr) return false;

        g_native_table = (void*)rip_rel(addr, 3);
        g_get_handler = (GetNativeHandlerFn)rip_rel(addr, 12);

        uintptr_t fv = find_pattern(base, size, "83 79 18 00 48 8B D1 74 4A FF 4A 18 48 63 4A 18 48 8D 41 04 48 8B 4C CA");
        if (fv) g_fix_vectors = (decltype(g_fix_vectors))fv;

        g_initialized = (g_native_table != nullptr && g_get_handler != nullptr);
        return g_initialized;
    }

    template<typename Ret, typename... Args>
    static Ret invoke(scrNativeHash hash, Args... args) {
        if ((!g_initialized && !init()) || !g_native_table || !g_get_handler) {
            return default_result<Ret>();
        }

        NativeContext ctx;
        (ctx.push(args), ...);
        runtime_debug::last_native_source = "native_invoke";
        runtime_debug::last_native_hash = hash;
        runtime_debug::last_native_handler = nullptr;
        runtime_debug::last_native_arg_count = static_cast<uint32_t>(sizeof...(Args));

        scrNativeHandler handler = nullptr;
        __try {
            handler = g_get_handler(g_native_table, hash);
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            NOCTUA_RUNTIME_LOG("SEH: native_invoke lookup 0x%08X hash=0x%llX", GetExceptionCode(), static_cast<unsigned long long>(hash));
            return default_result<Ret>();
        }

        runtime_debug::last_native_handler = reinterpret_cast<void*>(handler);
        if (handler) {
            __try {
                handler(&ctx);
                if (g_fix_vectors) g_fix_vectors(&ctx);
            }
            __except (EXCEPTION_EXECUTE_HANDLER) {
                NOCTUA_RUNTIME_LOG("SEH: native_invoke call 0x%08X hash=0x%llX", GetExceptionCode(), static_cast<unsigned long long>(hash));
                return default_result<Ret>();
            }
        }

        if constexpr (!std::is_void_v<Ret>) {
            return ctx.get_result<Ret>();
        }
    }

    namespace HUD {
        static void SET_TEXT_FONT(int font) { invoke<void>(0x66E0276CC5F6B9DA, font); }
        static void SET_TEXT_SCALE(float x, float y) { invoke<void>(0x07C837F9A01C34C9, x, y); }
        static void SET_TEXT_COLOUR(int r, int g, int b, int a) { invoke<void>(0xBE6B23FFA53FB442, r, g, b, a); }
        static void SET_TEXT_OUTLINE() { invoke<void>(0x2513DFB0FB8400FE); }
        static void SET_TEXT_CENTRE(bool toggle) { invoke<void>(0xC02F4DBFB51D988B, toggle); }
        static void BEGIN_TEXT_COMMAND_DISPLAY_TEXT(const char* text) { invoke<void>(0x25FBB336DF1804CB, text); }
        static void ADD_TEXT_COMPONENT_SUBSTRING_PLAYER_NAME(const char* text) { invoke<void>(0x6C188BE134E074AA, text); }
        static void END_TEXT_COMMAND_DISPLAY_TEXT(float x, float y, int p2) { invoke<void>(0xCD015E5BB0D96A57, x, y, p2); }
    }

    namespace GRAPHICS {
        static bool GET_SCREEN_COORD_FROM_WORLD_COORD(float wX, float wY, float wZ, float* sX, float* sY) {
            return invoke<bool>(0x34E82F05DF2974F5, wX, wY, wZ, sX, sY);
        }
        static void DRAW_LINE(float x1, float y1, float z1, float x2, float y2, float z2, int r, int g, int b, int a) {
            invoke<void>(0x6B7256074AE34680, x1, y1, z1, x2, y2, z2, r, g, b, a);
        }
    }

    namespace PED {
        static scrVector GET_PED_BONE_COORDS(int ped, int bone, float offX, float offY, float offZ) {
            return invoke<scrVector>(0x17C07FC640E86B4E, ped, bone, offX, offY, offZ);
        }
    }

    static void draw_text(const char* text, float x, float y, float scale, int r, int g, int b, int a) {
        HUD::SET_TEXT_FONT(4);
        HUD::SET_TEXT_SCALE(scale, scale);
        HUD::SET_TEXT_COLOUR(r, g, b, a);
        HUD::SET_TEXT_OUTLINE();
        HUD::SET_TEXT_CENTRE(true);
        HUD::BEGIN_TEXT_COMMAND_DISPLAY_TEXT("STRING");
        HUD::ADD_TEXT_COMPONENT_SUBSTRING_PLAYER_NAME(text);
        HUD::END_TEXT_COMMAND_DISPLAY_TEXT(x, y, 0);
    }

    static bool world_to_screen(float wx, float wy, float wz, float& sx, float& sy) {
        return GRAPHICS::GET_SCREEN_COORD_FROM_WORLD_COORD(wx, wy, wz, &sx, &sy);
    }
}


// ============================================================================
//  NOCTUA LICENSED BUILD WATERMARK  (auto-generated, do not remove)
//  Inert per-file identifier required by the EULA. It uniquely tags this
//  source file so this program is distinguishable from any other build.
//  Contains no functional logic.
//  file: src/native/native_invoke.hpp
// ============================================================================
#ifndef NOCTUA_LICENSE_MARK_2a9048e9f2e9f5b46ed42ebafbf5f8e3
#define NOCTUA_LICENSE_MARK_2a9048e9f2e9f5b46ed42ebafbf5f8e3
namespace noctua_license { namespace mark_2a9048e9f2e9f5b46ed42ebafbf5f8e3 {
    inline constexpr unsigned long long kMarkId = 0xc9b19f6eed49f646ull;
    inline constexpr char kMarkFile[] = "src/native/native_invoke.hpp";
    inline constexpr unsigned char kMarkEntropy[] = { 0x7e, 0xf3, 0x8c, 0x63, 0x98, 0x32, 0xfc, 0x0b, 0x0f, 0x0c, 0x79, 0xec, 0x18, 0x9a, 0x04, 0xd4 };
    inline constexpr unsigned long  kMarkSalts[]  = { 0x05b31a1cul, 0x718be59cul, 0xc295840eul, 0xf9a579b9ul, 0x3d42f0eful, 0x7d9d691bul, 0xcbdf7d7bul, 0xc09d2614ul };
    [[maybe_unused]] inline unsigned long long mark_digest() noexcept {
        unsigned long long h = 1469598103934665603ull ^ kMarkId;
        for (unsigned char b : kMarkEntropy) { h ^= b; h *= 1099511628211ull; }
        for (unsigned long  s : kMarkSalts)  { h ^= s; h *= 1099511628211ull; }
        return h;
    }
} } // namespace noctua_license::mark_2a9048e9f2e9f5b46ed42ebafbf5f8e3
#endif // NOCTUA_LICENSE_MARK_2a9048e9f2e9f5b46ed42ebafbf5f8e3
