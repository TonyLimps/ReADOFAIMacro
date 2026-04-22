#pragma once
#include <InputEvent.h>

#include <vector>

namespace ReADOFAIMacro {
	class Level;
	class PlayScript {
	public:
		explicit PlayScript(const Level& level, const std::vector<VK>& virtualKeys);
		auto getInputs() const -> const std::vector<std::vector<InputEvent>>& {
			return inputs;
		}
	private:
		KeySequence keySequence;
		std::vector<std::vector<InputEvent>> inputs;
	};
}