#ifndef AQUA_TEMPLATE_UTILITY_HEADER
#define AQUA_TEMPLATE_UTILITY_HEADER

#include <type_traits>

namespace aqua {
	template <typename Type, typename CompressedType>
	requires (std::is_empty_v<CompressedType> && !std::is_final_v<CompressedType>)
	class CompressedPair : public CompressedType {
	public:
		using ValueType			  = Type;
		using CompressedValueType = CompressedType;

	public:
		CompressedPair() : CompressedType(), value() {}

		CompressedPair(ValueType&& value, CompressedType&& compressed) :
			CompressedType(std::forward<CompressedType>(compressed)),
			value(std::forward<ValueType>(value)) {}

		~CompressedPair() = default;

	public:
		CompressedPair&		  GetCompressed()		noexcept { return *this; }
		const CompressedPair& GetCompressed() const noexcept { return *this; }

	public:
		Type value;
	};
} // namespace aqua

#endif // !AQUA_TEMPLATE_UTILITY_HEADER