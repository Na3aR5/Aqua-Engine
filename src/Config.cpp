#include <aqua/pch.h>
#include <aqua/Config.h>

aqua::Config::Config(const aqua::EngineInfo& info) :
	m_engine{
		.external = info,
		.internal = EngineInternalInfo{
			.name    = Literal("Aqua Engine"),
			.version = Version(0, 0, 0)
		}
	},
	m_logger {
		.destination = LoggerInfo::Destination::CONSOLE,
		.exitCmd	 = Literal("/exit\n")
	}
{}

aqua::Config& aqua::Config::SpecifyEngineInternalInfo(const EngineInternalInfo& info) noexcept {
	m_engine.internal = info;
	return *this;
}

const aqua::Config::EngineInfo& aqua::Config::GetEngineInfo() const noexcept { return m_engine; }
const aqua::Config::LoggerInfo& aqua::Config::GetLoggerInfo() const noexcept { return m_logger; }