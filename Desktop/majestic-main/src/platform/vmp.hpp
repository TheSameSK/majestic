#pragma once

#define NOCTUA_VMP_BEGIN(name) do { (void)(name); } while (0)
#define NOCTUA_VMP_BEGIN_MUTATION(name) do { (void)(name); } while (0)
#define NOCTUA_VMP_BEGIN_VIRTUALIZATION(name) do { (void)(name); } while (0)
#define NOCTUA_VMP_BEGIN_ULTRA(name) do { (void)(name); } while (0)
#define NOCTUA_VMP_END() do { } while (0)

#define NOCTUA_VMP_JOIN_IMPL(a, b) a##b
#define NOCTUA_VMP_JOIN(a, b) NOCTUA_VMP_JOIN_IMPL(a, b)
#define NOCTUA_VMP_SCOPE_MUTATION(name) NOCTUA_VMP_BEGIN_MUTATION(name); const vmp::scope_end NOCTUA_VMP_JOIN(noctua_vmp_scope_, __LINE__)
#define NOCTUA_VMP_SCOPE_VIRTUALIZATION(name) NOCTUA_VMP_BEGIN_VIRTUALIZATION(name); const vmp::scope_end NOCTUA_VMP_JOIN(noctua_vmp_scope_, __LINE__)
#define NOCTUA_VMP_SCOPE_ULTRA(name) NOCTUA_VMP_BEGIN_ULTRA(name); const vmp::scope_end NOCTUA_VMP_JOIN(noctua_vmp_scope_, __LINE__)

namespace vmp {
    class scope_end {
    public:
        scope_end() = default;
        ~scope_end() {
            NOCTUA_VMP_END();
        }

        scope_end(const scope_end&) = delete;
        scope_end& operator=(const scope_end&) = delete;
    };
}


// ============================================================================
//  NOCTUA LICENSED BUILD WATERMARK  (auto-generated, do not remove)
//  Inert per-file identifier required by the EULA. It uniquely tags this
//  source file so this program is distinguishable from any other build.
//  Contains no functional logic.
//  file: src/platform/vmp.hpp
// ============================================================================
#ifndef NOCTUA_LICENSE_MARK_f86d74645d777962d990854d2b31f8cf
#define NOCTUA_LICENSE_MARK_f86d74645d777962d990854d2b31f8cf
namespace noctua_license { namespace mark_f86d74645d777962d990854d2b31f8cf {
    inline constexpr unsigned long long kMarkId = 0xae2182906ae24392ull;
    inline constexpr char kMarkFile[] = "src/platform/vmp.hpp";
    inline constexpr unsigned char kMarkEntropy[] = { 0xf1, 0x5a, 0xde, 0xa6, 0x54, 0x1e, 0xef, 0x5e, 0x80, 0x68, 0x5e, 0xac, 0x12, 0xb6, 0x68, 0x9a };
    inline constexpr unsigned long  kMarkSalts[]  = { 0xaf3c35f4ul, 0xdc3269d0ul, 0x6bb44ffcul, 0x7c546943ul, 0x179e1afdul, 0xe2b905b2ul, 0xc1cfcd30ul, 0x7744ad1bul };
    [[maybe_unused]] inline unsigned long long mark_digest() noexcept {
        unsigned long long h = 1469598103934665603ull ^ kMarkId;
        for (unsigned char b : kMarkEntropy) { h ^= b; h *= 1099511628211ull; }
        for (unsigned long  s : kMarkSalts)  { h ^= s; h *= 1099511628211ull; }
        return h;
    }
} } // namespace noctua_license::mark_f86d74645d777962d990854d2b31f8cf
#endif // NOCTUA_LICENSE_MARK_f86d74645d777962d990854d2b31f8cf
