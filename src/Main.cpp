#include <Level.h>
#include <WinAPIController.h>
#include <iostream>

using namespace ReADOFAIMacro;

int main() {
	std::string path;
	std::getline(std::cin, path);
	Level level(path);
	std::vector<VK> vks{ALPHA_2,E,P,EQUALS};
	PlayScript script(level,vks);
	WinAPIController controller;
	controller.play(script,P);
}