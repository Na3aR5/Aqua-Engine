#ifndef AQUA_MATH_RANDOM_HEADER
#define AQUA_MATH_RANDOM_HEADER

#include <cstdlib>
#include <aqua/datastructures/StringBuffer.h>

namespace aqua {
	class Random {
	public:
		Random();

	public:
		// std::rand() in [min, max]
		// No range validation
		// Maximum 'max' value = RAND_MAX (usually 0x7fff)
		static int SimpleRandomNumber(unsigned min, unsigned max);

		// Used in case, when user did not specify application name
		static StringBuffer<char, 15> GenerateApplicationName();
	}; // class Random
} // namespace aqua

#endif // !AQUA_MATH_RANDOM_HEADER