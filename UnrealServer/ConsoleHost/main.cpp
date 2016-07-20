#include <iostream>
#include <thread>
#include "../LogicLib/Main.h"

int main(void)
{
	NLogicLib::Main LogicMain;
	LogicMain.Init();

	std::thread logicThread([&]() {LogicMain.Run(); });

	std::cout << "press any key to exit..";
	getchar();

	LogicMain.Stop();
	logicThread.join();

	return 0;
}