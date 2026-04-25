#pragma once
#include <InputEvent.h>

#include <vector>

namespace ReADOFAIMacro {
	class Level;
	class PlayScript {
	public:
		explicit PlayScript(const Level& level, double maxIndexingBpm);
		auto getInputs() const -> const std::vector<InputEvent>& {
			return inputs;
		}
		auto getTimeStamps() const -> const std::vector<uint_fast64_t>& {
			return timeStamps;
		}
	private:
		size_t allocatingIndex = 0;
		size_t allocatingIndex1 = 0;
		std::vector<InputEvent> inputs;
		std::vector<uint_fast64_t> floorTimeStamps;
		std::vector<uint_fast64_t> timeStamps;

		void allocateFingers(int count, bool usePreferredHand, bool lasting = true);
	};
}