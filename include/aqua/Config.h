#ifndef AQUA_CONFIG_HEADER
#define AQUA_CONFIG_HEADER

#include <aqua/Build.h>
#include <aqua/engine/Defines.h>
#include <aqua/utility/StringLiteral.h>

#include <cstdint>

namespace aqua {
	// Engine configuration
	class Config {
	public:
		Config(const EngineInfo& engineInfo);

	public:
		const struct Engine {
			StringLiteralPointer name;
			Version              version;
		} engine;

		const struct Application {
			const char* name;
			Version     version;
		} application;

		const struct Logger {
			const enum class Destination {
				FILE,
				CONSOLE
			} destination;

			StringLiteralPointer exitCmd;

			static constexpr uint16_t MAX_MESSAGE_LENGTH   = 512;
			static constexpr uint8_t  MAX_FORMAT_ARGUMENTS = 8;
			static constexpr uint8_t  MAX_PACKETS_AT_TIME  = 10;
			static constexpr uint8_t  MAX_DELAYED_MESSAGES = 10;
		} logger;
	}; // class Config
}

#endif // !AQUA_CONFIG_HEADER