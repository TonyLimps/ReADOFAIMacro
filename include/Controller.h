#pragma once

#include <InputEvent.h>
#include <PlayScript.h>

namespace ReADOFAIMacro{
	class Controller{
	public:
		virtual ~Controller() = default;
		virtual void play(const PlayScript& script, const KeySequence& keySequence, VK waitForKey) = 0;
		virtual void stop() = 0;
	protected:
		static VK getKeyFromOffset(const KeySequence& seq, KeyOffset offset) {
			const VK* base = &seq.n4;
			return *(base + static_cast<int>(offset));
		}
	};
}