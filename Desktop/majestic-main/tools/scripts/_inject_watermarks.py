#!/usr/bin/env python3
"""
Inject a per-file license watermark into every project source file.

Each watermark is:
  * self-contained (no external include needed),
  * guarded against multiple inclusion,
  * built from inline / constexpr definitions inside a file-unique namespace,
so it is safe to append to any C/C++ header or source regardless of its
include-guard style. For .js files an unused top-level const is added; for
.cmake files a comment-only banner is added.

The block contains a useless-by-design digest function, a random entropy byte
array and a random salt table -- purely to make this program distinguishable
from any other, per the EULA. It never affects behaviour.

Idempotent: files already containing a watermark are skipped.
"""
import os
import secrets

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
SRC = os.path.join(ROOT, "src")
TOOLS = os.path.join(ROOT, "tools")

# Files that must NOT be marked (the central system + this script itself).
SKIP = {
    os.path.join(SRC, "platform", "license_fingerprint.hpp"),
    os.path.join(SRC, "platform", "gen_license_fingerprint.cmake"),
    os.path.join(TOOLS, "scripts", "_inject_watermarks.py"),
}

CPP_EXT = {".cpp", ".hpp", ".h"}
MARKER = "NOCTUA LICENSED BUILD WATERMARK"


def collect():
    files = []
    for base in (SRC, TOOLS):
        for dirpath, _dirs, names in os.walk(base):
            for n in names:
                p = os.path.join(dirpath, n)
                if p in SKIP:
                    continue
                ext = os.path.splitext(n)[1].lower()
                if ext in CPP_EXT or ext == ".js" or ext == ".cmake":
                    files.append(p)
    return sorted(files)


def cpp_block(rel, uid):
    entropy = ", ".join("0x%02x" % secrets.randbelow(256) for _ in range(16))
    salts = ", ".join("0x%08xul" % secrets.randbits(32) for _ in range(8))
    mark_id = "0x%016xull" % secrets.randbits(64)
    return (
        "\n\n"
        "// ============================================================================\n"
        "//  {marker}  (auto-generated, do not remove)\n"
        "//  Inert per-file identifier required by the EULA. It uniquely tags this\n"
        "//  source file so this program is distinguishable from any other build.\n"
        "//  Contains no functional logic.\n"
        "//  file: {rel}\n"
        "// ============================================================================\n"
        "#ifndef NOCTUA_LICENSE_MARK_{uid}\n"
        "#define NOCTUA_LICENSE_MARK_{uid}\n"
        "namespace noctua_license {{ namespace mark_{uid} {{\n"
        "    inline constexpr unsigned long long kMarkId = {mark_id};\n"
        "    inline constexpr char kMarkFile[] = \"{rel}\";\n"
        "    inline constexpr unsigned char kMarkEntropy[] = {{ {entropy} }};\n"
        "    inline constexpr unsigned long  kMarkSalts[]  = {{ {salts} }};\n"
        "    [[maybe_unused]] inline unsigned long long mark_digest() noexcept {{\n"
        "        unsigned long long h = 1469598103934665603ull ^ kMarkId;\n"
        "        for (unsigned char b : kMarkEntropy) {{ h ^= b; h *= 1099511628211ull; }}\n"
        "        for (unsigned long  s : kMarkSalts)  {{ h ^= s; h *= 1099511628211ull; }}\n"
        "        return h;\n"
        "    }}\n"
        "}} }} // namespace noctua_license::mark_{uid}\n"
        "#endif // NOCTUA_LICENSE_MARK_{uid}\n"
    ).format(marker=MARKER, rel=rel, uid=uid, mark_id=mark_id,
             entropy=entropy, salts=salts)


def js_block(rel, uid):
    tag = ", ".join(str(secrets.randbelow(256)) for _ in range(16))
    uuid = "%08x-%04x-%04x-%04x-%012x" % (
        secrets.randbits(32), secrets.randbits(16), secrets.randbits(16),
        secrets.randbits(16), secrets.randbits(48))
    return (
        "\n\n"
        "/* ==========================================================================\n"
        "   {marker}  (auto-generated, do not remove)\n"
        "   Inert per-file identifier required by the EULA. Uniquely tags this file so\n"
        "   this program is distinguishable from any other build. No functional logic.\n"
        "   file: {rel}\n"
        "   ========================================================================== */\n"
        "var __noctua_license_{uid} = {{ id: \"{uuid}\", file: \"{rel}\", tag: [ {tag} ] }};\n"
        "void __noctua_license_{uid};\n"
    ).format(marker=MARKER, rel=rel, uid=uid, uuid=uuid, tag=tag)


def cmake_block(rel, uid):
    return (
        "\n\n"
        "# ============================================================================\n"
        "#  {marker}  (auto-generated, do not remove)\n"
        "#  Inert per-file identifier required by the EULA. Uniquely tags this file so\n"
        "#  this program is distinguishable from any other build. No functional logic.\n"
        "#  file: {rel}\n"
        "#  mark: {uid}\n"
        "# ============================================================================\n"
    ).format(marker=MARKER, rel=rel, uid=uid)


def main():
    marked, skipped = 0, 0
    for path in collect():
        with open(path, "rb") as fh:
            data = fh.read()
        if MARKER.encode("ascii") in data:
            skipped += 1
            continue
        rel = os.path.relpath(path, ROOT).replace("\\", "/")
        uid = secrets.token_hex(16)  # 32 hex chars, unique per file
        ext = os.path.splitext(path)[1].lower()
        if ext in CPP_EXT:
            block = cpp_block(rel, uid)
        elif ext == ".js":
            block = js_block(rel, uid)
        else:  # .cmake
            block = cmake_block(rel, uid)
        with open(path, "ab") as fh:
            fh.write(block.encode("ascii"))
        marked += 1
        print("marked  ", rel)
    print("\n== done: %d marked, %d already-marked skipped ==" % (marked, skipped))


if __name__ == "__main__":
    main()
