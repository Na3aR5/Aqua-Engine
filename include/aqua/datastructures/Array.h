#ifndef AQUA_ARRAY_HEADER
#define AQUA_ARRAY_HEADER

#include <aqua/utility/Memory.h>

#include <iterator>

namespace aqua {
	// Nothrow dynamic array
	template <typename T, typename Allocator = aqua::MemorySystem::GlobalAllocator::Proxy<T>>
	class SafeArray {
	public:
		using AllocatorType  = Allocator;

		using ValueType		 = T;
		using Pointer		 = typename AllocatorType::Pointer;
		using Reference	     = ValueType&;
		using ConstPointer	 = const Pointer;
		using ConstReference = const ValueType&;

		using Iterator		       = Pointer;
		using ConstIterator		   = ConstPointer;
		using ReverseIterator	   = std::reverse_iterator<Iterator>;
		using ConstReverseIterator = std::reverse_iterator<ConstIterator>;

	public:
		SafeArray() noexcept(std::is_nothrow_default_constructible_v<AllocatorType>) : m_pair() {};
		SafeArray(const SafeArray&) = delete;

		SafeArray(SafeArray&& other) noexcept : m_pair(std::move(other.m_pair)) {
			other.m_pair.value = _ThisData();
		}

		~SafeArray() { _ThisDeallocate(); }

	public:
		SafeArray& operator=(const SafeArray&) = delete;

		SafeArray& operator=(SafeArray&& other) noexcept {
			_ThisDeallocate();
			m_pair = std::move(other.m_pair);
			other.m_pair.value = _ThisData();
			return *this;
		}

		Reference	   operator[](size_t index)		  { return m_pair.value.first[index]; }
		ConstReference operator[](size_t index) const { return m_pair.value.first[index]; }

	// Set() method overloads
	public:
		// Set new value of array as copy of 'other' array
		[[nodiscard]] Status Set(const SafeArray& other) noexcept
		requires(std::is_nothrow_copy_constructible_v<ValueType>) {
			if (this == &other) {
				return Success{};
			}
			if (other.IsEmpty()) {
				_ThisDeallocate();
				return Success{};
			}
			AQUA_TRY(_CopyAllocatorFrom(other.m_pair.GetAllocator()), copiedAllocator);

			size_t otherSize = other._GetSizeUnchecked();
			if (m_pair.value.first == nullptr || otherSize > _GetCapacityUnchecked()) {
				size_t otherCapacity = other._GetCapacityUnchecked();
				AQUA_TRY(_Allocate(otherCapacity), newArray);

				_Copy(newArray.GetValue(), other.m_pair.value.first, other.m_pair.value.last);
				_ThisDeallocate();
				m_pair.value.first = newArray.GetValue();
				m_pair.value.end = m_pair.value.first + otherCapacity;
			}
			else {
				_DestroyRange(m_pair.value.first, m_pair.value.last);
				_Copy(m_pair.value.first, other.m_pair.value.first, other.m_pair.value.last);
			}
			m_pair.value.last = m_pair.value.first + otherSize;
			m_pair.GetAllocator() = std::move(copiedAllocator.GetValue());

			return Success{};
		}

		// Set new value of array as sequence of 'count' repeating 'value' objects
		[[nodiscard]] Status Set(size_t count, ConstReference value) noexcept
		requires(std::is_nothrow_copy_constructible_v<ValueType>) {
			if (count == 0) {
				_ThisDeallocate();
				return Success{};
			}
			if (m_pair.value.first == nullptr || count > _GetCapacityUnchecked()) {
				size_t newCapacity = _SizeToCapacity(count);
				AQUA_TRY(_Allocate(newCapacity), newArray);

				std::fill(newArray.GetValue(), newArray.GetValue() + count, value);
				_ThisDeallocate();
				m_pair.value.first = newArray.GetValue();
				m_pair.value.end = m_pair.value.first + newCapacity;
			}
			else {
				_DestroyRange(m_pair.value.first, m_pair.value.last);
				std::fill(m_pair.value.first, m_pair.value.first + count, value);
			}
			m_pair.value.last = m_pair.value.first + count;

			return Success{};
		}

		// Set new value of array as copied range ['rangeBegin', 'rangeEnd') of elements
		template <std::forward_iterator Iterator>
		[[nodiscard]] Status Set(Iterator rangeBegin, Iterator rangeEnd) noexcept
		requires(std::is_nothrow_assignable_v<ValueType, std::iter_value_t<Iterator>> ||
				(std::is_nothrow_convertible_v<std::iter_value_t<Iterator>, ValueType> &&
				 std::is_nothrow_copy_assignable_v<ValueType>)) {
			if (rangeBegin == rangeEnd) {
				_ThisDeallocate();
				return Success{};
			}
			size_t rangeSize = std::distance(rangeBegin, rangeEnd);
			if (m_pair.value.first == nullptr || rangeSize > _GetCapacityUnchecked()) {
				size_t newCapacity = _SizeToCapacity(rangeSize);
				AQUA_TRY(_Allocate(newCapacity), newArray);

				_ThisDeallocate();
				m_pair.value.first = newArray.GetValue();
				m_pair.value.end   = m_pair.value.first + newCapacity;
			}
			else {
				_DestroyRange(m_pair.value.first, m_pair.value.last);
			}
			Pointer dest = m_pair.value.first;
			while (rangeBegin != rangeEnd) {
				if constexpr (std::is_nothrow_assignable_v<ValueType, std::iter_value_t<Iterator>>) {
					*dest = *rangeBegin;
				}
				else {
					*dest = static_cast<ValueType>(*rangeBegin);
				}
				++dest, ++rangeBegin;
			}
			m_pair.value.last = m_pair.value.first + rangeSize;

			return Success{};
		}

	// 'Add' methods
	public:
		// Construct object at the end by perfect forwarding construct arguments 'args'
		template <typename ... Types>
		[[nodiscard]] Status EmplaceBack(Types&& ... args) noexcept
		requires(std::is_nothrow_constructible_v<ValueType, Types...>) {
			AQUA_TRY(Reserve(1), _);

			new (m_pair.value.last++) ValueType(std::forward<Types>(args)...);
			return Success{};
		}

		// Push value at the end by perfect forwarding construct arguments,
		// assuming array.capacity > array.size
		template <typename ... Types>
		void EmplaceBackUnchecked(Types&& ... args) noexcept
		requires(std::is_nothrow_constructible_v<ValueType, Types...>) {
			new (m_pair.value.last++) ValueType(std::forward<Types>(args)...);
		}

		// Copy 'value' to the end
		[[nodiscard]] Status Push(ConstReference value) noexcept {
			return EmplaceBack(value);
		}

		// Move 'value' to the end
		[[nodiscard]] Status Push(ValueType&& value) noexcept {
			return EmplaceBack(std::move(value));
		}

		// Copy 'count' times 'value' object to the end
		[[nodiscard]] Status Push(size_t count, ConstReference value) noexcept
		requires(std::is_nothrow_copy_constructible_v<ValueType>) {
			AQUA_TRY(Reserve(count), _);

			std::fill(m_pair.value.last, m_pair.value.first + (_GetSizeUnchecked() + count), value);
			m_pair.value.last += count;
			
			return Success{};
		}

		// Copy range ['rangeBegin', 'rangeEnd') of elements into the end
		template <std::forward_iterator Iterator>
		[[nodiscard]] Status Push(Iterator rangeBegin, Iterator rangeEnd) noexcept
		requires(std::is_nothrow_assignable_v<ValueType, std::iter_value_t<Iterator>> ||
				(std::is_nothrow_convertible_v<std::iter_value_t<Iterator>, ValueType> &&
				 std::is_nothrow_copy_assignable_v<ValueType>)) {
			size_t rangeSize = std::distance(rangeBegin, rangeEnd);

			AQUA_TRY(Reserve(rangeSize), _);

			_CopyIteratorRange(m_pair.value.last, rangeBegin, rangeEnd);
			m_pair.value.last += rangeSize;

			return Success{};
		}

		// Copy range ['rangeBegin', 'rangeEnd') of elements into the end
		// assuming array.capacity >= array.size + 'rangeSize'
		template <std::forward_iterator Iterator>
		void PushUnchecked(Iterator rangeBegin, Iterator rangeEnd) noexcept
		requires(std::is_nothrow_assignable_v<ValueType, std::iter_value_t<Iterator>> ||
				(std::is_nothrow_convertible_v<std::iter_value_t<Iterator>, ValueType> &&
				 std::is_nothrow_copy_assignable_v<ValueType>)) {
			size_t rangeSize = std::distance(rangeBegin, rangeEnd);
			
			_CopyIteratorRange(m_pair.value.last, rangeBegin, rangeEnd);
			m_pair.value.last += rangeSize;
		}

		// Copy 'value' to the end assuming array.capacity > array.size
		void PushUnchecked(ConstReference value) noexcept {
			EmplaceBackUnchecked(value);
		}

		// Move 'value' to the end assuming array.capacity > array.size
		void PushUnchecked(ValueType&& value) noexcept {
			EmplaceBackUnchecked(std::move(value));
		}

		// Construct object by perfect forwarding construction arguments 'args' at any valid position 'where'
		template <typename ... Types>
		[[nodiscard]] Expected<Iterator, Error> Emplace(ConstIterator where, Types&& ... args) noexcept
		requires(std::is_nothrow_constructible_v<ValueType, Types...>) {
			if (!_IsValidIterator(where)) {
				return Error::ITERATOR_OR_INDEX_OUT_OF_RANGE;
			}
			if (where == cend()) {
				return EmplaceBack(std::forward<Types>(args)...);
			}
			size_t gapIndex = where - cbegin();

			AQUA_TRY(_ThisMakeGap(_GetSizeUnchecked() + 1, gapIndex, 1), _);

			Pointer wherePtr = m_pair.value.first + gapIndex;
			new (wherePtr) ValueType(std::forward<Types>(args)...);

			return wherePtr;
		}

		// Add copy of 'value' object at any valid position 'where'
		[[nodiscard]] Expected<Iterator, Error> Insert(ConstIterator where, ConstReference value) noexcept {
			return Emplace(where, value);
		}

		// Move 'value' object at any valid position 'where'
		[[nodiscard]] Expected<Iterator, Error> Insert(ConstIterator where, ValueType&& value) noexcept {
			return Emplace(where, std::move(value));
		}

		// Copy 'count' times 'value' object starting from any valid position 'where'
		[[nodiscard]] Expected<Iterator, Error> Insert(
		ConstIterator where, size_t count, ConstReference value) noexcept
		requires(std::is_nothrow_copy_constructible_v<ValueType>) {
			if (!_IsValidIterator(where)) {
				return Error::ITERATOR_OR_INDEX_OUT_OF_RANGE;
			}
			if (count == 0) {
				return where == nullptr ? nullptr : m_pair.value.first + (where - cbegin());
			}
			if (where == cend()) {
				return Push(count, value);
			}
			size_t gapIndex = where - cbegin();

			AQUA_TRY(_ThisMakeGap(_GetSizeUnchecked() + count, gapIndex, count), _);

			Pointer wherePtr = m_pair.value.first + gapIndex;
			Pointer whereEnd = wherePtr + count;
			while (wherePtr != whereEnd) {
				new (wherePtr++) ValueType(value);
			}
			return m_pair.value.first + gapIndex;
		}

	// 'Remove' methods
	public:
		void Clear() noexcept {
			if (m_pair.value.first != nullptr) {
				_DestroyRange(m_pair.value.first, m_pair.value.last);
			}
		}

		void DeepClear() noexcept {
			if (m_pair.value.first != nullptr) {
				_DestroyRange(m_pair.value.first, m_pair.value.last);
				m_pair.GetAllocator().Deallocate(m_pair.value.first, _GetCapacityUnchecked());

				m_pair.value.first = nullptr;
				m_pair.value.last  = nullptr;
				m_pair.value.end   = nullptr;
			}
		}

		// Remove 'count' elements from the end
		void Pop(size_t count) {
			_DestroyRange(m_pair.value.last - count, m_pair.value.last);
			m_pair.value.last -= count;
		}

		// Remove last element
		void Pop() {
			if constexpr (!std::is_trivially_destructible_v<ValueType>) {
				(m_pair.value.last--)->~ValueType();
			}
			else {
				--m_pair.value.last;
			}
		}

	public:
		// Extend capacity to make sure that 'count' elements can be added without reallocation
		[[nodiscard]] Status Reserve(size_t count) noexcept {
			if (m_pair.value.first == nullptr && count > 0) {
				AQUA_TRY(_ThisReallocate(count), _);
				return Success{};
			}
			size_t thisSize = _GetSizeUnchecked();
			if (m_pair.value.first != nullptr && thisSize + count > _GetCapacityUnchecked()) {
				AQUA_TRY(_ThisReallocate(thisSize + count), _);
			}
			return Success{};
		}

		// If 'newSize' >= array.size, construct by default 'newSize - array.size' elements starting from the end
		// Otherwise - remove 'array.size - newSize' elements from the end
		[[nodiscard]] Status Resize(size_t newSize) noexcept
		requires(std::is_nothrow_default_constructible_v<ValueType>) {
			size_t thisSize = GetSize();
			if (newSize == thisSize) {
				return Success{};
			}
			if (newSize >= thisSize) {
				AQUA_TRY(_Reserve(newSize), _);

				Pointer& rangeBegin = m_pair.value.last;
				Pointer rangeEnd = m_pair.value.first + newSize;
				while (rangeBegin != rangeEnd) {
					new (rangeBegin++) ValueType();
				}
			}
			else {
				// todo
			}
			return Success{};
		}

	// Getters
	public:
		bool IsEmpty() const noexcept {
			return m_pair.value.first == m_pair.value.last;
		}

		size_t GetSize() const noexcept {
			return m_pair.value.first == nullptr ? 0 : m_pair.value.last - m_pair.value.first;
		}

		size_t GetCapacity() const noexcept {
			return m_pair.value.first == nullptr ? 0 : m_pair.value.end - m_pair.value.first;
		}

		Pointer      GetPtr()		noexcept { return m_pair.value.first; }
		ConstPointer GetPtr() const noexcept { return m_pair.value.first; }

		Reference      First()       { return *m_pair.value.first; }
		ConstReference First() const { return *m_pair.value.first; }

		Reference      Last()       { return *(m_pair.value.last - 1); }
		ConstReference Last() const { return *(m_pair.value.last - 1); }

	// Iterator methods
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
		Expected<Pointer, Error> _Allocate(size_t count) noexcept {
			if constexpr (noexcept(m_pair.GetAllocator().Allocate(count))) {
				Pointer ptr = m_pair.GetAllocator().Allocate(count);
				if (ptr == nullptr) {
					return Error::FAILED_TO_ALLOCATE_MEMORY;
				}
				return ptr;
			}
			else {
				try {
					return m_pair.GetAllocator().Allocate(count);
				} catch (...) {
					return Error::FAILED_TO_ALLOCATE_MEMORY;
				}
			}
		}

		Status _ThisReallocate(size_t newSize) noexcept {
			size_t newCapacity = _SizeToCapacity(newSize);
			AQUA_TRY(_Allocate(newCapacity), newArray);

			size_t thisSize = GetSize();
			if (m_pair.value.first != nullptr) {
				_Move(newArray.GetValue(), m_pair.value.first, m_pair.value.last);
				_ThisDeallocateUnchecked();
			}

			m_pair.value.first = newArray.GetValue();
			m_pair.value.last = m_pair.value.first + thisSize;
			m_pair.value.end = m_pair.value.first + newCapacity;

			return Success{};
		}

		Status _ThisReallocateNonEmpty(size_t newSize) noexcept {
			size_t newCapacity = _SizeToCapacity(newSize);
			AQUA_TRY(_Allocate(newCapacity), newArray);

			size_t thisSize = _GetSizeUnchecked();
			_Move(newArray.GetValue(), m_pair.value.first, m_pair.value.last);
			_ThisDeallocateUnchecked();

			m_pair.value.first = newArray.GetValue();
			m_pair.value.last = m_pair.value.first + thisSize;
			m_pair.value.end = m_pair.value.first + newCapacity;

			return Success{};
		}

		Status _ThisReallocateWithGap(size_t newSize, size_t gapIndex, size_t gapSize) noexcept {
			size_t newCapacity = _SizeToCapacity(newSize);
			AQUA_TRY(_Allocate(newCapacity), newArray);

			if (gapIndex > 0) {
				_Move(newArray.GetValue(), m_pair.value.first, m_pair.value.first + gapIndex);
			}
			_Move(newArray.GetValue() + (gapIndex + gapSize),
				m_pair.value.first + gapIndex, m_pair.value.last);
			_ThisDeallocateUnchecked();

			m_pair.value.first = newArray.GetValue();
			m_pair.value.last  = m_pair.value.first + newSize;
			m_pair.value.end   = m_pair.value.first + newCapacity;

			return Success{};
		}

		Status _ThisMakeGap(size_t newSize, size_t gapIndex, size_t gapSize) noexcept {
			if (newSize > GetCapacity()) {
				return _ThisReallocateWithGap(newSize, gapIndex, gapSize);
			}
			_MoveBackwards(m_pair.value.first + (gapIndex + gapSize),
				m_pair.value.first + gapIndex, m_pair.value.last);
			m_pair.value.last = m_pair.value.first + newSize;

			return Success{};
		}

		Status _Reserve(size_t newSize) noexcept {
			if (m_pair.value.first == nullptr && newSize > 0) {
				size_t newCapacity = _SizeToCapacity(newSize);
				AQUA_TRY(_Allocate(newCapacity), newArray);

				m_pair.value.first = newArray.GetValue();
				m_pair.value.last = m_pair.value.first;
				m_pair.value.end = m_pair.value.first + newCapacity;

				return Success{};
			}

			if (m_pair.value.first != nullptr && newSize > _GetCapacityUnchecked()) {
				AQUA_TRY(_ThisReallocateNonEmpty(newSize), _);
			}
			return Success{};
		}

		void _ThisDeallocate() noexcept {
			if (m_pair.value.first != nullptr) {
				_DestroyRange(m_pair.value.first, m_pair.value.last);
				m_pair.GetAllocator().Deallocate(m_pair.value.first, GetCapacity());
			}
		}

		void _ThisDeallocateUnchecked() noexcept {
			_DestroyRange(m_pair.value.first, m_pair.value.last);
			m_pair.GetAllocator().Deallocate(m_pair.value.first, GetCapacity());
		}

		void _DestroyRange(Pointer begin, Pointer end) noexcept {
			if constexpr (!std::is_trivially_destructible_v<ValueType>) {
				while (begin != end) {
					begin->~ValueType();
					++begin;
				}
			}
		}

	private:
		void _Move(Pointer dest, Pointer srcBegin, Pointer srcEnd) noexcept
		requires(std::is_nothrow_move_assignable_v<ValueType>) {
			if constexpr (std::is_trivially_move_assignable_v<ValueType>) {
				std::memcpy(dest, srcBegin, sizeof(ValueType) * (srcEnd - srcBegin));
			}
			else {
				while (srcBegin != srcEnd) {
					*dest = std::move(*srcBegin);
					++dest, ++srcBegin;
				}
			}
		}

		void _Move(Pointer dest, Pointer srcBegin, Pointer srcEnd) noexcept
		requires(!std::is_nothrow_move_assignable_v<ValueType>) {
			_Copy(dest, srcBegin, srcEnd);
			_DestroyRange(srcBegin, srcEnd);
		}

		void _Copy(Pointer dest, Pointer srcBegin, Pointer srcEnd) noexcept
		requires(std::is_nothrow_copy_assignable_v<ValueType>) {
			if constexpr (std::is_trivially_copy_assignable_v<ValueType>) {
				std::memcpy(dest, srcBegin, sizeof(ValueType) * (srcEnd - srcBegin));
			}
			else {
				while (srcBegin != srcEnd) {
					*dest = *srcBegin;
					++dest, ++srcBegin;
				}
			}
		}

		// Assuming dest > srcBegin
		void _MoveBackwards(Pointer dest, Pointer srcBegin, Pointer srcEnd) noexcept
		requires(std::is_nothrow_move_assignable_v<ValueType>) {
			if constexpr (std::is_trivially_move_assignable_v<ValueType>) {
				std::memmove(dest, srcBegin, sizeof(ValueType) * (srcEnd - srcBegin));
			}
			else {
				dest += (srcEnd - srcBegin);
				while (srcEnd >= srcBegin) {
					*--dest = std::move(*--srcEnd);
				}
			}
		}

		void _MoveBackwards(Pointer dest, Pointer srcBegin, Pointer srcEnd) noexcept
		requires(!std::is_nothrow_copy_assignable_v<ValueType>) { _CopyBackwards(dest, srcBegin, srcEnd); }

		void _CopyBackwards(Pointer dest, Pointer srcBegin, Pointer srcEnd) noexcept
		requires(!std::is_nothrow_copy_constructible_v<ValueType>) {
			if constexpr (std::is_trivially_copy_assignable_v<ValueType>) {
				std::memmove(dest, srcBegin, sizeof(ValueType) * (srcEnd - srcBegin));
			}
			else {
				dest += (srcEnd - srcBegin);
				while (srcEnd >= srcBegin) {
					*--dest = *--srcEnd;
				}
			}
		}

		template <typename Iterator>
		void _CopyIteratorRange(Pointer dest, Iterator rangeBegin, Iterator rangeEnd) noexcept {
			while (rangeBegin != rangeEnd) {
				if constexpr (std::is_nothrow_assignable_v<ValueType, std::iter_value_t<Iterator>>) {
					*dest = *rangeBegin;
				}
				else {
					*dest = static_cast<ValueType>(*rangeBegin);
				}
				++dest, ++rangeBegin;
			}
		}

		Expected<AllocatorType, Error> _CopyAllocatorFrom(const AllocatorType& other) const noexcept
		requires(std::is_nothrow_move_constructible_v<AllocatorType>) {
			if constexpr (noexcept(other.OnDataStructureCopy())) {
				return Expected<AllocatorType, Error>(std::move(other.OnDataStructureCopy()));
			}
			else {
				try {
					return Expected<AllocatorType, Error>(std::move(other.OnDataStructureCopy()));
				} catch (...) {
					return Error::DATA_STRUCTURE_FAILED_TO_COPY_ALLOCATOR;
				}
			}
		}

	// Misc
	private:
		bool _IsValidIterator(ConstIterator it) const noexcept {
			return it >= cbegin() && it <= cend();
		}

		size_t _GetSizeUnchecked() const noexcept {
			return m_pair.value.last - m_pair.value.first;
		}

		size_t _GetCapacityUnchecked() const noexcept {
			return m_pair.value.end - m_pair.value.last;
		}

		static size_t _SizeToCapacity(size_t size) noexcept {
			return size + (size >> 1) + 8;
		}

	private:
		struct _ThisData {
			Pointer first = nullptr;
			Pointer last  = nullptr;
			Pointer end   = nullptr;
		};

		_memory::_AllocatorPair<_ThisData, AllocatorType> m_pair;
	}; // class SafeArray
} // namespace aqua

#endif // AQUA_ARRAY_HEADER