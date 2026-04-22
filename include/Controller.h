#pragma once

#include <InputEvent.h>
#include <PlayScript.h>

namespace ReADOFAIMacro{
	class Controller{
	public:
		virtual ~Controller() = default;
		virtual void play(const PlayScript& script, VK waitForKey) = 0;
		virtual void stop() = 0;
	};
}