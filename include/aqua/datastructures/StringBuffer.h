#ifndef AQUA_STRING_BUFFER_HEADER
#define AQUA_STRING_BUFFER_HEADER

#include <aqua/Error.h>
#include <aqua/utility/StringLiteral.h>

#include <cstdlib>
#include <iterator>

namespace aqua {
	namespace _string {
		template <typename T>
		concept CharConcept = (
			std::is_same_v<T, char> ||
			std::is_same_v<T, wchar_t> ||
			std::is_same_v<T, signed char> ||
			std::is_same_v<T, unsigned char> ||
			std::is_same_v<T, char16_t> ||
			std::is_same_v<T, char32_t>
		#ifdef __cpp_char8_t
			|| std::is_same_v<T, char8_t>
		#endif // __cpp_char8_t
		); // concept CharConcept

		template <typename T>
		class _StringBase {
		protected:
			static size_t _CStrLength(const T* cstr) noexcept {
				size_t size = 0;
				for (; cstr[size] != T(0); ++size);
				return size;
			}

			static void _CopyBackwards(T* rbeginDest, T* rbeginSrc, size_t count) noexcept {
				for (size_t i = 0; i < count; ++i) {
					*(rbeginDest--) = *(rbeginSrc--);
				}
			}
		}; // class _StringBase
	} // namespace _string

	template <_string::CharConcept T, size_t BufferSize, T EOFchar = T(0)>
	class StringBuffer : public _string::_StringBase<T> {
	public:
		using BaseType = _string::_StringBase<T>;

		using ValueType		 = T;
		using Pointer		 = ValueType*;
		using Reference	     = ValueType&;
		using ConstPointer   = const ValueType*;
		using ConstReference = const ValueType&;

		using Iterator			   = Pointer;
		using ConstIterator		   = ConstPointer;
		using ReverseIterator	   = std::reverse_iterator<Iterator>;
		using ConstReverseIterator = std::reverse_iterator<ConstIterator>;

	public:
		StringBuffer() { m_buffer[0] = EOFchar; }

		template <size_t Size>
		StringBuffer(StringLiteral<Size> literal) noexcept requires(Size <= BufferSize + 1) : m_size(Size - 1) {
			std::memcpy(m_buffer, StringLiteralPointer(literal).GetPtr(), Size);
			m_buffer[Size] = EOFchar;
		}

	public:
		Reference	   operator[](size_t index) { return m_buffer[index]; }
		ConstReference operator[](size_t index) const { return m_buffer[index]; }

		bool operator==(const StringBuffer& other) const noexcept {
			if (m_size != other.m_size) {
				return false;
			}
			size_t i = 0;
			for (; i < m_size && i < other.m_size; ++i) {
				if (m_buffer[i] != other.m_buffer[i]) {
					return false;
				}
			}
			return true;
		}

		bool operator!=(const StringBuffer& other) const noexcept {
			return !(*this == other);
		}

		// Set() method overloads
	public:
		void Set(const StringBuffer& other) noexcept {
			if (this == &other) {
				return;
			}
			m_size = other.m_size;
			if (m_size == 0) {
				m_buffer[0] = EOFchar;
				return;
			}
			std::memcpy(m_buffer, other.m_buffer, sizeof(ValueType) * (m_size + 1));
		}

		template <size_t OtherBufferSize, T OtherEOFchar>
		void Set(const StringBuffer<T, OtherBufferSize, OtherEOFchar>& other) noexcept requires(OtherBufferSize < BufferSize) {
			m_size = other.GetSize();
			if (m_size == 0) {
				m_buffer[0] = EOFchar;
				return;
			}
			std::memcpy(m_buffer, other.GetPtr(), sizeof(ValueType) * m_size);
			m_buffer[m_size] = EOFchar;
		}

		bool Set(ConstPointer buffer, size_t count) noexcept {
			if (count > BufferSize) {
				return false;
			}
			m_size = count;
			std::memcpy(m_buffer, buffer, sizeof(ValueType) * count);
			m_buffer[m_size] = EOFchar;

			return true;
		}

		// Cut if 'cstr' length is too large and return false
		bool Set(ConstPointer cstr) noexcept {
			if (m_buffer == cstr) {
				return true;
			}
			size_t cstrSize;
			if (cstr == nullptr || (cstrSize = this->_CStrLength(cstr)) == 0) {
				m_size = 0;
				m_buffer[0] = EOFchar;
				return true;
			}
			if (cstrSize > BufferSize) {
				m_size = BufferSize;
				std::memcpy(m_buffer, cstr, sizeof(ValueType) * m_size);
				m_buffer[m_size] = EOFchar;
				return false;
			}
			m_size = cstrSize;
			std::memcpy(m_buffer, cstr, sizeof(ValueType) * m_size);
			m_buffer[m_size] = EOFchar;

			return true;
		}

		bool Set(size_t count, ValueType value) noexcept {
			if (count > BufferSize) {
				return false;
			}
			m_size = count;
			std::fill(m_buffer, m_buffer + m_size, value);
			m_buffer[m_size] = EOFchar;

			return true;
		}

		template <std::forward_iterator Iterator>
		bool Set(Iterator rangeBegin, Iterator rangeEnd) noexcept {
			size_t rangeSize = std::distance(rangeBegin, rangeEnd);
			if (rangeSize > BufferSize) {
				return false;
			}
			m_size = rangeSize;

			size_t i = 0;
			while (rangeBegin != rangeEnd) {
				m_buffer[i] = static_cast<ValueType>(*rangeBegin);
				++i, ++rangeBegin;
			}
			m_buffer[m_size] = EOFchar;

			return true;
		}

	public:
		bool IsEmpty() const noexcept { return m_size == 0; }

		size_t GetSize()   const noexcept { return m_size; }
		size_t GetLength() const noexcept { return m_size; }

		Pointer		 GetPtr()		noexcept { return m_buffer; }
		ConstPointer GetPtr() const noexcept { return m_buffer; }

		static constexpr size_t GetBufferSize() noexcept { return BufferSize; }

		// Append() method overloads
	public:
		bool Append(const StringBuffer& other) noexcept {
			if (other.IsEmpty()) {
				return true;
			}
			size_t newSize = m_size + other.m_size;
			if (newSize > BufferSize) {
				return false;
			}
			std::memcpy(m_buffer + m_size, other.m_buffer, sizeof(ValueType) * (other.m_size + 1));
			m_size = newSize;

			return true;
		}

		template <size_t OtherBufferSize, T OtherEOFchar>
		bool Append(const StringBuffer<T, OtherBufferSize, OtherEOFchar>& other) noexcept requires(OtherBufferSize <= BufferSize) {
			if (other.IsEmpty()) {
				return true;
			}
			size_t newSize = m_size + other.GetSize();
			if (newSize > BufferSize) {
				return false;
			}
			std::memcpy(m_buffer + m_size, other.GetPtr(), other.GetSize());
			m_size = newSize;
			m_buffer[m_size] = EOFchar;

			return true;
		}

		bool Append(ConstPointer cstr) noexcept {
			if (m_buffer == cstr) {
				return Append(*this);
			}
			size_t cstrSize;
			if (cstr == nullptr || (cstrSize = this->_CStrLength(cstr)) == 0) {
				return true;
			}
			size_t newSize = m_size + cstrSize;
			if (newSize > BufferSize) {
				return false;
			}
			std::memcpy(m_buffer + m_size, cstr, sizeof(ValueType) * cstrSize);
			m_size = newSize;
			m_buffer[m_size] = EOFchar;

			return true;
		}

		bool Append(size_t count, ValueType value) noexcept {
			if (count == 0) {
				return true;
			}
			size_t newSize = m_size + count;
			if (newSize > BufferSize) {
				return false;
			}
			std::fill(m_buffer + m_size, m_buffer + newSize, value);
			m_size = newSize;
			m_buffer[m_size] = EOFchar;

			return true;
		}

		template <std::forward_iterator Iterator>
		bool Append(Iterator rangeBegin, Iterator rangeEnd) noexcept {
			size_t rangeSize = std::distance(rangeBegin, rangeEnd);
			if (rangeSize == 0) {
				return true;
			}
			size_t newSize = m_size + rangeSize;
			if (newSize > BufferSize) {
				return false;
			}
			size_t i = m_size;
			while (rangeBegin != rangeEnd) {
				m_buffer[i] = static_cast<ValueType>(*rangeBegin);
				++i, ++rangeBegin;
			}
			m_size = newSize;
			m_buffer[newSize] = EOFchar;

			return true;
		}

		// 'Add' methods
	public:
		bool Push(ValueType value) noexcept {
			if (m_size + 1 > BufferSize) {
				return false;
			}
			m_buffer[m_size] = value;
			m_buffer[++m_size] = EOFchar;
			return true;
		}

		bool Insert(size_t where, size_t count, ValueType value) noexcept {
			if (where >= m_size) {
				return false;
			}
			if (count == 0) {
				return true;
			}
			size_t newSize = m_size + count;
			if (newSize > BufferSize) {
				return false;
			}
			this->_CopyBackwards(m_buffer + newSize, m_buffer + m_size, m_size - where + 1);
			std::fill(m_buffer + where, m_buffer + (where + count), value);

			return true;
		}

		bool Insert(size_t where, ValueType value) noexcept {
			return Insert(where, 1, value);
		}

		bool Insert(size_t where, const StringBuffer& other) noexcept {
			if (where >= m_size) {
				return false;
			}
			if (other.m_size == 0) {
				return true;
			}
			size_t newSize = m_size + other.m_size;
			if (newSize > BufferSize) {
				return false;
			}
			this->_CopyBackwards(m_buffer + newSize, m_buffer + m_size, m_size - where + 1);
			std::memcpy(m_buffer + where, other.m_buffer, sizeof(ValueType) * other.m_size);

			return true;
		}

		bool Insert(size_t where, ConstPointer cstr) noexcept {
			if (where >= m_size) {
				return false;
			}
			size_t cstrSize;
			if (cstr == nullptr || (cstrSize = this->_CStrLength(cstr)) == 0) {
				return true;
			}
			size_t newSize = m_size + cstrSize;
			if (newSize > BufferSize) {
				return false;
			}
			this->_CopyBackwards(m_buffer + newSize, m_buffer + m_size, m_size - where + 1);
			std::memcpy(m_buffer + where, cstr, sizeof(ValueType) * cstrSize);

			return true;
		}

		template <std::forward_iterator Iterator>
		bool Insert(size_t where, Iterator rangeBegin, Iterator rangeEnd) noexcept {
			if (where >= m_size) {
				return false;
			}
			size_t rangeSize = std::distance(rangeBegin, rangeEnd);
			if (rangeSize == 0) {
				return true;
			}
			size_t newSize = m_size + rangeSize;
			this->_CopyBackwards(m_buffer + newSize, m_buffer + m_size, m_size - where + 1);

			while (rangeBegin != rangeEnd) {
				m_buffer[where++] = static_cast<ValueType>(*rangeBegin);
				++rangeBegin;
			}
			return true;
		}

		// 'Remove' methods
	public:
		void Clear() noexcept { m_size = 0; m_buffer[m_size] = EOFchar; }

		void Pop() noexcept {
			m_buffer[--m_size] = EOFchar;
		}

		void Pop(size_t count) noexcept {
			m_size -= count;
			m_buffer[m_size] = EOFchar;
		}

		bool Remove(size_t where, size_t count) noexcept {
			if (where >= m_size) {
				return false;
			}
			std::memcpy(m_buffer + where, m_buffer + (where + count), m_size - (where + count) + 1);
			return true;
		}

		// Move EOF character into the 'size' index
		// Notice: previous EOF character won't be moved
		// This method is useful when you write in buffer directly, like: std::sprintf(strBuf.GetPtr(), ...)
		// then you need to specify new size by calling this method
		// Return false if 'size' is too large
		bool SetSize(size_t size) noexcept {
			if (size >= BufferSize) {
				return false;
			}
			m_size = size;
			m_buffer[size] = EOFchar;
			return true;
		}

	public:
		Iterator      begin()        noexcept { return Iterator(m_buffer); }
		ConstIterator begin()  const noexcept { return ConstIterator(m_buffer); }
		ConstIterator cbegin() const noexcept { return ConstIterator(m_buffer); }

		Iterator      end()        noexcept { return Iterator(m_buffer + m_size); }
		ConstIterator end()  const noexcept { return ConstIterator(m_buffer + m_size); }
		ConstIterator cend() const noexcept { return ConstIterator(m_buffer + m_size); }

		ReverseIterator      rbegin()        noexcept { return ReverseIterator(end()); }
		ConstReverseIterator rbegin()  const noexcept { return ConstReverseIterator(end()); }
		ConstReverseIterator crbegin() const noexcept { return ConstReverseIterator(end()); }

		ReverseIterator      rend()        noexcept { return ReverseIterator(begin()); }
		ConstReverseIterator rend()  const noexcept { return ConstReverseIterator(begin()); }
		ConstReverseIterator crend() const noexcept { return ConstReverseIterator(begin()); }

	private:
		bool _IsValidIterator(ConstIterator it) const noexcept {
			return it >= cbegin() && it <= cend();
		}

	private:
		size_t    m_size = 0;
		ValueType m_buffer[BufferSize + 1];
	}; // class StringBuffer

	using String128 = StringBuffer<char, 127>;
	using String256	= StringBuffer<char, 255>;
} // namespace aqua

#endif // !AQUA_STRING_BUFFER_HEADER