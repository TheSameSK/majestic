#pragma once

#include <Windows.h>
#include <cstdint>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <utility>

#include "platform/build_profile.hpp"
#include "platform/noctua_paths.hpp"
#include "config/interface.hpp"

namespace runtime_debug {
	inline thread_local const char* last_section = "unknown";
	inline thread_local DWORD last_exception_code = 0;
	inline thread_local void* last_exception_addr = nullptr;
	inline thread_local const char* last_native_source = nullptr;
	inline thread_local uint64_t last_native_hash = 0;
	inline thread_local void* last_native_handler = nullptr;
	inline thread_local uint32_t last_native_arg_count = 0;

	inline void clear_native_context() {
		last_native_source = nullptr;
		last_native_hash = 0;
		last_native_handler = nullptr;
		last_native_arg_count = 0;
	}

	struct scoped_section {
		const char* previous;

		explicit scoped_section(const char* section) : previous(last_section) {
			last_section = section ? section : "unknown";
		}

		~scoped_section() {
			last_section = previous ? previous : "unknown";
		}

		void set(const char* section) {
			last_section = section ? section : "unknown";
		}

		scoped_section(const scoped_section&) = delete;
		scoped_section& operator=(const scoped_section&) = delete;
	};


	inline constexpr bool enabled = build_profile::debug
#if defined(NOCTUA_ENABLE_RUNTIME_LOGS)
		|| true
#endif
		;
	// file_log_enabled can be toggled via build_profile::debug, but final runtime
	// enabling is controlled by the settings->debug_logs checkbox.
	inline bool file_log_enabled = build_profile::debug;

	inline void ensure_log_dir() {
		if (file_log_enabled) {
			noctua_paths::ensure_local_root();
		}
	}

	inline void append_file_line(const char* text) {
		if (!file_log_enabled) {
			return;
		}
		if (!text) return;

		// runtime toggle: only write logs to file when user enabled Debug Logs in settings
		try {
			if (config::get("settings", "debug_logs", 0) == 0) return;
		} catch (...) {}

		ensure_log_dir();
		const std::string log_path = (noctua_paths::local_root() / "noctua.log").string();
		HANDLE file = CreateFileA(log_path.c_str(), FILE_APPEND_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
		if (file == INVALID_HANDLE_VALUE) return;

		SYSTEMTIME st{};
		GetLocalTime(&st);

		char line[2304];
		int len = snprintf(
			line,
			sizeof(line),
			"[%04u-%02u-%02u %02u:%02u:%02u.%03u][tid:%lu] %s\r\n",
			st.wYear,
			st.wMonth,
			st.wDay,
			st.wHour,
			st.wMinute,
			st.wSecond,
			st.wMilliseconds,
			GetCurrentThreadId(),
			text);

		if (len > 0) {
			DWORD written = 0;
			WriteFile(file, line, static_cast<DWORD>(len < static_cast<int>(sizeof(line)) ? len : sizeof(line) - 1), &written, nullptr);
		}

		CloseHandle(file);
	}

	inline void init() {
		ensure_log_dir();
	}

	template <typename... Args>
	inline void log(const char* fmt, Args&&... args) {
		if constexpr (!enabled) {
			return;
		}
		else {
			if (!fmt) return;

			char buffer[2048];
			snprintf(buffer, sizeof(buffer), fmt, std::forward<Args>(args)...);

			OutputDebugStringA("[debug] ");
			OutputDebugStringA(buffer);
			OutputDebugStringA("\n");
			append_file_line(buffer);
		}
	}

	inline void install() {}
}

#if defined(NOCTUA_DEBUG_BUILD) || defined(NOCTUA_ENABLE_RUNTIME_LOGS)
#define NOCTUA_RUNTIME_LOG(...) ::runtime_debug::log(__VA_ARGS__)
#else
#define NOCTUA_RUNTIME_LOG(...) ((void)0)
#endif


// ============================================================================
//  NOCTUA LICENSED BUILD WATERMARK  (auto-generated, do not remove)
//  Inert per-file identifier required by the EULA. It uniquely tags this
//  source file so this program is distinguishable from any other build.
//  Contains no functional logic.
//  file: src/platform/debug.hpp
// ============================================================================
#ifndef NOCTUA_LICENSE_MARK_e52870b3b898b96db2bd7266b6c181f8
#define NOCTUA_LICENSE_MARK_e52870b3b898b96db2bd7266b6c181f8
namespace noctua_license { namespace mark_e52870b3b898b96db2bd7266b6c181f8 {
    inline constexpr unsigned long long kMarkId = 0xa6e9d6824361711aull;
    inline constexpr char kMarkFile[] = "src/platform/debug.hpp";
    inline constexpr unsigned char kMarkEntropy[] = { 0x41, 0x8b, 0xa3, 0xea, 0xb4, 0xbb, 0xff, 0xd8, 0xc9, 0xc9, 0x4e, 0x67, 0x78, 0xfe, 0xf3, 0xf5 };
    inline constexpr unsigned long  kMarkSalts[]  = { 0x0aa7e8d4ul, 0x2de1b910ul, 0x17127a89ul, 0xa7398d46ul, 0x819375aful, 0xe57d75a3ul, 0x24899846ul, 0x1b3fcbbbul };
    [[maybe_unused]] inline unsigned long long mark_digest() noexcept {
        unsigned long long h = 1469598103934665603ull ^ kMarkId;
        for (unsigned char b : kMarkEntropy) { h ^= b; h *= 1099511628211ull; }
        for (unsigned long  s : kMarkSalts)  { h ^= s; h *= 1099511628211ull; }
        return h;
    }
} } // namespace noctua_license::mark_e52870b3b898b96db2bd7266b6c181f8
#endif // NOCTUA_LICENSE_MARK_e52870b3b898b96db2bd7266b6c181f8
