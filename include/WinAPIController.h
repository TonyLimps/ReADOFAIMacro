#pragma once

#include <Controller.h>
#include <atomic>
#include <chrono>
#include <thread>
#include <windows.h>

namespace ReADOFAIMacro{

	class WinAPIController : public Controller {
	public:
		WinAPIController() : running(false) {};
		~WinAPIController() override;
		void play(const PlayScript& script, const KeySequence& keySequence, VK waitForKey) override;
		void stop() override;
	private:
		std::atomic<bool> running;
		std::vector<std::thread> threads;
		void pressKeys(const std::vector<INPUT>& inputs, const std::vector<uint_fast64_t>& timeStamps) const;
		auto convertInputEvents(const std::vector<InputEvent>& events, const KeySequence& keySequence) -> std::vector<INPUT>;
	};
}
