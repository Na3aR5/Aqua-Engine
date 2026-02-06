#ifndef AQUA_MATH_VECTOR_HEADER
#define AQUA_MATH_VECTOR_HEADER

#include <type_traits>

namespace aqua {
	template <typename T>
	struct Vec2 {
	public:
		static_assert(std::is_arithmetic_v<T>, "aqua::VecN - value type must be arithmetic");

	public:
		constexpr Vec2() = default;
		explicit constexpr Vec2(T value) : x(value), y(value) {}
		constexpr Vec2(T x, T y) : x(x), y(y) {}
		
		template <typename U>
		constexpr Vec2(const Vec2<U>& other) : x(U(other.x)), y(U(other.y)) {}

	public:
		T x = T(0);
		T y = T(0);
	}; // struct Vec2

	template <typename T>
	Vec2<T>& operator+=(Vec2<T>& left, const Vec2<T>& right) {
		left.x += right.x;
		left.y += right.y;
		return left;
	}

	template <typename T>
	Vec2<T> operator+(const Vec2<T>& left, const Vec2<T>& right) {
		Vec2<T> result = left;
		return result += right;
	}

	template <typename T>
	Vec2<T>& operator-=(Vec2<T>& left, const Vec2<T>& right) {
		left.x -= right.x;
		left.y -= right.y;
		return left;
	}

	template <typename T>
	Vec2<T> operator-(const Vec2<T>& left, const Vec2<T>& right) {
		Vec2<T> result = left;
		return result -= right;
	}

	template <typename T>
	Vec2<T>& operator*=(Vec2<T>& left, T value) {
		left.x *= value;
		left.y *= value;
		return left;
	}

	template <typename T>
	Vec2<T> operator*(const Vec2<T>& left, T value) {
		Vec2<T> result = left;
		return result *= value;
	}

	template <typename T>
	Vec2<T>& operator/=(Vec2<T>& left, T value) {
		left.x /= value;
		left.y /= value;
		return left;
	}

	template <typename T>
	Vec2<T> operator/(const Vec2<T>& left, T value) {
		Vec2<T> result = left;
		return result /= value;
	}

	using Vec2i = Vec2<int>;
	using Vec2u = Vec2<unsigned int>;
	using Vec2f = Vec2<float>;
	using Vec2d = Vec2<double>;
} // namespace aqua

#endif // !AQUA_MATH_VECTOR_HEADER
