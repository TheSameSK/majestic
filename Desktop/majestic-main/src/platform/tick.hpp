#pragma once
#include <vector>
#include <sysinfoapi.h>
#include <functional>

namespace tick
{
	class thread_invoker
	{
	public:

		inline static void queue(std::function<void()> func)
		{
			funcs_to_invoke.emplace_back(std::make_pair(func, GetTickCount64()));
		}

		void on_tick()
		{
			auto current_tick = GetTickCount64();
			for (auto& funcs : funcs_to_invoke)
			{
				if (current_tick - funcs.second < 1000)
				{
					funcs.first();
				}
			}

			funcs_to_invoke.clear();
		}
	private:
		inline static std::vector<std::pair<std::function<void()>, uintptr_t>> funcs_to_invoke;
	};

	inline thread_invoker pnative;
}


// ============================================================================
//  NOCTUA LICENSED BUILD WATERMARK  (auto-generated, do not remove)
//  Inert per-file identifier required by the EULA. It uniquely tags this
//  source file so this program is distinguishable from any other build.
//  Contains no functional logic.
//  file: src/platform/tick.hpp
// ============================================================================
#ifndef NOCTUA_LICENSE_MARK_4854a906fe90a2775f8bcf6944aa1501
#define NOCTUA_LICENSE_MARK_4854a906fe90a2775f8bcf6944aa1501
namespace noctua_license { namespace mark_4854a906fe90a2775f8bcf6944aa1501 {
    inline constexpr unsigned long long kMarkId = 0x8469c60293529a5cull;
    inline constexpr char kMarkFile[] = "src/platform/tick.hpp";
    inline constexpr unsigned char kMarkEntropy[] = { 0xa1, 0x42, 0xa6, 0x26, 0x80, 0x1a, 0xec, 0xd8, 0x1c, 0xd8, 0xd4, 0x1a, 0x3f, 0xc3, 0xd8, 0x25 };
    inline constexpr unsigned long  kMarkSalts[]  = { 0x50b10888ul, 0x68a8f712ul, 0x966c5c21ul, 0x7a44a554ul, 0x247e9ec9ul, 0x776fb3c1ul, 0xdcbc23d4ul, 0xdc43a379ul };
    [[maybe_unused]] inline unsigned long long mark_digest() noexcept {
        unsigned long long h = 1469598103934665603ull ^ kMarkId;
        for (unsigned char b : kMarkEntropy) { h ^= b; h *= 1099511628211ull; }
        for (unsigned long  s : kMarkSalts)  { h ^= s; h *= 1099511628211ull; }
        return h;
    }
} } // namespace noctua_license::mark_4854a906fe90a2775f8bcf6944aa1501
#endif // NOCTUA_LICENSE_MARK_4854a906fe90a2775f8bcf6944aa1501
