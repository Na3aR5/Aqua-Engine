#include <iostream>
#include <string>

#if defined(WIN32) && WIN32
	#include <windows.h>
#endif

static void SetConsoleColor(WORD color) {
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), color);
}

static int WriteTime(const std::string& message) {
	// write time point
	int i = 1;
	for (; message[i] != ']'; ++i) {
		std::cout << message[i];
	}
	std::cout << message[i++] << ' ';
	return i;
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

		int offset = 1;
		switch (logLevel) {
			case 0: // FATAL
				SetConsoleColor(FOREGROUND_RED | FOREGROUND_INTENSITY);
				offset = WriteTime(message);
				std::cout << "[FATAL ERROR] - ";
				break;

			case 1: // WARNING
				SetConsoleColor(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
				offset = WriteTime(message);
				std::cout << "[WARNING] - ";
				break;

			case 2: // INFO
				SetConsoleColor(FOREGROUND_BLUE | FOREGROUND_INTENSITY);
				offset = WriteTime(message);
				std::cout << "[INFO] - ";
				break;

			case 3: // Memory allocation / deallocation
				SetConsoleColor(FOREGROUND_GREEN | FOREGROUND_INTENSITY);
				offset = WriteTime(message);
				std::cout << "[MEMORY] - ";
				break;

			case 4: // Debug info
				SetConsoleColor(FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
				offset = WriteTime(message);
				std::cout << "[DEBUG] - ";
				break;
		}
		// write actual message
		std::cout << (message.c_str() + offset) << '\n';
	}

	system("pause");

	return 0;
}