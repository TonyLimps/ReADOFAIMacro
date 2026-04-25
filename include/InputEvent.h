#pragma once

#include <cstdint>

namespace ReADOFAIMacro {
	enum VK : int {
		UNKNOWN = 0,
		BACKSPACE = 8,
		TAB = 9,
		RETURN = 13,
		PAUSE = 19,
		CAPS_LOCK = 20,
		ESCAPE = 27,
		SPACE = 32,
		PAGE_UP = 33,
		PAGE_DOWN = 34,
		END = 35,
		HOME = 36,
		LEFT = 37,
		UP = 38,
		RIGHT = 39,
		DOWN = 40,
		PRINT_SCREEN = 44,
		INSERT = 45,
		DELETE = 46,
		ALPHA_0 = 48,
		ALPHA_1 = 49,
		ALPHA_2 = 50,
		ALPHA_3 = 51,
		ALPHA_4 = 52,
		ALPHA_5 = 53,
		ALPHA_6 = 54,
		ALPHA_7 = 55,
		ALPHA_8 = 56,
		ALPHA_9 = 57,
		A = 65,
		B = 66,
		C = 67,
		D = 68,
		E = 69,
		F = 70,
		G = 71,
		H = 72,
		I = 73,
		J = 74,
		K = 75,
		L = 76,
		M = 77,
		N = 78,
		O = 79,
		P = 80,
		Q = 81,
		R = 82,
		S = 83,
		T = 84,
		U = 85,
		V = 86,
		W = 87,
		X = 88,
		Y = 89,
		Z = 90,
		LEFT_WINDOWS = 91,
		RIGHT_WINDOWS = 92,
		MENU = 93,
		NUMPAD_0 = 96,
		NUMPAD_1 = 97,
		NUMPAD_2 = 98,
		NUMPAD_3 = 99,
		NUMPAD_4 = 100,
		NUMPAD_5 = 101,
		NUMPAD_6 = 102,
		NUMPAD_7 = 103,
		NUMPAD_8 = 104,
		NUMPAD_9 = 105,
		MULTIPLY = 106,
		ADD = 107,
		SUBTRACT = 109,
		DECIMAL = 110,
		DIVIDE = 111,
		F1 = 112,
		F2 = 113,
		F3 = 114,
		F4 = 115,
		F5 = 116,
		F6 = 117,
		F7 = 118,
		F8 = 119,
		F9 = 120,
		F10 = 121,
		F11 = 122,
		F12 = 123,
		NUM_LOCK = 144,
		SCROLL_LOCK = 145,
		LEFT_SHIFT = 160,
		RIGHT_SHIFT = 161,
		LEFT_CONTROL = 162,
		RIGHT_CONTROL = 163,
		LEFT_ALT = 164,
		RIGHT_ALT = 165,
		SEMICOLON = 186,
		EQUALS = 187,
		COMMA = 188,
		MINUS = 189,
		PERIOD = 190,
		SLASH = 191,
		GRAVE_ACCENT = 192,
		LEFT_BRACKET = 219,
		BACKSLASH = 220,
		RIGHT_BRACKET = 221,
		APOSTROPHE = 222
	};

	enum KeyOffset : int {
		n4 = 0, n3 = 1, n2 = 2, n1 = 3,
		p1 = 4, p2 = 5, p3 = 6, p4 = 7,
		sn4 = 8, sn3 = 9, sn2 = 10, sn1 = 11,
		sp1 = 12, sp2 = 13, sp3 = 14, sp4 = 15
	};

	struct KeySequence {
		// p:主手 n:副手
		// 这里右手是主手，那么一般键盘上的按键位置映射如下所示
		VK n4,n3,n2,n1,p1,p2,p3,p4; // 手8
		VK sn4,sn3,sn2,sn1,sp1,sp2,sp3,sp4; // 手16
		// 暂时用不上的
		// VK tn4,tn3,tn2,tn1,tp1,tp2,tp3,tp4; // 手键第三排(手18键可能用)

		// VK fn4,fn3,fn2,fn1,fp1,fp2,fp3,fp4; // 脚8
		// VK fsn4,fsn3,fsn2,fsn1,fsp1,fsp2,fsp3,fsp4; // 脚16
	};

	struct InputEvent {
		KeyOffset key{};
		bool state{};

		InputEvent() = default;
		InputEvent(KeyOffset key, bool state) {
			this-> key = key;
			this-> state = state;
		}
	};
}
