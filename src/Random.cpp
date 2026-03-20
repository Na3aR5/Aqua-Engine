#include <aqua/pch.h>
#include <aqua/math/Random.h>

namespace {
	aqua::Random g_Random;
}

aqua::Random::Random() {
	std::srand(static_cast<unsigned>(std::time(nullptr)));
}

int aqua::Random::SimpleRandomNumber(unsigned min, unsigned max) {
	int x = std::rand();
	return min + (x % (max - min + 1));
}

aqua::StringBuffer<char, 15> aqua::Random::GenerateApplicationName() {
	aqua::StringBuffer<char, 15> name;
	int rangeMin = (int)'a';
	int rangeMax = (int)'z';

	int nameLength = SimpleRandomNumber(7, 15);
	for (int i = 0; i < nameLength; ++i) {
		name.Push((char)SimpleRandomNumber(rangeMin, rangeMax));
	}
	return name;
}