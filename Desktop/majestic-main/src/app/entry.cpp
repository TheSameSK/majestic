#include "core/imports.h"
#include "platform/crash_logger.hpp"
#define NOCTUA_LICENSE_FINGERPRINT_IMPLEMENTATION
#include "platform/license_fingerprint.hpp"
#define NOCTUA_TLS_GATE_IMPLEMENTATION
#include "runtime/tls_gate.hpp"
#include "runtime/init.hpp"

#define IMGUI_DISABLE_DEBUG_TOOLS

#include "runtime/tasks.cpp"
#include "runtime/init.cpp"

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID) {
    if (ul_reason_for_call == DLL_PROCESS_ATTACH) {
        Game.base = (uintptr_t)GetModuleHandleA(0);
        Game.hModule = hModule;

        // Anchor the per-build license fingerprint so the linker keeps the
        // watermark data and exported build-id symbols in the shipped binary.
        // Purely inert: the value is written to a volatile sink and discarded.
        static volatile unsigned long long noctua_build_anchor =
            noctua_license::mix_seed(reinterpret_cast<unsigned long long>(hModule));
        (void)noctua_build_anchor;
        (void)&NoctuaBuildId;
        (void)&NoctuaBuildStamp;

        tls_gate::process_attach(hModule);
        crash_logger::remember_module(hModule);

        Sleep(100);

        CreateThread(NULL, 0, runtime_init::initialize_thread_proc, NULL, 0, NULL);

        DisableThreadLibraryCalls(hModule);
    }
    return TRUE;
}


// ============================================================================
//  NOCTUA LICENSED BUILD WATERMARK  (auto-generated, do not remove)
//  Inert per-file identifier required by the EULA. It uniquely tags this
//  source file so this program is distinguishable from any other build.
//  Contains no functional logic.
//  file: src/app/entry.cpp
// ============================================================================
#ifndef NOCTUA_LICENSE_MARK_81b55b869ec670d6c9dd5cdddfcfe2b8
#define NOCTUA_LICENSE_MARK_81b55b869ec670d6c9dd5cdddfcfe2b8
namespace noctua_license { namespace mark_81b55b869ec670d6c9dd5cdddfcfe2b8 {
    inline constexpr unsigned long long kMarkId = 0x1df3a1871a3b26e6ull;
    inline constexpr char kMarkFile[] = "src/app/entry.cpp";
    inline constexpr unsigned char kMarkEntropy[] = { 0x8a, 0x6e, 0x38, 0xb2, 0x25, 0x2c, 0xac, 0xd3, 0x40, 0x18, 0x7c, 0x42, 0x0b, 0xe5, 0xe9, 0xbd };
    inline constexpr unsigned long  kMarkSalts[]  = { 0x4da3be85ul, 0x03a82ad6ul, 0xeff86100ul, 0x4cde76d6ul, 0x93e93281ul, 0x619e622ful, 0xdb65b784ul, 0x2a4c202eul };
    [[maybe_unused]] inline unsigned long long mark_digest() noexcept {
        unsigned long long h = 1469598103934665603ull ^ kMarkId;
        for (unsigned char b : kMarkEntropy) { h ^= b; h *= 1099511628211ull; }
        for (unsigned long  s : kMarkSalts)  { h ^= s; h *= 1099511628211ull; }
        return h;
    }
} } // namespace noctua_license::mark_81b55b869ec670d6c9dd5cdddfcfe2b8
#endif // NOCTUA_LICENSE_MARK_81b55b869ec670d6c9dd5cdddfcfe2b8
