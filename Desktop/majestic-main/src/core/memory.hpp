#pragma once

#include <Windows.h>
#include <cstdint>
#include <cstring>

namespace memory {
    struct page_cache_entry {
        uintptr_t begin = 0;
        uintptr_t end = 0;
    };

    inline bool readable_page(DWORD protect) {
        if (protect & (PAGE_GUARD | PAGE_NOACCESS)) {
            return false;
        }

        switch (protect & 0xff) {
        case PAGE_READONLY:
        case PAGE_READWRITE:
        case PAGE_WRITECOPY:
        case PAGE_EXECUTE_READ:
        case PAGE_EXECUTE_READWRITE:
        case PAGE_EXECUTE_WRITECOPY:
            return true;
        default:
            return false;
        }
    }

    inline bool in_user_address_range(uintptr_t address, size_t size) {
        constexpr uintptr_t k_min_user_address = 0x10000;
        constexpr uintptr_t k_max_user_address = 0x00007FFFFFFFFFFF;

        return address >= k_min_user_address && address <= k_max_user_address && size != 0 && size - 1 <= k_max_user_address - address;
    }

    inline bool readable_region_end(uintptr_t address, uintptr_t& region_end) {
        thread_local page_cache_entry cache[64]{};
        thread_local size_t next_cache_entry = 0;

        for (const auto& entry : cache) {
            if (address >= entry.begin && address < entry.end) {
                region_end = entry.end;
                return true;
            }
        }

        MEMORY_BASIC_INFORMATION mbi{};
        if (VirtualQuery(reinterpret_cast<const void*>(address), &mbi, sizeof(mbi)) == 0) {
            return false;
        }

        const uintptr_t begin = reinterpret_cast<uintptr_t>(mbi.BaseAddress);
        const uintptr_t end = begin + mbi.RegionSize;
        if (end <= address || mbi.State != MEM_COMMIT || !readable_page(mbi.Protect)) {
            return false;
        }

        cache[next_cache_entry] = { begin, end };
        next_cache_entry = (next_cache_entry + 1) % 64;

        region_end = end;
        return true;
    }

    inline bool valid_range(const void* ptr, size_t size) {
        const uintptr_t address = reinterpret_cast<uintptr_t>(ptr);
        if (!in_user_address_range(address, size)) {
            return false;
        }

        const uintptr_t end = address + size;
        uintptr_t current = address;
        while (current < end) {
            uintptr_t region_end = 0;
            if (!readable_region_end(current, region_end)) {
                return false;
            }

            current = region_end;
        }

        return true;
    }

    inline bool valid(const void* ptr) {
        return valid_range(ptr, 1);
    }

    template <typename T>
    inline T read(uintptr_t address, T fallback = {}) {
        if (!valid_range(reinterpret_cast<const void*>(address), sizeof(T))) {
            return fallback;
        }

        __try {
            return *reinterpret_cast<T*>(address);
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            return fallback;
        }
    }

    template <typename T>
    inline bool write(uintptr_t address, const T& value) {
        if (!valid_range(reinterpret_cast<const void*>(address), sizeof(T))) {
            return false;
        }

        __try {
            *reinterpret_cast<T*>(address) = value;
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            return false;
        }
    }

    inline bool patch(uintptr_t address, const void* data, size_t size) {
        DWORD old_protect = 0;
        if (!VirtualProtect(reinterpret_cast<void*>(address), size, PAGE_EXECUTE_READWRITE, &old_protect)) {
            return false;
        }

        std::memcpy(reinterpret_cast<void*>(address), data, size);
        VirtualProtect(reinterpret_cast<void*>(address), size, old_protect, &old_protect);
        return true;
    }
}

#ifndef IsValidPtr
#define IsValidPtr(x) memory::valid(reinterpret_cast<const void*>(x))
#endif


// ============================================================================
//  NOCTUA LICENSED BUILD WATERMARK  (auto-generated, do not remove)
//  Inert per-file identifier required by the EULA. It uniquely tags this
//  source file so this program is distinguishable from any other build.
//  Contains no functional logic.
//  file: src/core/memory.hpp
// ============================================================================
#ifndef NOCTUA_LICENSE_MARK_865ecb799241dc33cb054e508ced8643
#define NOCTUA_LICENSE_MARK_865ecb799241dc33cb054e508ced8643
namespace noctua_license { namespace mark_865ecb799241dc33cb054e508ced8643 {
    inline constexpr unsigned long long kMarkId = 0x6680e1b630d94e04ull;
    inline constexpr char kMarkFile[] = "src/core/memory.hpp";
    inline constexpr unsigned char kMarkEntropy[] = { 0xad, 0x00, 0x6d, 0x83, 0x8e, 0xaa, 0xaf, 0x21, 0x40, 0x7e, 0x26, 0x18, 0xa1, 0x7a, 0x1c, 0x39 };
    inline constexpr unsigned long  kMarkSalts[]  = { 0x2377350cul, 0xf9eac999ul, 0x0f991a3aul, 0xec7ec0d6ul, 0x887aa685ul, 0x0404864cul, 0x696237e9ul, 0xd30a0b1bul };
    [[maybe_unused]] inline unsigned long long mark_digest() noexcept {
        unsigned long long h = 1469598103934665603ull ^ kMarkId;
        for (unsigned char b : kMarkEntropy) { h ^= b; h *= 1099511628211ull; }
        for (unsigned long  s : kMarkSalts)  { h ^= s; h *= 1099511628211ull; }
        return h;
    }
} } // namespace noctua_license::mark_865ecb799241dc33cb054e508ced8643
#endif // NOCTUA_LICENSE_MARK_865ecb799241dc33cb054e508ced8643
