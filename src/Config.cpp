#include <aqua/Config.h>

aqua::Config::Config() :
	logger {
		.destination = Logger::Destination::CONSOLE,
		.exitCmd     = Literal("/exit\n")
	}
{}