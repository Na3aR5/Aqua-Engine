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
		Config(const aqua::EngineInfo& info);
		Config& SpecifyEngineInternalInfo(const EngineInternalInfo& info) noexcept;

	public:
		struct EngineInfo {
			aqua::EngineInfo   external;
			EngineInternalInfo internal;
		};

		struct LoggerInfo {
			enum class Destination {
				FILE,
				CONSOLE
			} destination;

			StringLiteralPointer exitCmd;

			static constexpr uint16_t MAX_MESSAGE_LENGTH   = 512;
			static constexpr uint8_t  MAX_FORMAT_ARGUMENTS = 8;
			static constexpr uint8_t  MAX_DELAYED_MESSAGES = 10;

#if AQUA_DEBUG
			static constexpr uint8_t  MAX_PACKETS_AT_TIME = 50;
#else
			static constexpr uint8_t  MAX_PACKETS_AT_TIME = 5;
#endif // 
		};

	public:
		const Config::EngineInfo& GetEngineInfo() const noexcept;
		const LoggerInfo&	      GetLoggerInfo() const noexcept;

	private:
		Config::EngineInfo m_engine;
		LoggerInfo		   m_logger;
	}; // class Config
}

#endif // !AQUA_CONFIG_HEADER