#pragma once
#include <vector>
#include <utility>
#include <cstdint>

namespace game {
	namespace animals {
		// list of known animal model hashes, internal model name and localized display name
		struct AnimalEntry { uint32_t hash; const char* model; const char* display; };

		inline const std::vector<AnimalEntry> kAnimalList = {
			{ 1682622302u, "A_C_COYOTE", "Койот" },
			{ 3462393972u, "A_C_BOAR", "Кабан" }, // 0xCE5FF074
			{ 3753204865u, "A_C_Rabbit_01", "Заяц" }, // 0xDFB55C81
			{ 402729631u,  "A_C_Crow",      "Ворон" }, // 0x18012A9F
			{ 3630914197u, "A_C_Deer",      "Олень" }, // 0xD86B5A95
			{ 2971380566u, "A_C_Pig",       "Свинья" },  // 0xB11BAB56
			{ 1457690978u, "A_C_Cormorant", "Баклан" }, // 0x56E29962
			{ 2864127842u, "A_C_Chickenhawk", "Ястреб-кулик" }, // 0xAAB71F62
		};

		inline bool isAnimal(uint32_t hash) {
			for (const auto& e : kAnimalList) {
				if (e.hash == hash) return true;
			}
			return false;
		}

		inline const char* getAnimalModelName(uint32_t hash) {
			for (const auto& e : kAnimalList) {
				if (e.hash == hash) return e.model;
			}
			return nullptr;
		}

		inline const char* getAnimalDisplayName(uint32_t hash) {
			for (const auto& e : kAnimalList) {
				if (e.hash == hash) return e.display;
			}
			return nullptr;
		}
	}

	inline bool isAnimal(uint32_t hash) { return animals::isAnimal(hash); }
	inline const char* getAnimalName(uint32_t hash) { return animals::getAnimalModelName(hash); }
	inline const char* getAnimalDisplayName(uint32_t hash) { return animals::getAnimalDisplayName(hash); }
}
