#include <Level.h>
#include <WinAPIController.h>
#include <iostream>

using namespace ReADOFAIMacro;

int main() {
	std::string path;
	std::getline(std::cin, path);
	Level level(path);
	PlayScript script(level,840);
	KeySequence keySequence{};
	keySequence.p1 = P;
	keySequence.p2 = EQUALS;
	keySequence.p3 = BACKSPACE;
	keySequence.p4 = BACKSLASH;
	keySequence.n1 = E;
	keySequence.n2 = ALPHA_2;
	keySequence.n3 = ALPHA_1;
	keySequence.n4 = TAB;
	keySequence.sp1 = RIGHT_ALT;
	keySequence.sp2 = PERIOD;
	keySequence.sp3 = RIGHT_CONTROL;
	keySequence.sp4 = RETURN;
	keySequence.sn1 = SPACE;
	keySequence.sn2 = C;
	keySequence.sn3 = LEFT_CONTROL;
	keySequence.sn4 = CAPS_LOCK;
	WinAPIController controller;
	controller.play(script,keySequence,F);
	return 0;
}