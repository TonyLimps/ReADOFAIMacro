#pragma once
#include <InputEvent.h>

#include <vector>

namespace ReADOFAIMacro {
	class Level;
	class PlayScript {
	public:
		explicit PlayScript(const Level& level);
		auto getInputs() const -> const std::vector<InputEvent>& {
			return inputs;
		}
		auto getTimeStamps() const -> const std::vector<uint_fast64_t>& {
			return timeStamps;
		}
		void setKeySequence(const KeySequence& sequence) {
			keySequence = sequence;
		}
	private:
		KeySequence keySequence;
		std::vector<InputEvent> inputs;
		std::vector<uint_fast64_t> timeStamps;
	};
}