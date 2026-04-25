#include <iostream>
#include <WinAPIController.h>

uint_fast64_t getTimestampMs() {
	return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

bool isMouseAtZero() {
	POINT p;
	if (GetCursorPos(&p)) {
		return p.x == 0 && p.y == 0;
	}
	return false;
}

void releaseKeys() {
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

namespace ReADOFAIMacro {

	auto WinAPIController::convertInputEvents(const std::vector<InputEvent>& events, const KeySequence& keySequence) -> std::vector<INPUT> {
		const size_t inputSize = events.size();
		std::vector<INPUT> inputs;
		for (int i = 0; i < inputSize; i++) {
			const auto& e = events[i];
			const VK vk = getKeyFromOffset(keySequence, e.key);
			INPUT input{};
			input.type = INPUT_KEYBOARD;
			if (!e.state) {
				input.ki.dwFlags = KEYEVENTF_KEYUP;
			}
			input.ki.wVk = vk;

			// 扫描码转换
			// 对于特殊键（左右Ctrl/Alt/Shift等）需要设置扫描码以正确使用SendInput
			switch (vk) {
				case LEFT_CONTROL:
					input.ki.wScan = 0x1D;
					break;
				case RIGHT_CONTROL:
					input.ki.wScan = 0x1D;
					input.ki.dwFlags |= KEYEVENTF_EXTENDEDKEY;
					break;
				case LEFT_ALT:
					input.ki.wScan = 0x38;
					break;
				case RIGHT_ALT:
					input.ki.wScan = 0x38;
					input.ki.dwFlags |= KEYEVENTF_EXTENDEDKEY;
					break;
				case LEFT_SHIFT:
					input.ki.wScan = 0x2A;
					break;
				case RIGHT_SHIFT:
					input.ki.wScan = 0x36;
					break;
				case LEFT_WINDOWS:
					input.ki.wScan = 0x5B;
					input.ki.dwFlags |= KEYEVENTF_EXTENDEDKEY;
					break;
				case RIGHT_WINDOWS:
					input.ki.wScan = 0x5C;
					input.ki.dwFlags |= KEYEVENTF_EXTENDEDKEY;
					break;
				case MENU:
					input.ki.wScan = 0x5D;
					input.ki.dwFlags |= KEYEVENTF_EXTENDEDKEY;
					break;
				default:
					input.ki.wScan = MapVirtualKeyW(static_cast<UINT>(vk), MAPVK_VK_TO_VSC);
					break;
			}

			// 如果设置了扫描码，添加KEYEVENTF_SCANCODE标志
			if (input.ki.wScan != 0) {
				input.ki.dwFlags |= KEYEVENTF_SCANCODE;
			}

			inputs.push_back(input);
		}
		return inputs;
	}

	WinAPIController::~WinAPIController() {
		running = false;
		for (auto& thread : threads) {
			if (thread.joinable()) {
				thread.join();
			}
		}
		releaseKeys();
	}

	void WinAPIController::play(const PlayScript& script, const KeySequence& keySequence, VK waitForKey) {
		if (running)
			return;
		const auto& inputs = convertInputEvents(script.getInputs(), keySequence);
		auto timeStamps = script.getTimeStamps();

		while (!(GetAsyncKeyState(waitForKey) & 0x8000)) {
		}
		uint_fast64_t baseTimeStamp = getTimestampMs();
		for (auto& v : timeStamps) {
			v += baseTimeStamp;
		}
		running = true;
		threads.emplace_back([this, inputs, timeStamps] { pressKeys(inputs, timeStamps); });
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
		releaseKeys();
	}

	void WinAPIController::pressKeys(const std::vector<INPUT>& inputs, const std::vector<uint_fast64_t>& timeStamps) const {
		size_t i = 0;
		while (running.load()) {
			while (running.load() && getTimestampMs() < timeStamps[i]) {
			}
			SendInput(1, const_cast<LPINPUT>(&inputs[i]), sizeof(INPUT));
			i++;
		}
	}

} // namespace ReADOFAIMacro