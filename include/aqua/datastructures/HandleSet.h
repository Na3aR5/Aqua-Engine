#ifndef AQUA_HANDLE_SET_HEADER
#define AQUA_HANDLE_SET_HEADER

#include <aqua/datastructures/Array.h>

#include <iostream>

namespace aqua {
	// Unordered data structure that provides: Addition - O(1), Removal - O(1), Get - O(1), Stable handles
	template <typename T, typename Allocator = aqua::MemorySystem::GlobalAllocator::Proxy<T>>
	class HandleSet;

	namespace _datastructures {
		class HandleSetBase {
		public:
			static constexpr size_t s_MaxGeneration = (~(size_t)0);
			static constexpr size_t s_MaxHandleCount = (~(size_t)0) >> 1;

		public:
			class Handle {
			public:
				Handle() = default;
				Handle(size_t id, size_t gen) : m_id(id), m_gen(gen) {}

				bool operator==(const Handle& other) const noexcept;
				
			public:
				template <typename T, typename Allocator>
				friend class HandleSet;

				size_t m_id  = 0;
				size_t m_gen = 0;
			}; // class Handle

			class Slot {
			public:
				Slot() noexcept = default;
				Slot(size_t index, size_t gen) noexcept : m_index((index << 1) | 1), m_gen(gen) {}

			public:
				size_t GetGeneration() const noexcept;
				size_t GetIndex() const noexcept;

				void SetIndex(size_t index) noexcept;
				void IncrementGeneration() noexcept;

				void Empty() noexcept;
				void Reuse() noexcept;
				bool IsEmpty() const noexcept;

			private:
				size_t m_index = 0;
				size_t m_gen   = 0;
			}; // class Slot
		}; // class HandleSetBase
	} // namespace _datastructures

	// Unordered data structure that provides: Addition - O(1), Removal - O(1), Get - O(1), Stable handles
	template <typename T, typename Allocator>
	class HandleSet : public _datastructures::HandleSetBase {
	public:
		static_assert(std::is_swappable_v<T>, "aqua::HandleSet - Value type must be swappable");

	public:
		using AllocatorType = Allocator;

		using ValueType      = T;
		using Pointer        = typename AllocatorType::Pointer;
		using Reference      = ValueType&;
		using ConstPointer   = const Pointer;
		using ConstReference = const ValueType&;

	public:
		template <typename ... Types>
		Expected<Handle, Error> Emplace(Types&& ... args) noexcept
		requires (std::is_nothrow_constructible_v<T, Types...>) {
			size_t valueIndex = m_values.GetSize();

			if (valueIndex < m_valueIndices.GetSize()) {
				size_t valueID = m_indexToId[valueIndex];

				m_values.EmplaceBackUnchecked(std::forward<Types>(args)...);
				m_valueIndices[valueID].Reuse();
				m_valueIndices[valueID].SetIndex(valueIndex);

				return Handle(valueID, m_valueIndices[valueID].GetGeneration());
			}
			AQUA_TRY(m_values.EmplaceBack(std::forward<Types>(args)...));
			AQUA_TRY(m_valueIndices.EmplaceBack(valueIndex, 0));
			AQUA_TRY(m_indexToId.EmplaceBack(valueIndex));

			return Handle(valueIndex, 0);
		}

		bool IsValid(Handle handle) const noexcept {
			return handle.m_id < m_valueIndices.GetSize() &&
				   m_valueIndices[handle.m_id].GetIndex() < m_values.GetSize() &&
				   !m_valueIndices[handle.m_id].IsEmpty() &&
				   m_valueIndices[handle.m_id].GetGeneration() == handle.m_gen;
		}

		Reference Get(Handle handle) noexcept {
			return m_values[m_valueIndices[handle.m_id].GetIndex()];
		}

		ConstReference Get(Handle handle) const noexcept {
			return m_values[m_valueIndices[handle.m_id].GetIndex()];
		}

		void Remove(Handle handle) noexcept {
			if (!IsValid(handle)) {
				return;
			}
			size_t valueIndex = m_valueIndices[handle.m_id].GetIndex();
			size_t lastValueID = m_indexToId[m_values.GetSize() - 1];
			size_t currValueID = m_indexToId[valueIndex];

			Slot& lastValueSlot = m_valueIndices[lastValueID];
			Slot& currValueSlot = m_valueIndices[currValueID];

			std::swap(m_values[valueIndex], m_values.Last());
			std::swap(m_indexToId[valueIndex], m_indexToId[m_values.GetSize() - 1]);

			lastValueSlot.SetIndex(valueIndex);

			currValueSlot.IncrementGeneration();
			currValueSlot.Empty();
			m_values.Pop();
		}

	private:
		// Get value by 'valueIndex'
		aqua::SafeArray<T, AllocatorType> m_values;

		// Get 'ID' by 'valueIndex'
		aqua::SafeArray<size_t, typename AllocatorType::template Rebind<size_t>::AllocatorType> m_indexToId;

		// Get 'valueIndex' & 'generation' by 'ID'
		aqua::SafeArray<Slot, typename AllocatorType::template Rebind<Slot>::AllocatorType> m_valueIndices;
	}; // class HandleSet
} // namespace aqua

#endif // !AQUA_HANDLE_SET_HEADER