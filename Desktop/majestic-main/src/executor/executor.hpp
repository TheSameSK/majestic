#pragma once

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <Psapi.h>
#include <atomic>
#include <fstream>
#include <string>
#include <vector>
#include <thread>
#include <intrin.h>
#include <sstream>
#if defined(NOCTUA_DEBUG_BUILD)
#include "generated/executor_payload_embedded.hpp"
#endif
#include "platform/debug.hpp"
#include "platform/vmp.hpp"
#pragma comment(lib, "psapi.lib")

namespace cfg_bridge {
    inline void start() {}
    inline void stop() {}
    inline void update(int, int, int, int, int, int) {}
}

namespace executor {
    inline DWORD64 g_dataOffset = 0;
    inline DWORD64 g_sizeOffset = 0;

    inline bool pattern_match(const BYTE* data, const char* pattern, size_t patternLen) {
        for (size_t i = 0; i < patternLen; i++) {
            if (pattern[i] == '?') continue;
            if (data[i] != (BYTE)pattern[i]) return false;
        }
        return true;
    }

    struct AOBPattern {
        std::vector<BYTE> bytes;
        std::vector<bool> mask;
        size_t length;
    };

    inline AOBPattern parse_aob(const char* sig) {
        AOBPattern pat;
        std::string s(sig);
        size_t pos = 0;
        while (pos < s.size()) {
            while (pos < s.size() && s[pos] == ' ') pos++;
            if (pos >= s.size()) break;
            if (s[pos] == '?') {
                pat.bytes.push_back(0);
                pat.mask.push_back(true);
                pos++;
                if (pos < s.size() && s[pos] == '?') pos++;
            } else {
                char hex[3] = { s[pos], s[pos + 1], 0 };
                pat.bytes.push_back((BYTE)strtoul(hex, nullptr, 16));
                pat.mask.push_back(false);
                pos += 2;
            }
        }
        pat.length = pat.bytes.size();
        return pat;
    }

    inline DWORD64 scan_pattern(DWORD64 base, DWORD size, const AOBPattern& pat) {
        if (pat.length == 0 || pat.length > size) return 0;
        const BYTE* mem = (const BYTE*)base;
        for (DWORD i = 0; i < size - (DWORD)pat.length; i++) {
            bool found = true;
            for (size_t j = 0; j < pat.length; j++) {
                if (pat.mask[j]) continue;
                if (mem[i + j] != pat.bytes[j]) { found = false; break; }
            }
            if (found) return base + i;
        }
        return 0;
    }

    inline DWORD64 resolve_rip_relative(DWORD64 instrAddr, int offsetPos) {
        int32_t rip_offset = *(int32_t*)(instrAddr + offsetPos);
        return instrAddr + offsetPos + 4 + rip_offset;
    }

    inline bool find_offsets_by_aob(DWORD64 moduleBase, DWORD moduleSize) {
        NOCTUA_VMP_SCOPE_ULTRA("executor.find_offsets_by_aob");
        const char* sig_str = "48 8B 0D ? ? ? ? 48 8D 50 ? 48 81 FA ? ? ? ? 72 ? 4C 8B 41 ? 48 83 C1 ? 4C 29 C1 48 83 F9 ? 73 ? 48 83 C0 ? 48 89 C2 4C 89 C1 E8 ? ? ? ? 48 C7 05 ? ? ? ? ? ? ? ? 48 C7 05 ? ? ? ? ? ? ? ? C6 05 ? ? ? ? ? 48 83 C4 ? C3 FF 15 ? ? ? ? CC CC CC 56 57 53 48 83 EC ? 48 8B 05";

        AOBPattern pat_str = parse_aob(sig_str);
        DWORD64 addr_str = scan_pattern(moduleBase, moduleSize, pat_str);

        if (addr_str) {
            g_dataOffset = resolve_rip_relative(addr_str, 3) - moduleBase;
            g_sizeOffset = g_dataOffset + 0x10;
            return true;
        }

        g_dataOffset = 0;
        g_sizeOffset = 0;
        return false;
    }

    inline std::atomic<bool> g_initialized{false};
    inline std::atomic<bool> g_patched{false};
    inline std::atomic<bool> g_monitoring{false};
    inline DWORD64 g_majesticBase = 0;
    inline char* g_patchBuffer = nullptr;
    inline size_t g_patchBufferSize = 0;
    inline HANDLE g_monitorThread = nullptr;
    inline std::string g_customScript = "";
    inline std::string g_cloudBridgeScript = "";

    inline void set_cloud_bridge_script(std::string script) {
        g_cloudBridgeScript = std::move(script);
    }

    inline HMODULE find_majestic_module() {
        HMODULE hMods[1024];
        DWORD cbNeeded;
        if (EnumProcessModules(GetCurrentProcess(), hMods, sizeof(hMods), &cbNeeded)) {
            for (unsigned int i = 0; i < cbNeeded / sizeof(HMODULE); i++) {
                char name[MAX_PATH];
                if (GetModuleFileNameA(hMods[i], name, sizeof(name))) {
                    if (strstr(name, "majestic-client")) {
                        return hMods[i];
                    }
                }
            }
        }
        return nullptr;
    }

    inline bool is_readable_range(void* ptr, size_t size) {
        MEMORY_BASIC_INFORMATION mbi;
        if (VirtualQuery(ptr, &mbi, sizeof(mbi)) == 0) return false;
        if (mbi.State != MEM_COMMIT) return false;
        if (mbi.Protect & (PAGE_NOACCESS | PAGE_GUARD)) return false;
        size_t region = (size_t)mbi.RegionSize - ((uintptr_t)ptr - (uintptr_t)mbi.BaseAddress);
        return size <= region;
    }

    inline bool source_starts_with(const char* data, size_t sizeVal, const char* signature) {
        const size_t signatureLen = strlen(signature);
        return sizeVal >= signatureLen &&
            is_readable_range((void*)data, signatureLen) &&
            memcmp(data, signature, signatureLen) == 0;
    }

    inline bool source_has_executor_prefix(const char* data, size_t sizeVal) {
        return source_starts_with(data, sizeVal, "// payload guard") ||
            source_starts_with(data, sizeVal, "const WS_URL = 'ws://127.0.0.1:8080';") ||
            source_starts_with(data, sizeVal, "const WS_URL='ws://127.0.0.1:8080'");
    }

    inline bool ensure_patch_buffer(size_t required) {
        if (g_patchBuffer && g_patchBufferSize >= required) return true;
        if (g_patchBuffer) {
            VirtualFree(g_patchBuffer, 0, MEM_RELEASE);
            g_patchBuffer = nullptr;
            g_patchBufferSize = 0;
        }
        g_patchBuffer = (char*)VirtualAlloc(nullptr, required, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        if (!g_patchBuffer) return false;
        g_patchBufferSize = required;
        return true;
    }

    inline std::string build_executor_code() {
        NOCTUA_VMP_SCOPE_ULTRA("executor.build_executor_code");
#if defined(NOCTUA_DEBUG_BUILD)
        if (!g_cloudBridgeScript.empty()) return g_cloudBridgeScript;
        return executor_payload::source();
#else
        return g_cloudBridgeScript;
#endif
    }

    inline bool apply_script_patch(DWORD64* pDataPtr, size_t* pSize, char* data, size_t sizeVal, const std::string& executorCode) {
        NOCTUA_VMP_SCOPE_ULTRA("executor.apply_script_patch");
        const size_t executorLen = executorCode.length();
        const size_t newSize = executorLen + sizeVal;
        const size_t required = newSize + 1;

        if (!ensure_patch_buffer(required)) {
            NOCTUA_RUNTIME_LOG("executor monitor: patch buffer allocation failed size=%zu", required);
            return false;
        }

        memcpy(g_patchBuffer, executorCode.c_str(), executorLen);
        memcpy(g_patchBuffer + executorLen, data, sizeVal);
        g_patchBuffer[newSize] = 0;

        *pDataPtr = (DWORD64)g_patchBuffer;
        _mm_mfence();
        *pSize = newSize;

        g_patched = true;
        NOCTUA_RUNTIME_LOG("executor monitor: patched script old_size=%zu new_size=%zu", sizeVal, newSize);

        if (!g_cloudBridgeScript.empty()) {
            SecureZeroMemory(g_cloudBridgeScript.data(), g_cloudBridgeScript.size());
            g_cloudBridgeScript.clear();
            g_cloudBridgeScript.shrink_to_fit();
        }
        return true;
    }

    inline DWORD WINAPI monitor_thread_proc(LPVOID param) {
        NOCTUA_VMP_SCOPE_VIRTUALIZATION("executor.monitor_thread_proc");
        runtime_debug::last_section = "executor_monitor_start";
        HMODULE hMajestic = nullptr;
        int waitCount = 0;

        while (!hMajestic && g_monitoring) {
            hMajestic = find_majestic_module();
            if (!hMajestic) {
                Sleep(100);
                waitCount++;
                if (waitCount > 600) {
                    return 1;
                }
            }
        }

        if (!hMajestic) {
            return 1;
        }

        runtime_debug::last_section = "executor_monitor_module";
        MODULEINFO mi;
        GetModuleInformation(GetCurrentProcess(), hMajestic, &mi, sizeof(mi));
        g_majesticBase = (DWORD64)mi.lpBaseOfDll;

        runtime_debug::last_section = "executor_monitor_aob";
        if (!find_offsets_by_aob(g_majesticBase, mi.SizeOfImage)) {
            NOCTUA_RUNTIME_LOG("executor monitor: script buffer AOB not found");
            return 1;
        }
        NOCTUA_RUNTIME_LOG("executor monitor: offsets data=0x%llX size=0x%llX", static_cast<unsigned long long>(g_dataOffset), static_cast<unsigned long long>(g_sizeOffset));

        if (g_dataOffset + 8 > mi.SizeOfImage || g_sizeOffset + 8 > mi.SizeOfImage) {
            NOCTUA_RUNTIME_LOG("executor monitor: offsets out of module size=0x%lX", mi.SizeOfImage);
            return 1;
        }

        runtime_debug::last_section = "executor_monitor_script";
        std::string executorCode = build_executor_code();

        size_t executorLen = executorCode.length();
        if (executorLen == 0) {
            NOCTUA_RUNTIME_LOG("executor monitor: empty bridge script");
            return 1;
        }

        runtime_debug::last_section = "executor_monitor_slots";
        DWORD64* pDataPtr = (DWORD64*)(g_majesticBase + g_dataOffset);
        size_t* pSize = (size_t*)(g_majesticBase + g_sizeOffset);

        if (!is_readable_range(pDataPtr, 8) || !is_readable_range(pSize, 8)) {
            NOCTUA_RUNTIME_LOG("executor monitor: pointer slots are not readable");
            return 1;
        }

        DWORD64 lastDataPtr = 0;
        size_t lastSizeVal = 0;

        for (int attempt = 0; attempt < 30000 && g_monitoring && !g_patched; attempt++) {
            DWORD64 dataPtr = *pDataPtr;
            size_t sizeVal = *pSize;

            if (dataPtr != lastDataPtr || sizeVal != lastSizeVal || attempt % 1500 == 0) {
                lastDataPtr = dataPtr;
                lastSizeVal = sizeVal;
            }

            if (dataPtr > 0x10000 && dataPtr < 0x00007FFFFFFFFFFF && sizeVal > 100000 && sizeVal < 100000000) {
                char* data = (char*)dataPtr;
                if (is_readable_range(data, 64) && is_readable_range(data, sizeVal)) {
                    runtime_debug::last_section = "executor_monitor_patch";

                    char preview[65] = {0};
                    memcpy(preview, data, 64);
                    for (int i = 0; i < 64; i++) {
                        if (preview[i] < 32 || preview[i] > 126) preview[i] = '.';
                    }

                    if (source_has_executor_prefix(data, sizeVal)) {
                        g_patched = true;
                        NOCTUA_RUNTIME_LOG("executor monitor: script already patched");
                        if (!g_cloudBridgeScript.empty()) {
                            SecureZeroMemory(g_cloudBridgeScript.data(), g_cloudBridgeScript.size());
                            g_cloudBridgeScript.clear();
                            g_cloudBridgeScript.shrink_to_fit();
                        }
                        return 0;
                    }

                    if (!apply_script_patch(pDataPtr, pSize, data, sizeVal, executorCode)) {
                        return 1;
                    }

                    return 0;
                }
            }

            Sleep(20);
        }

        return g_patched ? 0 : 1;
    }

    inline void start_early_monitoring() {

        if (g_monitoring.exchange(true)) {
            return;
        }

        g_monitorThread = CreateThread(nullptr, 0, monitor_thread_proc, nullptr, 0, nullptr);
        if (!g_monitorThread) {
            g_monitoring = false;
        }
    }

    inline void initialize() {

        if (g_initialized.exchange(true)) {
            return;
        }

    }

    inline bool is_patched() { return g_patched.load(); }
    inline bool is_monitoring() { return g_monitoring.load(); }
    inline bool is_initialized() { return g_initialized.load(); }
    inline bool is_injected() { return g_patched.load(); }

    inline std::string get_status() {
        if (g_patched) return "ready";
        if (g_monitoring) return "Monitoring...";
        if (g_initialized) return "Initialized";
        return "Not started";
    }

    inline int get_trigger_count() { return g_patched ? 1 : 0; }
    inline int get_injection_count() { return g_patched ? 1 : 0; }

    inline bool inject() {
        if (!g_monitoring) start_early_monitoring();
        return g_patched.load();
    }

    inline DWORD64 get_module_base() { return g_majesticBase; }

inline void shutdown() {
        g_monitoring = false;
        g_initialized = false;
        g_patched = false;

        if (g_monitorThread) {
            WaitForSingleObject(g_monitorThread, 3000);
            CloseHandle(g_monitorThread);
            g_monitorThread = nullptr;
}
}

}


// ============================================================================
//  NOCTUA LICENSED BUILD WATERMARK  (auto-generated, do not remove)
//  Inert per-file identifier required by the EULA. It uniquely tags this
//  source file so this program is distinguishable from any other build.
//  Contains no functional logic.
//  file: src/executor/executor.hpp
// ============================================================================
#ifndef NOCTUA_LICENSE_MARK_792e259c2a947e6e76fdf8a8156ba862
#define NOCTUA_LICENSE_MARK_792e259c2a947e6e76fdf8a8156ba862
namespace noctua_license { namespace mark_792e259c2a947e6e76fdf8a8156ba862 {
    inline constexpr unsigned long long kMarkId = 0x77e2cccdffe070b8ull;
    inline constexpr char kMarkFile[] = "src/executor/executor.hpp";
    inline constexpr unsigned char kMarkEntropy[] = { 0xec, 0x42, 0x3d, 0x1e, 0xef, 0xdf, 0xa0, 0x3b, 0xde, 0x73, 0xb0, 0x2b, 0x27, 0xcd, 0xae, 0x0e };
    inline constexpr unsigned long  kMarkSalts[]  = { 0xe6f0b1daul, 0xff36d6adul, 0x13f0e140ul, 0xbdb0fcb4ul, 0x1f4a1815ul, 0x4654a0ceul, 0xe3c4de21ul, 0x2131cca8ul };
    [[maybe_unused]] inline unsigned long long mark_digest() noexcept {
        unsigned long long h = 1469598103934665603ull ^ kMarkId;
        for (unsigned char b : kMarkEntropy) { h ^= b; h *= 1099511628211ull; }
        for (unsigned long  s : kMarkSalts)  { h ^= s; h *= 1099511628211ull; }
        return h;
    }
} } // namespace noctua_license::mark_792e259c2a947e6e76fdf8a8156ba862
#endif // NOCTUA_LICENSE_MARK_792e259c2a947e6e76fdf8a8156ba862
