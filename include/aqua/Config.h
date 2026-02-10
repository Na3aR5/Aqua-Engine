#ifndef AQUA_CONFIG_HEADER
#define AQUA_CONFIG_HEADER

#include <aqua/Build.h>
#include <aqua/utility/StringLiteral.h>

#include <cstdint>

namespace aqua {
	// Engine configuration states
	class Config {
	public:
		Config();

	public:
		const struct Logger {
			const enum class Destination {
				FILE,
				CONSOLE
			} destination;

			StringLiteralPointer exitCmd;

			static constexpr uint16_t MAX_MESSAGE_LENGTH   = 512;
			static constexpr uint8_t  MAX_FORMAT_ARGUMENTS = 8;
			static constexpr uint8_t  MAX_PACKETS_AT_TIME  = 5;
			static constexpr uint8_t  MAX_DELAYED_MESSAGES = 3;
		} logger;
	}; // class Config
}

#endif // !AQUA_CONFIG_HEADER