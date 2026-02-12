#ifndef AQUA_LOGGER_HEADER
#define AQUA_LOGGER_HEADER

#include <aqua/Config.h>
#include <aqua/utility/StringLiteral.h>
#include <aqua/datastructures/String.h>

#include <concepts>
#include <atomic>
#include <thread>
#include <chrono>

namespace aqua {
	class Engine;

	// Async ring buffer logger
	class Logger {
	public:
		class ILoggerProcess {
		public:
			virtual ~ILoggerProcess() = default;

		public:
			virtual Status WriteMessage(const char* buffer, unsigned size) = 0;

			virtual void Destroy() noexcept = 0;
		}; // class ILoggerProcess

		enum class Level : uint8_t {
			FATAL,
			WARNING,
			INFO,
			MEMORY
		}; // enum Level

	public:
		Logger(const Logger&)     = delete;
		Logger(Logger&&) noexcept = delete;

		Logger& operator=(const Logger&)     = delete;
		Logger& operator=(Logger&&) noexcept = delete;

		~Logger();

	public:
		static Logger&		 Get()      noexcept;
		static const Logger& GetConst() noexcept;

		template <typename ... Types>
		static void LogFormat(Level level, const String256& format, const Types& ... args) {
			static_assert(sizeof...(args) <= Config::Logger::MAX_FORMAT_ARGUMENTS,
				"aqua::Logger::LogFormat - too many arguments");

			auto timePoint = std::chrono::system_clock::now();

			if (s_Logger == nullptr) {
				if (s_DelayedMessageCount < Config::Logger::MAX_DELAYED_MESSAGES) {
					_Packet& packet = s_DelayedMessages[s_DelayedMessageCount++];
					_CreatePacketInPlace(packet, format, level, args...);
				}
				return;
			}
			Logger& logger = *s_Logger;

#if AQUA_DEBUG
				++logger.m_logMessagesRequested;
#endif // AQUA_DEBUG

			size_t writeIndex = logger.m_writeIndex.load(std::memory_order_relaxed);
			size_t readIndex  = logger.m_readIndex.load(std::memory_order_acquire);

			if (writeIndex - readIndex >= Config::Logger::MAX_PACKETS_AT_TIME) {
				return; // log capacity overflow; drop log message
			}
#if AQUA_DEBUG
				++logger.m_logMessagesWritten; // will be written later
#endif // AQUA_DEBUG

			_Packet& packet = logger.m_packetBuffer[writeIndex % Config::Logger::MAX_PACKETS_AT_TIME];
			packet.timePoint = timePoint;
			_CreatePacketInPlace(packet, format, level, args...);

			logger.m_writeIndex.store(writeIndex + 1, std::memory_order_release);
		}

		template <size_t Size, typename ... Types>
		static void LogFormat(Level level, StringLiteral<Size> format, const Types& ... args) {
			static_assert(Size <= String256::GetBufferSize() + 1,
				"aqua::Logger::LogFormat - format string is too long");

			aqua::String256 format256 = format;
			LogFormat(level, format256, args...);
		}

	private:
		struct _Packet {
		public:
			enum ArgumentType : uint16_t {
				LL,
				ULL,
				DOUBLE,
				STRING
			}; // enum ArgumentType

			union Argument {
				Argument() : ll(0) {};

				template <std::integral T>
				inline Argument& operator=(T x) { ll = x; return *this; }
				inline Argument& operator=(unsigned long long x) { ull = x; return *this; }
				inline Argument& operator=(double x) { dbl = x; return *this; }
				inline Argument& operator=(const StringLiteralPointer& s) { str = s; return *this; }

				long long		     ll;
				unsigned long long   ull;
				double               dbl;
				StringLiteralPointer str;
			};

		public:
			std::chrono::system_clock::time_point timePoint;

			uint8_t						  level;
			uint8_t						  argCount;
			uint16_t					  argTypes;
			StringBuffer<char, 257, '\n'> formatString;
			Argument					  args[Config::Logger::MAX_FORMAT_ARGUMENTS];
		}; // struct _Packet

		template <std::integral T>
		static uint16_t _ForwardType(T) noexcept {
			return _Packet::ArgumentType::LL;
		}
		static uint16_t _ForwardType(unsigned long long) noexcept {
			return _Packet::ArgumentType::ULL;
		}

		template <std::floating_point T>
		static uint16_t _ForwardType(T) noexcept {
			return _Packet::ArgumentType::DOUBLE;
		}

		template <size_t Size>
		static uint16_t _ForwardType(StringLiteral<Size>) noexcept {
			return _Packet::ArgumentType::STRING;
		}
		static uint16_t _ForwardType(const StringLiteralPointer&) noexcept {
			return _Packet::ArgumentType::STRING;
		}

		template <std::integral T>
		static long long _ForwardArg(T v) noexcept {
			return static_cast<long long>(v);
		}
		static long long _ForwardArg(unsigned long long v) noexcept { return v; }

		template <std::floating_point T>
		static double _ForwardArg(T v) noexcept {
			return static_cast<double>(v);
		}

		template <size_t Size>
		static StringLiteralPointer _ForwardArg(StringLiteral<Size> literal) noexcept {
			return StringLiteralPointer(literal);
		}
		static StringLiteralPointer _ForwardArg(const StringLiteralPointer& literal) noexcept {
			return literal;
		}

		template <typename ... Types>
		static void _CreatePacketInPlace(
		_Packet& pkt, const aqua::String256& fmt, Level lvl, const Types& ... args) {
			pkt.level = (uint8_t)lvl;
			pkt.argCount = sizeof...(args);

			if constexpr (sizeof...(args) > 0) {
				uint8_t argIndex = 0;
				pkt.argTypes = ((_ForwardType(args) << (argIndex++ << 1)) | ...);

				argIndex = 0;
				((pkt.args[argIndex++] = _ForwardArg(args)), ...);
			}
			pkt.formatString.Clear();
			pkt.formatString.Push((char)lvl);
			pkt.formatString.Append(fmt);
		}

	private:
		int _WriteArg(size_t where, _Packet::ArgumentType type, const _Packet::Argument& arg) noexcept;
		int _WriteTime(size_t where, std::chrono::system_clock::time_point timePoint) noexcept;

	private:
		friend class Engine;
		Logger(
			const Config& config,
			Status& status
		);

	private:
		const Config&		       m_config;
		UniqueData<ILoggerProcess> m_loggerProcess;

	#if AQUA_DEBUG
		size_t                     m_logMessagesRequested = 0;
		size_t                     m_logMessagesWritten   = 0;
	#endif // AQUA_DEBUG

		_Packet					   m_packetBuffer[Config::Logger::MAX_PACKETS_AT_TIME] = {};
		char					   m_buffer[Config::Logger::MAX_MESSAGE_LENGTH + 1] = {};

		std::atomic<size_t>		   m_writeIndex{ 0 };
		std::atomic<size_t>		   m_readIndex{ 0 };
		std::atomic<bool>		   m_isRunning{ false };
		std::thread				   m_logThread;

		static Logger* s_Logger;
		static uint8_t s_DelayedMessageCount;
		static _Packet s_DelayedMessages[Config::Logger::MAX_DELAYED_MESSAGES];
	}; // class Logger
} // namespace aqua

#define AQUA_LOG_FATAL_ERROR(format, ...) \
	aqua::Logger::LogFormat(aqua::Logger::Level::FATAL, format, __VA_ARGS__)

#if AQUA_BUILD_TYPE_ENABLE_WARNINGS
	#define AQUA_LOG_WARNING(format, ...) \
		aqua::Logger::LogFormat(aqua::Logger::Level::WARNING, format, __VA_ARGS__);

	#define AQUA_LOG_WARNING_IF(condition, format, ...) \
		if (condition) {						   \
			AQUA_LOG_WARNING(format, __VA_ARGS__); \
		}
#else
	#define AQUA_LOG_WARNING(format, ...)
#endif // AQUA_BUILD_TYPE_ENABLE_WARNINGS

#if AQUA_BUILD_TYPE_ENABLE_LOG_INFO
	#define AQUA_LOG(format, ...) \
		aqua::Logger::LogFormat(aqua::Logger::Level::INFO, format, __VA_ARGS__)
#else
	#define AQUA_LOG(format, ...)
#endif // AQUA_BUILD_TYPE_ENABLE_LOG_INFO

#if AQUA_BUILD_TYPE_ENABLE_LOG_ALLOCATIONS
	#define AQUA_LOG_MEMORY(format, ...) \
		aqua::Logger::LogFormat(aqua::Logger::Level::MEMORY, format, __VA_ARGS__)

	#define AQUA_LOG_MEMORY_IF(condition, format, ...) \
		if (condition) {					     \
			AQUA_LOG_MEMORY(format, __VA_ARGS__); \
		}
#endif // AQUA_BUILD_TYPE_ENABLE_LOG_ALLOCATIONS

#endif // !AQUA_LOGGER_HEADER