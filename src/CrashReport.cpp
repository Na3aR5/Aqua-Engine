#include <aqua/CrashReport.h>
#include <aqua/Logger.h>

namespace {
	aqua::String256 g_FormatString;
}

void aqua::CrashReport(
const String128& message,
const StringLiteralPointer& file,
const StringLiteralPointer& func,
uint64_t line) {
	g_FormatString.Set(message);
	g_FormatString.Append(" (crashed in {}, in {} function, at {} line)");

	AQUA_LOG_FATAL_ERROR(g_FormatString, file, func, line);
}

void aqua::__CrashReport(
const StringLiteralPointer& message,
const StringLiteralPointer& file,
const StringLiteralPointer& func,
uint64_t line) {
	g_FormatString.Set(message.GetPtr());
	g_FormatString.Append(" (crashed in {}, in {} function, at {} line)");

	AQUA_LOG_FATAL_ERROR(g_FormatString, file, func, line);
}