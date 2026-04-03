#ifndef AQUA_STRING_HEADER
#define AQUA_STRING_HEADER

#include <aqua/utility/Memory.h>
#include <aqua/datastructures/StringBuffer.h>

#include <cstdint>
#include <concepts>

namespace aqua {
	// Nothrow string
	template <_string::CharConcept T, typename Allocator>
	class BasicSafeString : public _string::_StringBase<T> {
	public:
		using BaseType = _string::_StringBase<T>;

		using AllocatorType  = Allocator;

		using ValueType		 = T;
		using Pointer		 = typename AllocatorType::Pointer;
		using Reference      = ValueType&;
		using ConstPointer   = typename AllocatorType::ConstPointer;
		using ConstReference = const ValueType&;

		using Iterator             = Pointer;
		using ConstIterator		   = ConstPointer;
		using ReverseIterator	   = std::reverse_iterator<Iterator>;
		using ConstReverseIterator = std::reverse_iterator<ConstIterator>;

	public:
		BasicSafeString() noexcept requires(std::is_nothrow_default_constructible_v<Allocator>) : m_pair() {}

		BasicSafeString(BasicSafeString&& other) noexcept requires(std::is_nothrow_move_constructible_v<Allocator>) :
		m_pair(other.m_pair.value, std::move(other.m_pair.GetAllocator())) {
			other.m_pair.value = _ThisData();
		}

		BasicSafeString(const AllocatorType& allocator) noexcept requires(std::is_nothrow_copy_constructible_v<Allocator>) :
		m_pair(_ThisData(), allocator) {}

		BasicSafeString(AllocatorType&& allocator) noexcept requires(std::is_nothrow_move_constructible_v<Allocator>) :
		m_pair(_ThisData(), std::move(allocator)) { }

		BasicSafeString(const BasicSafeString&) = delete;

		~BasicSafeString() { _ThisDeallocate(); }

	public:
		BasicSafeString& operator=(BasicSafeString&& other) noexcept {
			_ThisDeallocate();

			m_pair.value = other.m_pair.value;
			m_pair.GetAllocator() = std::move(other.m_pair.GetAllocator());
			other.m_pair.value = _ThisData();

			return *this;
		}

		BasicSafeString& operator=(const BasicSafeString&) = delete;

		Reference      operator[](size_t index)		  { return m_pair.value.first[index]; }
		ConstReference operator[](size_t index) const { return m_pair.value.first[index]; }

		bool operator==(const BasicSafeString& other) const noexcept {
			size_t thisSize  = GetSize();
			size_t otherSize = other.GetSize();

			if (thisSize != otherSize) {
				return false;
			}
			size_t i = 0;
			for (; i < thisSize && i < otherSize; ++i) {
				if (m_pair.value.first[i] != other.m_pair.value.first[i]) {
					return false;
				}
			}
			return true;
		}

		bool operator!=(const BasicSafeString& other) const noexcept {
			return !(*this == other);
		}

	public:
		bool IsEmpty() const noexcept {
			return m_pair.value.first == m_pair.value.last;
		}

		size_t GetSize() const noexcept {
			return m_pair.value.first == nullptr ? 0 : m_pair.value.last - m_pair.value.first;
		}

		size_t GetLength() const noexcept {
			return GetSize();
		}

		size_t GetCapacity() const noexcept {
			return m_pair.value.first == nullptr ? 0 : m_pair.value.end - m_pair.value.first;
		}

		Pointer GetPtr() noexcept { return m_pair.value.first; }
		ConstPointer GetPtr() const noexcept { return m_pair.value.first; }

	// Set() method overloads
	public:
		AQUA_NODISCARD Status Set(const BasicSafeString& other) noexcept {
			if (this == &other) {
				return Success{};
			}
			if (other.IsEmpty()) {
				_ThisDeallocate();
				return Success{};
			}
			size_t otherSize     = other.GetSize();
			size_t otherCapacity = other.GetCapacity();

			AQUA_TRY(other._CopyAllocator(), allocator);
			AQUA_TRY(_Allocate(otherCapacity + 1), ptr);

			_ThisDeallocate();

			m_pair.value.first = ptr.GetValue();
			std::memcpy(m_pair.value.first, other.m_pair.value.first, sizeof(ValueType) * (other.GetSize() + 1));
			m_pair.value.last = m_pair.value.first + otherSize;
			m_pair.value.end  = m_pair.value.first + otherCapacity;
			m_pair.GetAllocator() = std::move(allocator.GetValue());

			return Success{};
		}

		AQUA_NODISCARD Status Set(const ValueType* cstr) noexcept {
			if (m_pair.value.first == cstr) {
				return Success{};
			}
			size_t cstrSize;
			if (cstr == nullptr || (cstrSize = this->_CStrLength(cstr)) == 0) {
				_ThisDeallocate();
				return Success{};
			}
			size_t capacity = _SizeToCapacity(cstrSize);
			AQUA_TRY(_Allocate(capacity + 1), ptr);

			_ThisDeallocate();

			m_pair.value.first = ptr.GetValue();
			std::memcpy(m_pair.value.first, cstr, sizeof(ValueType) * (cstrSize + 1));
			m_pair.value.last = m_pair.value.first + cstrSize;
			m_pair.value.end  = m_pair.value.first + capacity;

			return Success{};
		}

		AQUA_NODISCARD Status Set(const ValueType* cstr, size_t count) noexcept {
			if (cstr == nullptr && count > 0) {
				return Error::INPUT_ARGUMENTS_ARE_INVALID;
			}
			if (count == 0) {
				_ThisDeallocate();
				return Success{};
			}
			size_t capacity = _SizeToCapacity(count);
			AQUA_TRY(_Allocate(capacity + 1), ptr);

			_ThisDeallocate();

			m_pair.value.first = ptr.GetValue();
			std::memcpy(m_pair.value.first, cstr, sizeof(ValueType) * count);
			m_pair.value.last = m_pair.value.first + count;
			m_pair.value.end = m_pair.value.first + capacity;
			*m_pair.value.last = ValueType(0);

			return Success{};
		}

		// Set new value to the string as 'count' times repeated character 'value'
		AQUA_NODISCARD Status Set(size_t count, ValueType value) noexcept {
			if (count == 0) {
				_ThisDeallocate();
				return Success{};
			}
			size_t capacity = _SizeToCapacity(count);
			AQUA_TRY(_Allocate(capacity + 1), ptr);

			_ThisDeallocate();

			m_pair.value = ptr.GetValue();
			std::fill(m_pair.value, m_pair.value + count, value);
			m_pair.value.last  = m_pair.value.first + count;
			m_pair.value.end   = m_pair.value.first + capacity;
			*m_pair.value.last = ValueType(0);

			return Success{};
		}

		// Set new value to the string by copying range ['begin, 'end') of elements
		template <std::forward_iterator Iterator>
		AQUA_NODISCARD Status Set(Iterator begin, Iterator end) noexcept {
			size_t size = std::distance(begin, end);
			if (size == 0) {
				_ThisDeallocate();
				return Success{};
			}
			size_t capacity = _SizeToCapacity(size);
			AQUA_TRY(_Allocate(capacity + 1), ptr);

			_ThisDeallocate();

			Pointer dest = ptr.GetValue();
			while (begin != end) {
				*dest = static_cast<ValueType>(*begin);
				++dest, ++begin;
			}
			m_pair.value.first = ptr.GetValue();
			m_pair.value.last  = m_pair.value.first + size;
			m_pair.value.end   = m_pair.value.first + capacity;
			*m_pair.value.last = ValueType(0);

			return Success{};
		}

	// Append() method overloads
	public:
		// Add 'other' string to the end of this string (can add itself)
		AQUA_NODISCARD Status Append(const BasicSafeString& other) noexcept {
			if (other.IsEmpty()) {
				return Success{};
			}
			return _AppendUnchecked(other);
		}

		// Add c-style string 'cstr' to the end (can add it's own pointer)
		// 'nullptr' is considered as an empty string
		AQUA_NODISCARD Status Append(ConstPointer cstr) noexcept {
			size_t cstrSize;
			if (cstr == nullptr || (cstrSize = this->_CStrLength(cstr)) == 0) {
				return Success{};
			}
			if (cstr == m_pair.value.first) {
				return Append(*this);
			}
			return _AppendUnchecked(cstr, cstrSize);
		}

		// Add 'count' characters 'value' to the end
		AQUA_NODISCARD Status Append(size_t count, ValueType value) noexcept {
			if (count == 0) {
				return Success{};
			}
			return _AppendUnchecked(count, value);
		}

		// Add range ['begin', 'end') of elements to the end by copy
		template <std::forward_iterator Iterator>
		AQUA_NODISCARD Status Append(Iterator begin, Iterator end) noexcept {
			size_t size = std::distance(begin, end);
			if (size == 0) {
				return Success{};
			}
			return _AppendUnchecked(begin, end, size);
		}

	// 'Add' methods
	public:
		// Add 'value' to the end
		AQUA_NODISCARD Status Push(ValueType value) noexcept {
			size_t newSize = GetSize() + 1;
			if (newSize > GetCapacity()) {
				AQUA_TRY(_ThisReallocate(_SizeToCapacity(newSize)));
			}
			*m_pair.value.last   = value;
			*++m_pair.value.last = ValueType(0);

			return Success{};
		}

		// Insert 'value' 'count' times starting from any valid position 'where' in the string
		AQUA_NODISCARD Status Insert(ConstIterator where, size_t count, ValueType value) noexcept {
			if (!_IsValidIterator(where)) {
				return Unexpected<Error>(Error::ITERATOR_OR_INDEX_OUT_OF_RANGE);
			}
			ConstIterator begin = cbegin();
			ConstIterator end   = cend();

			if (where == end) {
				return Append(count, value);
			}
			AQUA_TRY(_ThisMakeGap(GetSize() + count, where - begin, count));
			
			Pointer wherePtr = m_pair.value.first + (where - begin);
			std::fill(wherePtr, wherePtr + count, value);

			return Success{};
		}

		// Insert 'value' at any valid position 'where' in the string
		AQUA_NODISCARD Status Insert(ConstIterator where, ValueType value) noexcept {
			return Insert(where, 1, value);
		}

		// Insert 'other' string at any valid position 'where' in this string
		AQUA_NODISCARD Status Insert(
		ConstIterator where, const BasicSafeString& other) noexcept {
			if (!_IsValidIterator(where)) {
				return Unexpected<Error>(Error::ITERATOR_OR_INDEX_OUT_OF_RANGE);
			}
			ConstIterator begin = cbegin();
			ConstIterator end   = cend();

			if (where == end) {
				return Append(other);
			}
			size_t    otherSize = other.GetSize();
			ptrdiff_t gapIndex = where - begin;
			AQUA_TRY(_ThisMakeGap(GetSize() + otherSize, gapIndex, otherSize));

			std::memcpy(m_pair.value.first + gapIndex, other.m_pair.value.first, sizeof(ValueType) * otherSize);
			return Success{};
		}

		// Insert c-style string 'cstr' at any valid position 'where' in the string
		// 'nullptr' is considered as an empty string
		AQUA_NODISCARD Status Insert(ConstIterator where, ConstPointer cstr) noexcept {
			if (!_IsValidIterator(where)) {
				return Unexpected<Error>(Error::ITERATOR_OR_INDEX_OUT_OF_RANGE);
			}
			size_t cstrSize;
			if (cstr == nullptr || (cstrSize = this->_CStrLength(cstr)) == 0) {
				return Success{};
			}
			ConstIterator begin = cbegin();
			ConstIterator end   = cend();

			if (where == end) {
				return _AppendUnchecked(cstr, cstrSize);
			}
			ptrdiff_t gapIndex = where - begin;
			AQUA_TRY(_ThisMakeGap(GetSize() + cstrSize, gapIndex, cstrSize));

			std::memcpy(m_pair.value.first + gapIndex, cstr, sizeof(ValueType) * cstrSize);
			return Success{};
		}

		template <std::forward_iterator Iterator>
		AQUA_NODISCARD Status Insert(
		ConstIterator where, Iterator rangeBegin, Iterator rangeEnd) noexcept {
			if (!_IsValidIterator(where)) {
				return Unexpected<Error>(Error::ITERATOR_OR_INDEX_OUT_OF_RANGE);
			}
			size_t size = std::distance(rangeBegin, rangeEnd);
			if (size == 0) {
				return Success{};
			}
			ConstIterator begin = cbegin();
			ConstIterator end   = cend();

			if (where == end) {
				return _AppendUnchecked(rangeBegin, rangeEnd, size);
			}
			ptrdiff_t gapIndex = where - begin;
			AQUA_TRY(_ThisMakeGap(GetSize() + size, gapIndex, size));

			Pointer dest = m_pair.value.first + gapIndex;
			while (rangeBegin != rangeEnd) {
				*dest = static_cast<ValueType>(*rangeBegin);
				++dest, ++rangeBegin;
			}
			return Success{};
		}

	public:
		AQUA_NODISCARD Status Resize(size_t newSize, ValueType value = ValueType(0)) noexcept {
			size_t thisSize = GetSize();
			if (thisSize == newSize) {
				return Success{};
			}
			if (newSize > thisSize) {
				AQUA_TRY(_Reserve(newSize));
				std::memset(m_pair.value.last, (int)value, sizeof(ValueType) * (newSize - thisSize));
				m_pair.value.last += (newSize - thisSize);
			}
			else {
				m_pair.value.last = m_pair.value.first + newSize;
			}
			return Success{};
		}

	// 'Remove' methods
	public:
		// Remove all elements in the string, but do not deallocate memory (save capacity)
		void Clear() noexcept {
			if (!IsEmpty()) {
				*m_pair.value.first = ValueType(0);
			}
			m_pair.value.last = m_pair.value.first;
		}

		// Remove all elements in the string and deallocate memory
		void DeepClear() noexcept {
			_ThisDeallocate();
		}

		// Remove last element (assuming this.length > 0)
		void Pop() noexcept {
			--m_pair.value.last;
			*m_pair.value.last = ValueType(0);
		}
		
		// Remove 'count' elements from the end (assuming this.length >= 'count')
		void Pop(size_t count) noexcept {
			m_pair.value.last -= count;
			*m_pair.value.last = ValueType(0);
		}

		// Remove characters in any valid range ['rangeBegin, 'rangeEnd') in the string
		Status Remove(ConstIterator rangeBegin, ConstIterator rangeEnd) noexcept {
			if (!_IsValidIterator(rangeBegin) || !_IsValidIterator(rangeEnd)) {
				return Unexpected<Error>(Error::ITERATOR_OR_INDEX_OUT_OF_RANGE);
			}
			ConstIterator begin = cbegin();
			ConstIterator end   = cend();

			if (rangeBegin == rangeEnd) {
				return Success{};
			}
			ptrdiff_t rangeSize = rangeEnd - rangeBegin;
			if (rangeEnd == end) {
				Pop(rangeSize);
				return Success{};
			}
			Pointer beginPtr = m_pair.value.first + (rangeBegin - begin);
			std::memcpy(beginPtr, beginPtr + rangeSize,
				sizeof(ValueType) * ((m_pair.value.last - (beginPtr + rangeSize)) + 1));
			return Success{};
		}

		// Remove element at any valid position 'where' in the string
		Status Remove(ConstIterator where) noexcept {
			return Remove(where, where + 1);
		}

	public:
		Iterator      begin()        noexcept { return Iterator(m_pair.value.first); }
		ConstIterator begin()  const noexcept { return ConstIterator(m_pair.value.first); }
		ConstIterator cbegin() const noexcept { return ConstIterator(m_pair.value.first); }

		Iterator      end()        noexcept { return Iterator(m_pair.value.last); }
		ConstIterator end()  const noexcept { return ConstIterator(m_pair.value.last); }
		ConstIterator cend() const noexcept { return ConstIterator(m_pair.value.last); }

		ReverseIterator      rbegin()        noexcept { return ReverseIterator(end()); }
		ConstReverseIterator rbegin()  const noexcept { return ConstReverseIterator(end()); }
		ConstReverseIterator crbegin() const noexcept { return ConstReverseIterator(end()); }

		ReverseIterator      rend()        noexcept { return ReverseIterator(begin()); }
		ConstReverseIterator rend()  const noexcept { return ConstReverseIterator(begin()); }
		ConstReverseIterator crend() const noexcept { return ConstReverseIterator(begin()); }

	private:
		Status _AppendUnchecked(const BasicSafeString& other) noexcept {
			size_t thisSize  = GetSize();
			size_t otherSize = other.GetSize();
			size_t newSize   = thisSize + otherSize;

			if (newSize > GetCapacity()) {
				AQUA_TRY(_ThisReallocate(_SizeToCapacity(newSize)));
			}
			std::memcpy(m_pair.value.first + thisSize, other.m_pair.value.first, sizeof(ValueType) * otherSize);
			m_pair.value.last  = m_pair.value.first + newSize;
			*m_pair.value.last = ValueType(0);

			return Success{};
		}

		Status _AppendUnchecked(ConstPointer cstr, size_t size) noexcept {
			size_t thisSize = GetSize();
			size_t newSize  = thisSize + size;

			if (newSize > GetCapacity()) {
				AQUA_TRY(_ThisReallocate(_SizeToCapacity(newSize)));
			}
			std::memcpy(m_pair.value.first + thisSize, cstr, sizeof(ValueType) * (size + 1));
			m_pair.value.last = m_pair.value.first + newSize;

			return Success{};
		}

		Status _AppendUnchecked(size_t count, ValueType value) noexcept {
			size_t thisSize = GetSize();
			size_t newSize  = thisSize + count;

			if (newSize > GetCapacity()) {
				AQUA_TRY(_ThisReallocate(_SizeToCapacity(newSize)));
			}
			std::fill(m_pair.value.first + thisSize, m_pair.value.first + newSize, value);
			m_pair.value.last  = m_pair.value.first + newSize;
			*m_pair.value.last = ValueType(0);

			return Success{};
		}

		template <typename Iterator>
		Status _AppendUnchecked(Iterator rangeBegin, Iterator rangeEnd, size_t size) noexcept {
			size_t thisSize = GetSize();
			size_t newSize  = thisSize + size;

			if (newSize > GetCapacity()) {
				AQUA_TRY(_ThisReallocate(_SizeToCapacity(newSize)));
			}
			Pointer dest = m_pair.value.first + thisSize;
			while (rangeBegin != rangeEnd) {
				*dest = static_cast<ValueType>(*rangeBegin);
				++dest, ++rangeBegin;
			}
			m_pair.value.end  = m_pair.value.first + newSize;
			*m_pair.value.end = ValueType(0);

			return Success{};
		}

	private:
		Expected<Pointer, Error> _Allocate(size_t count) noexcept {
			if constexpr (noexcept(m_pair.GetAllocator().Allocate(count))) {
				Pointer ptr = m_pair.GetAllocator().Allocate(count);
				if (ptr == nullptr) {
					return Unexpected<Error>(Error::FAILED_TO_ALLOCATE_MEMORY);
				}
				return ptr;
			}
			else {
				try {
					return m_pair.GetAllocator().Allocate(count);
				}
				catch (...) {
					return Unexpected<Error>(Error::FAILED_TO_ALLOCATE_MEMORY);
				}
			}
		}

		Status _ThisReallocate(size_t newCapacity) noexcept {
			AQUA_TRY(_Allocate(newCapacity + 1), ptr);

			size_t thisSize = GetSize();

			if (m_pair.value.first != nullptr) {
				std::memcpy(ptr.GetValue(), m_pair.value.first, sizeof(ValueType) * thisSize);
				m_pair.GetAllocator().Deallocate(m_pair.value.first, GetCapacity() + 1);
			}
			m_pair.value.first = ptr.GetValue();
			m_pair.value.last  = m_pair.value.first + thisSize;
			m_pair.value.end   = m_pair.value.first + newCapacity;

			return Success{};
		}

		Status _ThisGapReallocate(size_t newCapacity, size_t gapIndex, size_t gapSize) noexcept {
			AQUA_TRY(_Allocate(newCapacity + 1), ptr);

			Pointer dest     = ptr.GetValue();
			Pointer gapStart = dest + gapIndex;

			size_t thisSize = GetSize();

			if (gapIndex > 0) {
				std::memcpy(dest, m_pair.value.first, sizeof(ValueType) * gapIndex);
			}
			std::memcpy(gapStart + gapSize, m_pair.value.first + gapIndex,
				sizeof(ValueType) * (thisSize - gapIndex + 1));

			_ThisDeallocate();
			m_pair.value.first = dest;
			m_pair.value.last  = m_pair.value.first + (thisSize + gapSize);
			m_pair.value.end   = dest + newCapacity;

			return Success{};
		}

		void _Deallocate(Pointer ptr, size_t count) noexcept {
			m_pair.GetAllocator().Deallocate(ptr, count);
		}

		void _ThisDeallocate() noexcept {
			if (m_pair.value.first != nullptr) {
				_Deallocate(m_pair.value.first, GetCapacity() + 1);
			}
			m_pair.value = _ThisData();
		}

		void _ThisDeallocateUnchecked() noexcept {
			m_pair.GetAllocator().Deallocate(m_pair.value.first, GetCapacity());
		}

		Status _ThisReallocateNonEmpty(size_t newSize) noexcept {
			size_t newCapacity = _SizeToCapacity(newSize);
			AQUA_TRY(_Allocate(newCapacity), ptr);

			size_t thisSize = _GetSizeUnchecked();
			std::memcpy(ptr.GetValue(), m_pair.value.first, sizeof(ValueType) * thisSize);
			_ThisDeallocateUnchecked();

			m_pair.value.first = ptr.GetValue();
			m_pair.value.last  = m_pair.value.first + thisSize;
			m_pair.value.end   = m_pair.value.first + newCapacity;

			return Success{};
		}

		Status _Reserve(size_t newSize) noexcept {
			if (m_pair.value.first == nullptr && newSize > 0) {
				size_t newCapacity = _SizeToCapacity(newSize);
				AQUA_TRY(_Allocate(newCapacity), ptr);

				m_pair.value.first = ptr.GetValue();
				m_pair.value.last  = m_pair.value.first;
				m_pair.value.end   = m_pair.value.first + newCapacity;

				return Success{};
			}
			if (m_pair.value.first != nullptr && newSize > _GetCapacityUnchecked()) {
				AQUA_TRY(_ThisReallocateNonEmpty(newSize), _);
			}
			return Success{};
		}

	private:
		Status _ThisMakeGap(size_t size, size_t gapIndex, size_t gapSize) noexcept {
			if (size > GetCapacity()) {
				AQUA_TRY(_ThisGapReallocate(_SizeToCapacity(size), gapIndex, gapSize));
			}
			else {
				this->_CopyBackwards(m_pair.value.last + gapSize, m_pair.value.last,
					(m_pair.value.last - (m_pair.value.first + gapIndex)) + 1);
				m_pair.value.last = m_pair.value.first + size;
			}
			return Success{};
		}

	private:
		Expected<AllocatorType, Error> _CopyAllocator() const noexcept {
			if constexpr (noexcept(m_pair.GetAllocator().CopyAsField())) {
				return m_pair.GetAllocator().OnStructureCopyAssign();
			}
			else {
				try {
					return m_pair.GetAllocator().OnStructureCopyAssign();
				} catch (...) {
					return Unexpected<Error>(Error::DATA_STRUCTURE_FAILED_TO_COPY_ALLOCATOR);
				}
			}
		}

	private:
		bool _IsValidIterator(ConstIterator it) const noexcept {
			return (it == cbegin() && it == cend()) || (it >= cbegin() && it <= cend());
		}

		size_t _GetSizeUnchecked() const noexcept {
			return m_pair.value.last - m_pair.value.first;
		}

		size_t _GetCapacityUnchecked() const noexcept {
			return m_pair.value.end - m_pair.value.last;
		}

	private:
		static size_t _SizeToCapacity(size_t size) noexcept {
			return size + (size >> 1);
		}

	private:
		struct _ThisData {
			Pointer first = nullptr;
			Pointer last  = nullptr;
			Pointer end   = nullptr;
		};

		_memory::_AllocatorPair<_ThisData, AllocatorType> m_pair;
	}; // class BasicSafeString

	template <std::integral T>
	StringBuffer<char, 20> ToString(T x) noexcept {
		StringBuffer<char, 20> result;

		if (x == 0) {
			result.Push('0');
			return result;
		}
		char buffer[20];

		unsigned long long ux = static_cast<unsigned long long>(x);

		int8_t i = 19;
		while (ux > 0) {
			buffer[i--] = (char)('0' + (ux % 10));
			ux /= 10;
		}
		bool isNeg = x < 0;
		buffer[i] = (isNeg ? '-' : '\0');
		i += !(isNeg);

		result.Set(buffer + i, 20 - i);
		return result;
	}

	StringBuffer<char, 20> ToString(unsigned long long x) noexcept;

	using SafeString = BasicSafeString<char, aqua::MemorySystem::GlobalAllocator::Proxy<char>>;
} // namespace aqua

#endif // !AQUA_STRING_HEADER