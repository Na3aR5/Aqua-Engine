#include <aqua/pch.h>
#include <aqua/datastructures/String.h>

aqua::StringBuffer<char, 20> aqua::ToString(unsigned long long x) noexcept {
	StringBuffer<char, 20> result;

	if (x == 0) {
		result.Push('0');
		return result;
	}
	char buffer[20];

	int8_t i = 19;
	while (x > 0) {
		buffer[i--] = (char)('0' + (x % 10));
		x /= 10;
	}

	result.Set(buffer + i + 1, 19 - i);
	return result;
}