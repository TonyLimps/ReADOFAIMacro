#include <Level.h>
#include <WinAPIController.h>
#include <iostream>

using namespace ReADOFAIMacro;

int main() {
	std::string path;
	std::getline(std::cin, path);
	Level level(path);
	PlayScript script(level);
	KeySequence keySequence{};
	keySequence.p1 = P;
	script.setKeySequence(keySequence);
	WinAPIController controller;
	controller.play(script,P);
}