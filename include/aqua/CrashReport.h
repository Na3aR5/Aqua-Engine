#ifndef AQUA_CRASH_REPORT_HEADER
#define AQUA_CRASH_REPORT_HEADER

#include <aqua/utility/StringLiteral.h>
#include <aqua/datastructures/StringBuffer.h>

#include <cstdint>

namespace aqua {
	void CrashReport(
		const String128& message,
		const StringLiteralPointer& file,
		const StringLiteralPointer& func,
		uint64_t line
	);

	void __CrashReport( // helper
		const StringLiteralPointer& message,
		const StringLiteralPointer& file,
		const StringLiteralPointer& func,
		uint64_t line
	);

	template <size_t MsgSize, size_t FileSize, size_t FuncSize>
	void CrashReport(StringLiteral<MsgSize> message, StringLiteral<FileSize> file,
	StringLiteral<FuncSize> func, uint64_t line) requires (MsgSize <= String128::GetBufferSize() + 1) {
		__CrashReport(message, file, func, line);
	}
} // namespace aqua

#endif // !AQUA_CRASH_REPORT_HEADER