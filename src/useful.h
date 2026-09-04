#pragma once
#include <Windows.h>
#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <shlobj.h>
#include <ctime>
#include <dxgi.h>
#include <atomic>
#include <mutex>
#include <cstdio>
#include "xor.h"
#include "platform/build_profile.hpp"
#include "platform/noctua_paths.hpp"
#include "config/interface.hpp"

using namespace std;

namespace logs {
	inline std::mutex g_log_mutex;

	inline void ensure_log_dir() {
		try {
			noctua_paths::ensure_local_root();
		} catch (...) {}
	}

	inline string make_prefix() {
		SYSTEMTIME st{};
		GetLocalTime(&st);
		char buffer[128];
		sprintf_s(
			buffer,
			"[%04u-%02u-%02u %02u:%02u:%02u.%03u][tid:%lu] ",
			st.wYear, st.wMonth, st.wDay,
			st.wHour, st.wMinute, st.wSecond, st.wMilliseconds,
			GetCurrentThreadId()
		);
		return string(buffer);
	}

	inline void directWrite(const string& line) {
		if (line.empty()) return;
		// output to debug output in debug builds
		if constexpr (build_profile::debug) {
			OutputDebugStringA(line.c_str());
		}

		// runtime toggle: only write esp_incoming.log when Debug Logs setting enabled
		try {
			if (config::get("settings", "debug_logs", 0) == 0) return;
		} catch (...) {}
		// also append to a file in the local root for diagnostics
		try {
			ensure_log_dir();
			const auto path = (noctua_paths::local_root() / "esp_incoming.log").string();
			std::lock_guard<std::mutex> lk(g_log_mutex);
			std::ofstream f(path, std::ios::app | std::ios::binary);
			if (f.is_open()) {
				f.write(line.c_str(), static_cast<std::streamsize>(line.size()));
				f.put('\n');
				f.close();
			}
		} catch (...) {}
	}

	inline void add(string msg) {
		directWrite(make_prefix() + msg + "\r\n");
	}

	inline void clear() {
	}

	inline void addLog(string msg) {
		add(std::move(msg));
	}

	inline void addHResult(string label, HRESULT hr) {
		char buffer[64];
		sprintf_s(buffer, "0x%08X", static_cast<unsigned int>(hr));
		add(label + " " + buffer);
	}

	inline void logSystemInfo() {
		add("pid=" + std::to_string(GetCurrentProcessId()));
	}
}


// ============================================================================
//  NOCTUA LICENSED BUILD WATERMARK  (auto-generated, do not remove)
//  Inert per-file identifier required by the EULA. It uniquely tags this
//  source file so this program is distinguishable from any other build.
//  Contains no functional logic.
//  file: src/useful.h
// ============================================================================
#ifndef NOCTUA_LICENSE_MARK_392299ddd61cd04e24d960742cf22fc3
#define NOCTUA_LICENSE_MARK_392299ddd61cd04e24d960742cf22fc3
namespace noctua_license { namespace mark_392299ddd61cd04e24d960742cf22fc3 {
    inline constexpr unsigned long long kMarkId = 0x2953f35725a95bb5ull;
    inline constexpr char kMarkFile[] = "src/useful.h";
    inline constexpr unsigned char kMarkEntropy[] = { 0xc6, 0xbe, 0x7c, 0x3d, 0x48, 0x93, 0xf3, 0x1d, 0x2e, 0x39, 0x32, 0x3c, 0x40, 0xcc, 0x0e, 0x38 };
    inline constexpr unsigned long  kMarkSalts[]  = { 0x3cfe8814ul, 0xaad41137ul, 0xa4daa77dul, 0x47c4c65ful, 0xd923b832ul, 0x82b5fefeul, 0x629afb60ul, 0x486c4dc4ul };
    [[maybe_unused]] inline unsigned long long mark_digest() noexcept {
        unsigned long long h = 1469598103934665603ull ^ kMarkId;
        for (unsigned char b : kMarkEntropy) { h ^= b; h *= 1099511628211ull; }
        for (unsigned long  s : kMarkSalts)  { h ^= s; h *= 1099511628211ull; }
        return h;
    }
} } // namespace noctua_license::mark_392299ddd61cd04e24d960742cf22fc3
#endif // NOCTUA_LICENSE_MARK_392299ddd61cd04e24d960742cf22fc3
