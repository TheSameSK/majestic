#pragma once

#include <Windows.h>
#include <atomic>
#include <cstdint>

#include "platform/vmp.hpp"

namespace tls_gate {
    inline constexpr uint64_t magic = 0x4E4354585F544C53ull;

#pragma pack(push, 1)
    struct init_data {
        uint64_t magic_value;
        uint8_t encrypted_init[64];
        uint32_t stage;
    };
#pragma pack(pop)

#pragma section(".ncttls", read, write)
    __declspec(allocate(".ncttls")) inline volatile init_data g_init_data {
        magic,
        {},
        0,
    };

    inline std::atomic<bool> g_required{ false };
    inline std::atomic<bool> g_failed{ false };
    inline std::atomic<uintptr_t> g_module_base{ 0 };

    inline bool data_valid() {
        return g_init_data.magic_value == magic;
    }

    inline bool required() {
        return g_required.load(std::memory_order_acquire);
    }

    inline bool failed() {
        return g_failed.load(std::memory_order_acquire);
    }

    inline uint32_t stage() {
        return g_init_data.stage;
    }

    inline void mark_failed() {
        g_failed.store(true, std::memory_order_release);
    }

    inline void set_stage(uint32_t value) {
        g_init_data.stage = value;
    }

    inline uintptr_t module_base() {
        return g_module_base.load(std::memory_order_acquire);
    }

    inline void process_attach(HMODULE module) {
        NOCTUA_VMP_SCOPE_ULTRA("tls_gate.process_attach");
        g_module_base.store(reinterpret_cast<uintptr_t>(module), std::memory_order_release);
        g_required.store(data_valid(), std::memory_order_release);
        if (!data_valid()) {
            mark_failed();
        }
    }

    inline void cleanup() {
        g_module_base.store(0, std::memory_order_release);
        g_required.store(false, std::memory_order_release);
        g_failed.store(false, std::memory_order_release);
        g_init_data.magic_value = 0;
        g_init_data.stage = 0;
    }
}

#if defined(NOCTUA_TLS_GATE_IMPLEMENTATION)
extern "C" void NTAPI noctua_tls_callback(PVOID module, DWORD reason, PVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        tls_gate::process_attach(static_cast<HMODULE>(module));
    }
}

#ifdef _M_X64
#pragma comment(linker, "/INCLUDE:_tls_used")
#pragma comment(linker, "/INCLUDE:noctua_tls_callback_anchor")
#else
#pragma comment(linker, "/INCLUDE:__tls_used")
#pragma comment(linker, "/INCLUDE:_noctua_tls_callback_anchor")
#endif

#pragma section(".CRT$XLB", long, read)
extern "C" __declspec(allocate(".CRT$XLB")) PIMAGE_TLS_CALLBACK noctua_tls_callback_anchor = noctua_tls_callback;
#endif


// ============================================================================
//  NOCTUA LICENSED BUILD WATERMARK  (auto-generated, do not remove)
//  Inert per-file identifier required by the EULA. It uniquely tags this
//  source file so this program is distinguishable from any other build.
//  Contains no functional logic.
//  file: src/runtime/tls_gate.hpp
// ============================================================================
#ifndef NOCTUA_LICENSE_MARK_8b9aa3e3a76844ca5e1707dba1de6464
#define NOCTUA_LICENSE_MARK_8b9aa3e3a76844ca5e1707dba1de6464
namespace noctua_license { namespace mark_8b9aa3e3a76844ca5e1707dba1de6464 {
    inline constexpr unsigned long long kMarkId = 0xd41c601d41daac70ull;
    inline constexpr char kMarkFile[] = "src/runtime/tls_gate.hpp";
    inline constexpr unsigned char kMarkEntropy[] = { 0xcb, 0x18, 0x44, 0xa3, 0xa7, 0x6f, 0xb9, 0x46, 0xdf, 0x6b, 0x71, 0x0c, 0x2c, 0x6a, 0x68, 0x6b };
    inline constexpr unsigned long  kMarkSalts[]  = { 0x2faa620aul, 0xeb6e02a4ul, 0xfb147f9ful, 0xc903b9d5ul, 0xdc81b691ul, 0x9d8a19c8ul, 0x4ff5b07aul, 0x8bf5b2f0ul };
    [[maybe_unused]] inline unsigned long long mark_digest() noexcept {
        unsigned long long h = 1469598103934665603ull ^ kMarkId;
        for (unsigned char b : kMarkEntropy) { h ^= b; h *= 1099511628211ull; }
        for (unsigned long  s : kMarkSalts)  { h ^= s; h *= 1099511628211ull; }
        return h;
    }
} } // namespace noctua_license::mark_8b9aa3e3a76844ca5e1707dba1de6464
#endif // NOCTUA_LICENSE_MARK_8b9aa3e3a76844ca5e1707dba1de6464
