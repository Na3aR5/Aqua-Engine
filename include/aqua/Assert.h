#ifndef AQUA_ASSERT_HEADER
#define AQUA_ASSERT_HEADER

#include <aqua/Build.h>
#include <aqua/CrashReport.h>

#include <cstdlib>

#if AQUA_DEBUG
	#define AQUA_ASSERT(condition, message) \
		do {                                \
			if (!(condition)) [[unlikely]] {\
				aqua::CrashReport(message, aqua::Literal(__FILE__), aqua::Literal(__func__), __LINE__); \
				std::abort();               \
			}                               \
		} while (false)

	#define AQUA_VERIFY(expression, message) AQUA_ASSERT(expression, message)
#else
	#define AQUA_ASSERT(condition, message)

	#define AQUA_VERIFY(expression, message) (expression)
#endif // AQUA_DEBUG

#endif // !AQUA_ASSERT_HEADER