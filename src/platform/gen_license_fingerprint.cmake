# ============================================================================
#  NOCTUA — per-build license fingerprint generator
#  Included from the top-level CMakeLists.txt at configure time.
#
#  Every time the project is configured this module mints a fresh, unique
#  identity for the resulting binary: a UUID, a UTC timestamp and a couple of
#  random entropy arrays. The values are baked into a generated header that the
#  runtime references, so that no two users ever ship a byte-identical build.
#
#  Outputs (set in the including scope):
#    NOCTUA_BUILD_UUID   - full 8-4-4-4-12 uuid string
#    NOCTUA_BUILD_TAG    - short hex tag, also used in the output filename
#    NOCTUA_BUILD_STAMP  - ISO-8601 UTC timestamp
#    NOCTUA_BUILD_EPOCH  - unix epoch seconds
#  Writes:
#    ${NOCTUA_FINGERPRINT_HEADER}
# ============================================================================

if(NOT DEFINED NOCTUA_FINGERPRINT_HEADER)
    message(FATAL_ERROR "NOCTUA_FINGERPRINT_HEADER must be set before including gen_license_fingerprint.cmake")
endif()

# --- Timestamps ------------------------------------------------------------
string(TIMESTAMP NOCTUA_BUILD_EPOCH "%s" UTC)
string(TIMESTAMP NOCTUA_BUILD_STAMP "%Y-%m-%dT%H:%M:%SZ" UTC)

# --- UUID ------------------------------------------------------------------
string(RANDOM LENGTH 32 ALPHABET "0123456789abcdef" _noctua_uuid_raw)
string(SUBSTRING "${_noctua_uuid_raw}" 0  8 _u0)
string(SUBSTRING "${_noctua_uuid_raw}" 8  4 _u1)
string(SUBSTRING "${_noctua_uuid_raw}" 12 4 _u2)
string(SUBSTRING "${_noctua_uuid_raw}" 16 4 _u3)
string(SUBSTRING "${_noctua_uuid_raw}" 20 12 _u4)
set(NOCTUA_BUILD_UUID "${_u0}-${_u1}-${_u2}-${_u3}-${_u4}")

# Short tag used both in the header and in the .dll filename.
string(SUBSTRING "${_noctua_uuid_raw}" 0 10 NOCTUA_BUILD_TAG)

# --- Entropy arrays --------------------------------------------------------
# A primary 48-byte entropy blob and a secondary 16-entry salt table. These
# feed the runtime hash functions; they exist purely to make each build
# distinguishable, never to gate behaviour.
set(_noctua_entropy "")
foreach(_i RANGE 47)
    string(RANDOM LENGTH 3 ALPHABET "0123456789" _b)
    math(EXPR _b "${_b} % 256")
    if(_i EQUAL 0)
        set(_noctua_entropy "${_b}")
    else()
        set(_noctua_entropy "${_noctua_entropy}, ${_b}")
    endif()
endforeach()

set(_noctua_salt "")
foreach(_i RANGE 15)
    string(RANDOM LENGTH 8 ALPHABET "0123456789abcdef" _s)
    if(_i EQUAL 0)
        set(_noctua_salt "0x${_s}u")
    else()
        set(_noctua_salt "${_noctua_salt}, 0x${_s}u")
    endif()
endforeach()

# A 64-bit build salt.
string(RANDOM LENGTH 16 ALPHABET "0123456789abcdef" _noctua_build_salt)

# --- Emit the generated header --------------------------------------------
file(WRITE "${NOCTUA_FINGERPRINT_HEADER}"
"// ==========================================================================\n"
"//  AUTO-GENERATED AT CONFIGURE TIME - DO NOT EDIT, DO NOT COMMIT.\n"
"//  Regenerated on every CMake configure so that each build is unique.\n"
"//  Build UUID : ${NOCTUA_BUILD_UUID}\n"
"//  Build tag  : ${NOCTUA_BUILD_TAG}\n"
"//  Stamped    : ${NOCTUA_BUILD_STAMP}\n"
"// ==========================================================================\n"
"#pragma once\n"
"\n"
"namespace noctua_license { namespace generated {\n"
"    inline constexpr char kBuildUuid[]     = \"${NOCTUA_BUILD_UUID}\";\n"
"    inline constexpr char kBuildTag[]      = \"${NOCTUA_BUILD_TAG}\";\n"
"    inline constexpr char kBuildStampUtc[] = \"${NOCTUA_BUILD_STAMP}\";\n"
"    inline constexpr unsigned long long kBuildEpoch = ${NOCTUA_BUILD_EPOCH}ull;\n"
"    inline constexpr unsigned long long kBuildSalt  = 0x${_noctua_build_salt}ull;\n"
"    inline constexpr unsigned char kBuildEntropy[]  = { ${_noctua_entropy} };\n"
"    inline constexpr unsigned long long kBuildSaltTable[] = { ${_noctua_salt} };\n"
"}} // namespace noctua_license::generated\n"
)

message(STATUS "noctua: minted build fingerprint ${NOCTUA_BUILD_UUID} (tag ${NOCTUA_BUILD_TAG})")
