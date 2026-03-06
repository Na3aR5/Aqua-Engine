#ifndef AQUA_DEBUG_HEADER
#define AQUA_DEBUG_HEADER

#include <aqua/Build.h>

#if AQUA_DEBUG
	#ifdef _MSC_VER
		#define AQUA_DEBUG_BREAK __debugbreak()
	#elif defined(__GNUC__) || defined(__clang__)
		#define AQUA_DEBUG_BREAK __builtin_trap()
	#endif // _MSC_VER
#else
	#define AQUA_DEBUG_BREAK
#endif // AQUA_DEBUG

#define AQUA_DEBUG_CRASH_REPORT_AFTER_LOG AQUA_DEBUG_BREAK

#endif // !AQUA_DEBUG_HEADER