#include <aqua/Logger.h>
#include <aqua/Assert.h>
#include <aqua/Platform.h>

#if AQUA_PLATFORM_WINDOWS
	#include <Windows.h>
#endif // AQUA_PLATFORM_WINDOWS

#include <ctime>

namespace {
#if AQUA_PLATFORM_WINDOWS
	class LoggerProcessNewConsole : public aqua::Logger::ILoggerProcess {
	public:
		struct CommandSet {
			aqua::StringLiteralPointer exitCmd;
		}; // struct CommandSet

	public:
		template <size_t Size>
		LoggerProcessNewConsole(
		aqua::StringLiteral<Size> processPath, const CommandSet& cmdSet, aqua::Status& status) : m_cmdSet(cmdSet) {
			SECURITY_ATTRIBUTES securityAttributes{};
			securityAttributes.nLength		  = sizeof(securityAttributes);
			securityAttributes.bInheritHandle = TRUE;

			HANDLE hRead, hWrite;
			if (!CreatePipe(&hRead, &hWrite, &securityAttributes, 0)) {
				status.EmplaceError(aqua::Error::FAILED_TO_CREATE_LOGGER_OWN_CONSOLE);
				return;
			}

			STARTUPINFO startupInfo{};
			startupInfo.cb		  = sizeof(startupInfo);
			startupInfo.dwFlags   = STARTF_USESTDHANDLES;
			startupInfo.hStdInput = hRead;

			aqua::StringLiteralPointer processPathPtr = processPath;

			PROCESS_INFORMATION processInfo{};
			BOOL result = CreateProcessA(processPathPtr.GetPtr(), NULL, NULL, NULL, TRUE,
				CREATE_NEW_CONSOLE, NULL, NULL, &startupInfo, &processInfo);
			if (!result) {
				status.EmplaceError(aqua::Error::FAILED_TO_CREATE_LOGGER_OWN_CONSOLE);
				return;
			}
			CloseHandle(hRead);

			m_processHandle = processInfo.hProcess;
			m_threadHandle  = processInfo.hThread;
			m_writeHandle   = hWrite;

			status.EmplaceValue();
		}

		LoggerProcessNewConsole(const LoggerProcessNewConsole&) = delete;
		LoggerProcessNewConsole& operator=(const LoggerProcessNewConsole&) = delete;

		LoggerProcessNewConsole(LoggerProcessNewConsole&&) noexcept = delete;
		LoggerProcessNewConsole& operator=(LoggerProcessNewConsole&&) noexcept = delete;

		~LoggerProcessNewConsole() { Destroy(); }

	public:
		virtual aqua::Status WriteMessage(const char* buffer, unsigned size) override {
			DWORD written;
			WriteFile(m_writeHandle, buffer, size, &written, NULL);
			if (written != size) {
				return aqua::Unexpected<aqua::Error>(aqua::Error::FAILED_TO_WRITE_LOG_MESSAGE);
			}
			return aqua::Success{};
		}

		virtual void Destroy() noexcept override {
			if (m_processHandle) {
				if (m_writeHandle) {
					DWORD written;
					WriteFile(m_writeHandle, m_cmdSet.exitCmd.GetPtr(),
						static_cast<DWORD>(m_cmdSet.exitCmd.GetSize()), &written, NULL);
				}
				WaitForSingleObject(m_processHandle, INFINITE);
				CloseHandle(m_processHandle);
			}
			if (m_threadHandle) {
				CloseHandle(m_threadHandle);
			}
			m_writeHandle   = nullptr;
			m_processHandle = nullptr;
			m_threadHandle  = nullptr;
		}

	private:
		HANDLE     m_writeHandle   = nullptr;
		HANDLE     m_processHandle = nullptr;
		HANDLE     m_threadHandle  = nullptr;

		CommandSet m_cmdSet;
	}; // class LoggerProcessNewConsole
#endif // AQUA_PLATFORM_WINDOWS
}

aqua::Logger* aqua::Logger::s_Logger = nullptr;
uint8_t aqua::Logger::s_DelayedMessageCount = 0;
aqua::Logger::_Packet aqua::Logger::s_DelayedMessages[aqua::Config::Logger::MAX_DELAYED_MESSAGES] = {};

aqua::Logger::Logger(
const Config& config, Status& status) : m_config(config) {
	AQUA_ASSERT(s_Logger == nullptr, Literal("Attempt to create another Logger instance"));

	if (!status.IsSuccess()) {
		return;
	}
	if (s_Logger != nullptr) {
		return;
	}

	switch (m_config.logger.destination) {
		case Config::Logger::Destination::CONSOLE: {
			LoggerProcessNewConsole::CommandSet cmdSet = {
				.exitCmd = config.logger.exitCmd
			};
			auto expectedProcess = CreateUniqueData<LoggerProcessNewConsole>(
				Literal(AQUA_LOGGER_CONSOLE_PROCESS_PATH), cmdSet, status
			);
			if (!expectedProcess.HasValue() || !status.IsSuccess()) {
				return;
			}
			m_loggerProcess = std::move(expectedProcess.GetValue());

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
		while (m_isRunning) {
			size_t readIndex  = m_readIndex.load(std::memory_order_relaxed);
			size_t writeIndex = m_writeIndex.load(std::memory_order_acquire);

			if (readIndex != writeIndex) {
				_Packet packet = m_packetBuffer[readIndex % Config::Logger::MAX_PACKETS_AT_TIME];

				m_buffer[0] = (char)packet.level; // level header
				int writtenTime = _WriteTime(1, packet.timePoint); // write ~14 bytes

				if (packet.argCount == 0) { // zero arguments optimization
					int written = sprintf_s(m_buffer + 1 + writtenTime, Config::Logger::MAX_MESSAGE_LENGTH - 15, "%s",
						packet.formatString.GetPtr() + 1);
					m_loggerProcess->WriteMessage(m_buffer, 1 + writtenTime +
						static_cast<unsigned>(packet.formatString.GetLength()));
				}
				else {
					uint8_t argIndex = 0;
					unsigned formatStringLength = static_cast<unsigned>(packet.formatString.GetLength());

					unsigned bufferOffset = 1 + writtenTime; // level header + written 14 bytes of time
					for (unsigned i = 1; i < formatStringLength && bufferOffset <= Config::Logger::MAX_MESSAGE_LENGTH;) {
						if (packet.formatString[i] == '{' && packet.formatString[i + 1] == '}') { // argument
							_Packet::ArgumentType argType =
								(_Packet::ArgumentType)((packet.argTypes >> (argIndex << 1)) & _Packet::STRING);
							int written = _WriteArg(bufferOffset, argType, packet.args[argIndex++]);

							if (written < 0 || (unsigned)written >= Config::Logger::MAX_MESSAGE_LENGTH - bufferOffset) {
								break; // message overflow or format error;
							}
							bufferOffset += written;
							i += 2;
						}
						else {
							m_buffer[bufferOffset++] = packet.formatString[i++];
						}
					}
					if (bufferOffset >= Config::Logger::MAX_MESSAGE_LENGTH) {
						bufferOffset = Config::Logger::MAX_MESSAGE_LENGTH;
						m_buffer[bufferOffset - 1] = '\n'; // cut
					}
					else {
						m_buffer[bufferOffset++] = '\n';
					}
					m_loggerProcess->WriteMessage(m_buffer, bufferOffset);
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

int aqua::Logger::_WriteArg(size_t where, _Packet::ArgumentType type, const _Packet::Argument& arg) noexcept {
	switch (type) {
		case _Packet::ArgumentType::LL:
			return sprintf_s(m_buffer + where, Config::Logger::MAX_MESSAGE_LENGTH - where, "%lld", arg.ll);

		case _Packet::ArgumentType::ULL:
			return sprintf_s(m_buffer + where, Config::Logger::MAX_MESSAGE_LENGTH - where, "%llu", arg.ull);

		case _Packet::ArgumentType::DOUBLE:
			return sprintf_s(m_buffer + where, Config::Logger::MAX_MESSAGE_LENGTH - where, "%f", arg.dbl);

		case _Packet::ArgumentType::STRING:
			return sprintf_s(m_buffer + where, Config::Logger::MAX_MESSAGE_LENGTH - where, "%s", arg.str.GetPtr());

		default:
			break;
	}
	return 0;
}

// h:m:s.ms
int aqua::Logger::_WriteTime(size_t where, std::chrono::system_clock::time_point time) noexcept {
	auto s = std::chrono::floor<std::chrono::seconds>(time);
	auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(time - s).count();

	auto days = std::chrono::floor<std::chrono::days>(s);
	auto time_of_day = s - days;

	int h = int(duration_cast<std::chrono::hours>(time_of_day).count());
	int mi = int(duration_cast<std::chrono::minutes>(time_of_day).count() % 60);
	int se = int(duration_cast<std::chrono::seconds>(time_of_day).count() % 60);

	return std::snprintf(m_buffer + where, Config::Logger::MAX_MESSAGE_LENGTH - where,
		"[%02d:%02d:%02d.%03d]",
		h, mi, se, (int)ms
	);
}