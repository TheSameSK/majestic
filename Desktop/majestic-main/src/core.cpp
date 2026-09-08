namespace noctua {
void bootstrap() {}
}


// ============================================================================
//  NOCTUA LICENSED BUILD WATERMARK  (auto-generated, do not remove)
//  Inert per-file identifier required by the EULA. It uniquely tags this
//  source file so this program is distinguishable from any other build.
//  Contains no functional logic.
//  file: src/core.cpp
// ============================================================================
#ifndef NOCTUA_LICENSE_MARK_4fd6a17c540c58e7bc84ecd156ca86f2
#define NOCTUA_LICENSE_MARK_4fd6a17c540c58e7bc84ecd156ca86f2
namespace noctua_license { namespace mark_4fd6a17c540c58e7bc84ecd156ca86f2 {
    inline constexpr unsigned long long kMarkId = 0x2ba74292f707e347ull;
    inline constexpr char kMarkFile[] = "src/core.cpp";
    inline constexpr unsigned char kMarkEntropy[] = { 0xa9, 0xc1, 0xa2, 0x6f, 0xfb, 0xbd, 0x61, 0x95, 0x08, 0xb6, 0x13, 0xc4, 0xb4, 0xf8, 0x14, 0x1c };
    inline constexpr unsigned long  kMarkSalts[]  = { 0xf621e1d7ul, 0xed69e1a2ul, 0xd0cccc5aul, 0xb59563b2ul, 0xc4528183ul, 0x047713ccul, 0xb742e3ecul, 0x8332dff9ul };
    [[maybe_unused]] inline unsigned long long mark_digest() noexcept {
        unsigned long long h = 1469598103934665603ull ^ kMarkId;
        for (unsigned char b : kMarkEntropy) { h ^= b; h *= 1099511628211ull; }
        for (unsigned long  s : kMarkSalts)  { h ^= s; h *= 1099511628211ull; }
        return h;
    }
} } // namespace noctua_license::mark_4fd6a17c540c58e7bc84ecd156ca86f2
#endif // NOCTUA_LICENSE_MARK_4fd6a17c540c58e7bc84ecd156ca86f2
