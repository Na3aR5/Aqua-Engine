#include <aqua/Logger.h>
#include <aqua/Assert.h>
#include <aqua/Platform.h>

#if AQUA_PLATFORM_WINDOWS
	#include <Windows.h>
#endif // AQUA_PLATFORM_WINDOWS

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

namespace {
	aqua::Logger* g_Logger = nullptr;
}

aqua::Logger::Logger(
const Config& config, Status& status) : m_config(config) {
	AQUA_ASSERT(g_Logger == nullptr, Literal("Attempt to create another Logger instance"));

	if (g_Logger != nullptr) {
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
			break;
		}

		default:
			break;
	}

	m_isRunning = true;
	m_logThread = std::thread([this]() {
		while (m_isRunning) {
			size_t readIndex  = m_readIndex.load(std::memory_order_relaxed);
			size_t writeIndex = m_writeIndex.load(std::memory_order_acquire);

			if (readIndex != writeIndex) {
				_Packet packet = m_packetBuffer[readIndex % Config::Logger::MAX_PACKETS_AT_TIME];

				if (packet.argCount == 0) { // zero arguments optimization
					m_loggerProcess->WriteMessage(packet.formatString.GetPtr(),
						static_cast<unsigned>(packet.formatString.GetSize()) + 1);
				}
				else {
					uint8_t argIndex = 0;
					unsigned formatStringLength = static_cast<unsigned>(packet.formatString.GetLength());

					m_buffer[0] = (char)packet.level; // level header

					unsigned bufferOffset = 1;
					for (unsigned i = 1; i < formatStringLength && bufferOffset <= Config::Logger::MAX_MESSAGE_LENGTH;) {
						if (packet.formatString[i] == '{' && packet.formatString[i + 1] == '}') { // argument
							int written = _WriteArg(
								bufferOffset,
								(_Packet::ArgumentType)(packet.argTypes & (_Packet::STRING << (argIndex << 1))),
								packet.args[argIndex]
							);
							if (written < 0) break; // message overflow
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

	g_Logger = this;
}

aqua::Logger::~Logger() {
	if (m_isRunning == true) {
		m_isRunning = false;
		m_logThread.join();
	}
}

aqua::Logger& aqua::Logger::Get() noexcept {
	return *g_Logger;
}
const aqua::Logger& aqua::Logger::GetConst() noexcept {
	return *g_Logger;
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