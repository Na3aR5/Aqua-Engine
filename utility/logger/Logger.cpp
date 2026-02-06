#include <iostream>
#include <string>

#if defined(WIN32) && WIN32
#include <windows.h>
#endif

static void SetConsoleColor(WORD color) {
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), color);
}

int main() {
	std::cout << "Start logger ...\n\n";

	std::string message;
	while (std::getline(std::cin, message)) {
		if (message == "/exit") {
			SetConsoleColor(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
			std::cout << "\nTerminate logger ...\n";
			break; // end process request
		}
		int logLevel = (int)message[0];

		switch (logLevel) {
			case 0: // FATAL
				SetConsoleColor(FOREGROUND_RED | FOREGROUND_INTENSITY);
				std::cout << "[FATAL ERROR] - ";
				break;

			case 1: // WARNING
				SetConsoleColor(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
				std::cout << "[WARNING] - ";
				break;
		}
		std::cout << (message.c_str() + 1) << std::endl;
	}

	system("pause");

	return 0;
}