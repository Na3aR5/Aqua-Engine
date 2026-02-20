#include <aqua/Config.h>

aqua::Config::Config(const EngineInfo& info) :
	engine {
		.name = Literal("Aqua Engine"),
		.version = Version(1, 0, 0)
	},
	application {
		.name = info.applicationName,
		.version = info.applicationVersion
	},
	logger {
		.destination = Logger::Destination::CONSOLE,
		.exitCmd     = Literal("/exit\n")
	}
{}