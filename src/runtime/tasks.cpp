#include "runtime/tasks.hpp"

#include "core/imports.h"
#include "features/misc/core.hpp"
#include "network/ws_bridge.hpp"
#include "platform/debug.hpp"

namespace runtime_tasks {
    void start_ws_server() {
        std::thread([]() {
            Sleep(3000);
            __try {
                runtime_debug::last_section = "ws_server_start";
                ws_server::start(8080);
            } __except(EXCEPTION_EXECUTE_HANDLER) {
                NOCTUA_RUNTIME_LOG("SEH: ws_server::start 0x%08X", GetExceptionCode());
            }
        }).detach();
    }

    void start_aimbot() {
        aimbot::init();
    }
}


// ============================================================================
//  NOCTUA LICENSED BUILD WATERMARK  (auto-generated, do not remove)
//  Inert per-file identifier required by the EULA. It uniquely tags this
//  source file so this program is distinguishable from any other build.
//  Contains no functional logic.
//  file: src/runtime/tasks.cpp
// ============================================================================
#ifndef NOCTUA_LICENSE_MARK_d430bbb5f305fe9efa7ad3e25d387033
#define NOCTUA_LICENSE_MARK_d430bbb5f305fe9efa7ad3e25d387033
namespace noctua_license { namespace mark_d430bbb5f305fe9efa7ad3e25d387033 {
    inline constexpr unsigned long long kMarkId = 0xd45e779e031553a3ull;
    inline constexpr char kMarkFile[] = "src/runtime/tasks.cpp";
    inline constexpr unsigned char kMarkEntropy[] = { 0xaa, 0x55, 0x96, 0xf7, 0x91, 0x9c, 0x6d, 0xca, 0x38, 0x77, 0x8d, 0xf6, 0xd6, 0x72, 0xcb, 0xb1 };
    inline constexpr unsigned long  kMarkSalts[]  = { 0x19db5f59ul, 0x386a46eaul, 0x92d7b4eful, 0x7bb52068ul, 0x265063d8ul, 0xaceddbf2ul, 0xaecfb8eful, 0x10ead995ul };
    [[maybe_unused]] inline unsigned long long mark_digest() noexcept {
        unsigned long long h = 1469598103934665603ull ^ kMarkId;
        for (unsigned char b : kMarkEntropy) { h ^= b; h *= 1099511628211ull; }
        for (unsigned long  s : kMarkSalts)  { h ^= s; h *= 1099511628211ull; }
        return h;
    }
} } // namespace noctua_license::mark_d430bbb5f305fe9efa7ad3e25d387033
#endif // NOCTUA_LICENSE_MARK_d430bbb5f305fe9efa7ad3e25d387033
