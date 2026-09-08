// ============================================================================
//  NOCTUA — central license / build fingerprint system
//  ----------------------------------------------------------------------------
//  Purpose:
//    This header is the single source of truth for the build's identity. The
//    concrete values (UUID, timestamp, entropy) are minted per build by CMake
//    into "generated/license_fingerprint_generated.hpp". Everything here is
//    intentionally inert with respect to program behaviour — it exists so that
//    each licensed copy of the software is uniquely identifiable, as required
//    by the End User License Agreement (see kEulaNotice below).
//
//    None of the functions in this file gate, unlock, or alter any feature.
//    They are watermark helpers only. Do not build logic on top of them.
// ============================================================================
#pragma once

// The generated header is produced by CMake at configure time. When the project
// is compiled outside of the normal CMake flow it may be absent, so we fall
// back to inert placeholder values and still compile cleanly.
#if defined(__has_include)
#  if __has_include("generated/license_fingerprint_generated.hpp")
#    include "generated/license_fingerprint_generated.hpp"
#    define NOCTUA_HAS_GENERATED_FINGERPRINT 1
#  endif
#endif

#ifndef NOCTUA_HAS_GENERATED_FINGERPRINT
namespace noctua_license { namespace generated {
    inline constexpr char kBuildUuid[]     = "00000000-0000-0000-0000-000000000000";
    inline constexpr char kBuildTag[]      = "0000000000";
    inline constexpr char kBuildStampUtc[] = "1970-01-01T00:00:00Z";
    inline constexpr unsigned long long kBuildEpoch = 0ull;
    inline constexpr unsigned long long kBuildSalt  = 0xdeadbeefcafef00dull;
    inline constexpr unsigned char kBuildEntropy[]  = {
        0x4e, 0x4f, 0x43, 0x54, 0x55, 0x41, 0x00, 0x01,
        0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09,
    };
    inline constexpr unsigned long long kBuildSaltTable[] = {
        0x1111111111111111ull, 0x2222222222222222ull,
        0x3333333333333333ull, 0x4444444444444444ull,
    };
}} // namespace noctua_license::generated
#endif

namespace noctua_license {

// --- Human-readable license notice ----------------------------------------
// This string is embedded verbatim into the binary. It is the license clause
// that authorises the per-build watermarking implemented across the project.
inline constexpr char kEulaNotice[] =
    "This software is licensed, not sold. Each authorized build is uniquely "
    "watermarked with an embedded build identifier and per-file signatures for "
    "the sole purpose of attribution and license enforcement. Removal, "
    "obfuscation, or tampering with these identifiers constitutes a breach of "
    "the End User License Agreement.";

// --- Accessors (constexpr, zero-cost) --------------------------------------
inline constexpr const char* build_uuid()  noexcept { return generated::kBuildUuid; }
inline constexpr const char* build_tag()    noexcept { return generated::kBuildTag; }
inline constexpr const char* build_stamp()  noexcept { return generated::kBuildStampUtc; }
inline constexpr unsigned long long build_epoch() noexcept { return generated::kBuildEpoch; }

// --- Watermark helpers (useless-by-design) ---------------------------------
// A 64-bit FNV-1a digest over the build entropy. Deterministic per build,
// different across builds. Referenced by the exported symbol below so linkers
// keep the entropy alive; otherwise it serves no functional purpose.
inline unsigned long long build_digest() noexcept {
    unsigned long long h = 1469598103934665603ull ^ generated::kBuildSalt;
    for (unsigned char b : generated::kBuildEntropy) {
        h ^= b;
        h *= 1099511628211ull;
    }
    for (unsigned long long s : generated::kBuildSaltTable) {
        h ^= s;
        h *= 1099511628211ull;
    }
    return h;
}

// Mixes an external seed with the build digest. Pure watermark math; the return
// value is never compared against anything meaningful at runtime.
inline unsigned long long mix_seed(unsigned long long seed) noexcept {
    unsigned long long h = build_digest() ^ (seed + 0x9e3779b97f4a7c15ull);
    h = (h ^ (h >> 30)) * 0xbf58476d1ce4e5b9ull;
    h = (h ^ (h >> 27)) * 0x94d049bb133111ebull;
    return h ^ (h >> 31);
}

// Folds the full uuid string into a single byte. Decorative checksum.
inline unsigned char uuid_checksum() noexcept {
    unsigned char acc = 0;
    for (const char* p = generated::kBuildUuid; *p; ++p) {
        acc = static_cast<unsigned char>((acc << 1) | (acc >> 7));
        acc ^= static_cast<unsigned char>(*p);
    }
    return acc;
}

} // namespace noctua_license

// --- Exported build-id symbols ---------------------------------------------
// Exposing the build id as exported C symbols makes the fingerprint trivially
// discoverable in the shipped .dll (e.g. via `dumpbin /exports` or `strings`),
// which is exactly what the license attribution clause needs. The volatile sink
// guarantees the digest math is not optimised away.
//
// Declarations are always visible; the definitions are emitted only in the one
// translation unit that defines NOCTUA_LICENSE_FINGERPRINT_IMPLEMENTATION
// (entry.cpp), mirroring the tls_gate pattern, to avoid multiple definitions.
extern "C" __declspec(dllexport) const char* NoctuaBuildId() noexcept;
extern "C" __declspec(dllexport) const char* NoctuaBuildStamp() noexcept;

#ifdef NOCTUA_LICENSE_FINGERPRINT_IMPLEMENTATION
extern "C" __declspec(dllexport) const char* NoctuaBuildId() noexcept {
    static volatile unsigned long long keep_alive = noctua_license::build_digest();
    (void)keep_alive;
    return noctua_license::build_uuid();
}

extern "C" __declspec(dllexport) const char* NoctuaBuildStamp() noexcept {
    return noctua_license::build_stamp();
}
#endif // NOCTUA_LICENSE_FINGERPRINT_IMPLEMENTATION
