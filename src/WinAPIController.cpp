#include <WinAPIController.h>

uint_fast64_t getTimestampMs() {
	return std::chrono::duration_cast<std::chrono::milliseconds>(
			   std::chrono::system_clock::now().time_since_epoch())
		.count();
}

bool isMouseAtZero() {
	POINT p;
	if (GetCursorPos(&p)) {
		return p.x == 0 && p.y == 0;
	}
	return false;
}

namespace ReADOFAIMacro {



	WinAPIController::~WinAPIController() {
		running = false;
		for (auto& thread : threads) {
			if (thread.joinable()) {
				thread.join();
			}
		}
		for (int key = 1; key < 256; key++) {
			INPUT up;
			up.type = INPUT_KEYBOARD;
			up.ki.wVk = key;
			up.ki.dwFlags = KEYEVENTF_KEYUP;
			up.ki.time = 0;
			up.ki.dwExtraInfo = 0;
			up.ki.wScan = 0;
			SendInput(1, &up, sizeof(INPUT));
		}
	}
	void WinAPIController::play(const PlayScript& script, VK waitForKey) {
		const std::vector<std::vector<InputEvent>>& originalInputs = script.getInputs();
		size_t inputSize = originalInputs.size();
		std::vector<std::vector<INPUT>> inputs(inputSize);
		std::vector<std::vector<uint_fast64_t>> timeStamps(inputSize);
		for (int i = 0; i < inputSize; i++) {
			for (auto& e : originalInputs[i]) {
				INPUT input = {};
				input.type = INPUT_KEYBOARD;
				if (!e.state) {
					input.ki.dwFlags = KEYEVENTF_KEYUP;
				}
				input.ki.wVk = e.key;
				inputs[i].push_back(input);
				timeStamps[i].push_back(e.time);
			}
		}
		while (!(GetAsyncKeyState(waitForKey) & 0x8000)) {}

		uint_fast64_t baseTimeStamp = getTimestampMs();
		for (auto& v : timeStamps) {
			for (auto& t : v) {
				t += baseTimeStamp;
			}
		}
		const size_t keyCount = timeStamps.size();
		running = true;
		for (int i = 0; i < keyCount; i++) {
			threads.emplace_back([this, inputs, timeStamps, i] { pressKeys(inputs[i], timeStamps[i]);});
		}
		while (!isMouseAtZero()) {
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}
		stop();
	}

	void WinAPIController::stop() {
		running = false;
		for (auto& thread : threads) {
			if (thread.joinable()) {
				thread.join();
			}
		}
		for (int key = 1; key < 256; key++) {
			INPUT up;
			up.type = INPUT_KEYBOARD;
			up.ki.wVk = key;
			up.ki.dwFlags = KEYEVENTF_KEYUP;
			up.ki.time = 0;
			up.ki.dwExtraInfo = 0;
			up.ki.wScan = 0;
			SendInput(1, &up, sizeof(INPUT));
		}
	}

	void WinAPIController::pressKeys(const std::vector<INPUT>& inputs, const std::vector<uint_fast64_t>& timeStamps) const {
		size_t i = 0;
		while (running.load()) {
			while (running.load() && getTimestampMs() < timeStamps[i]) {}
			SendInput(1, const_cast<LPINPUT>(&inputs[i]), sizeof(INPUT));
			i++;
		}
	}



}