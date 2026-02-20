#ifndef AQUA_STRING_LITERAL_HEADER
#define AQUA_STRING_LITERAL_HEADER

namespace aqua {
	class StringLiteralPointer;

	template <size_t Size>
	class StringLiteral {
	public:
		friend class StringLiteralPointer;
		constexpr StringLiteral(const char(&string)[Size]) noexcept : m_string(string) {}

	public:
		char operator[](size_t index) const { return m_string[index]; }

	public:
		static constexpr size_t GetSize()   noexcept { return Size - 1; }
		static constexpr size_t GetLength() noexcept { return Size - 1; }
		const char* GetPtr() const noexcept { return m_string; }

	private:
		const char(&m_string)[Size];
	}; // class StringLiteral

	// const char*
	class StringLiteralPointer {
	public:
		StringLiteralPointer() = default;

		template <size_t Size>
		constexpr StringLiteralPointer(StringLiteral<Size> literal) noexcept :
			m_size(Size - 1), m_literal(literal.m_string) {}

	public:
		size_t      GetSize() const noexcept { return m_size; }
		const char* GetPtr()  const noexcept { return m_literal; }

	private:
		size_t      m_size	  = 0;
		const char* m_literal = nullptr;
	};

	template <size_t Size>
	StringLiteral<Size> Literal(const char(&string)[Size]) {
		return StringLiteral<Size>(string);
	}
} // namespace aqua

#endif // !AQUA_STRING_LITERAL_HEADER