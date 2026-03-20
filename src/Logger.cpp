#include <aqua/pch.h>
#include <aqua/Logger.h>
#include <aqua/Assert.h>
#include <aqua/System.h>

namespace {
	class LoggerProcessNewConsole : public aqua::Logger::ILoggerProcess {
	public:
		struct CommandSet {
			aqua::StringLiteralPointer exitCmd;
		}; // struct CommandSet

	public:
		LoggerProcessNewConsole(const aqua::Config& config, aqua::Status& status) {
			auto result = aqua::System::Get().Create<aqua::System::IChildProcess>(
				aqua::System::ChildProcessCreateInfo {
					.executablePath = config.GetLoggerInfo().loggerExecutablePath.GetPtr(),
					.flags = aqua::System::ChildProcessCreateInfo::ENABLE_WRITE_BIT
				}
			);
			if (!result.HasValue()) {
				status.EmplaceError(result.GetError());
				return;
			}
			m_processHandle = result.GetValue();

			m_cmdSet.exitCmd = config.GetLoggerInfo().exitCmd;
		}

		LoggerProcessNewConsole(const LoggerProcessNewConsole&) = delete;
		LoggerProcessNewConsole& operator=(const LoggerProcessNewConsole&) = delete;

		LoggerProcessNewConsole(LoggerProcessNewConsole&&) noexcept = delete;
		LoggerProcessNewConsole& operator=(LoggerProcessNewConsole&&) noexcept = delete;

		~LoggerProcessNewConsole() { Destroy(); }

	public:
		virtual void Destroy() noexcept override {
			WriteMessage(m_cmdSet.exitCmd.GetPtr(), (unsigned)m_cmdSet.exitCmd.GetSize());
			aqua::System::Get().Destroy(m_processHandle);
		}

		virtual aqua::Status WriteMessage(const char* buffer, unsigned size) override {
			return m_processHandle->Write(buffer, size);
		}

	private:
		aqua::System::Handle<aqua::System::IChildProcess> m_processHandle;
		CommandSet										  m_cmdSet;
	}; // class LoggerProcessNewConsole
}

aqua::Logger* aqua::Logger::s_Logger = nullptr;
uint8_t aqua::Logger::s_DelayedMessageCount = 0;
aqua::Logger::_Packet aqua::Logger::s_DelayedMessages[aqua::Config::LoggerInfo::MAX_DELAYED_MESSAGES] = {};

aqua::Logger::Logger(
const Config& config, Status& status) : m_config(config) {
	AQUA_ASSERT(s_Logger == nullptr, Literal("Attempt to create another Logger instance"));

	if (!status.IsSuccess() || s_Logger != nullptr) {
		return;
	}

	switch (m_config.GetLoggerInfo().destination) {
		case Config::LoggerInfo::Destination::CONSOLE: {
			auto process = aqua::CreateUniqueData<LoggerProcessNewConsole>(m_config, status);
			if (!process.HasValue() || !status.IsSuccess()) {
				status.EmplaceError(process.HasValue() ? process.GetError() : status.GetError());
				return;
			}
			m_loggerProcess = std::move(process.GetValue());

			AQUA_LOG(Literal("Logger own console is created"));
			break;
		}

		default:
			break;
	}

	m_writeIndex.store(s_DelayedMessageCount, std::memory_order_relaxed);
	for (uint8_t i = 0; i < s_DelayedMessageCount; ++i) {
		m_packetBuffer[i] = s_DelayedMessages[i];
	}

	m_isRunning = true;
	m_logThread = std::thread([this]() {
		while (m_isRunning || m_readIndex.load() != m_writeIndex.load()) {
			size_t readIndex  = m_readIndex.load(std::memory_order_relaxed);
			size_t writeIndex = m_writeIndex.load(std::memory_order_acquire);

			if (readIndex != writeIndex) {
				_Packet packet = m_packetBuffer[readIndex % Config::LoggerInfo::MAX_PACKETS_AT_TIME];

				m_buffer[0] = (char)packet.level; // level header
				int writtenTime = _WriteTime(
					m_buffer + 1,
					Config::LoggerInfo::MAX_MESSAGE_LENGTH - 1,
					packet.timePoint
				); // write ~14 bytes

				if (packet.argCount == 0) { // zero arguments optimization
					sprintf_s(m_buffer + 1 + writtenTime,
						(size_t)Config::LoggerInfo::MAX_MESSAGE_LENGTH - (size_t)(1 + writtenTime), "%s",
						packet.formatString.GetPtr()
					);
					m_loggerProcess->WriteMessage(m_buffer, 2 + writtenTime +
						static_cast<unsigned>(packet.formatString.GetLength()));
				}
				else {
					size_t offset = (size_t)(1 + writtenTime);
					size_t size = (size_t)Config::LoggerInfo::MAX_MESSAGE_LENGTH;

					size_t written = _MakeFormat(
						packet.formatString.GetPtr(), packet.formatString.GetLength(),
						m_buffer, size, offset, packet
					);
					m_loggerProcess->WriteMessage(m_buffer, static_cast<unsigned>(written));
				}
				m_readIndex.store(readIndex + 1, std::memory_order_release);
			}
			else {
				std::this_thread::sleep_for(std::chrono::milliseconds(1));
			}
		}
	});

	s_Logger = this;
}

aqua::Logger::~Logger() {
	if (m_isRunning == true) {
		m_isRunning = false;
		m_logThread.join();
	}
	s_Logger = nullptr;
}

aqua::Logger& aqua::Logger::Get() noexcept {
	return *s_Logger;
}
const aqua::Logger& aqua::Logger::GetConst() noexcept {
	return *s_Logger;
}

int aqua::Logger::_WriteArg(char* buffer, size_t size, _Packet::ArgumentType type, const _Packet::Argument& arg) noexcept {
	switch (type) {
		case _Packet::ArgumentType::LL:
			return sprintf_s(buffer, size, "%lld", arg.ll);

		case _Packet::ArgumentType::ULL:
			return sprintf_s(buffer, size, "%llu", arg.ull);

		case _Packet::ArgumentType::DOUBLE:
			return sprintf_s(buffer, size, "%f", arg.dbl);

		case _Packet::ArgumentType::STRING:
			return sprintf_s(buffer, size, "%s", arg.str);

		default:
			break;
	}
	return 0;
}

// h:m:s.ms
int aqua::Logger::_WriteTime(char* buffer, size_t size, std::chrono::system_clock::time_point time) noexcept {
	auto s = std::chrono::floor<std::chrono::seconds>(time);
	auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(time - s).count();

	auto days = std::chrono::floor<std::chrono::days>(s);
	auto time_of_day = s - days;

	int h = int(duration_cast<std::chrono::hours>(time_of_day).count());
	int mi = int(duration_cast<std::chrono::minutes>(time_of_day).count() % 60);
	int se = int(duration_cast<std::chrono::seconds>(time_of_day).count() % 60);

	return std::snprintf(buffer, size, "[%02d:%02d:%02d.%03d]",
		h, mi, se, (int)ms
	);
}

size_t aqua::Logger::_MakeFormat(
const char* format, size_t formatLength, char* buffer, size_t size, size_t offset, _Packet& packet) noexcept {
	uint8_t argIndex = 0;
	for (size_t i = 0; i < formatLength && offset <= size;) {
		if (format[i] == '{' && format[i + 1] == '}') { // argument
			_Packet::ArgumentType argType =
				(_Packet::ArgumentType)((packet.argTypes >> (argIndex << 1)) & _Packet::STRING);
			int written = _WriteArg(
				buffer + offset,
				size - offset,
				argType,
				packet.args[argIndex++]
			);
			if (written < 0 || (size_t)written >= size - offset) {
				break; // message overflow or format error
			}
			offset += written;
			i += 2;
		}
		else {
			buffer[offset++] = format[i++];
		}
	}
	if (offset >= size) {
		offset = size;
		buffer[offset - 1] = '\n'; // cut
	}
	else {
		buffer[offset++] = '\n';
	}
	return offset;
}