#ifndef AQUA_UTILITY_MEMORY_HEADER
#define AQUA_UTILITY_MEMORY_HEADER

#include <aqua/Error.h>
#include <aqua/engine/MemorySystem.h>

namespace aqua {
	// Data, that cannot be copied (incapsulates pointer to this data and behaves like pointer)
	template <typename T, typename Allocator = aqua::MemorySystem::GlobalAllocator::Proxy<T>>
	class UniqueData;

	// Data, that can be shared and observed (incapsulates pointer to this data and behaves like pointer)
	template <typename T, typename Allocator = aqua::MemorySystem::GlobalAllocator::Proxy<T>>
	class SharedData;

	// Observes shared data and check, whether it's alive or not
	template <typename T>
	class DataObserver;

	namespace _memory {
		template <typename T, typename Allocator, bool = std::is_empty_v<Allocator> && !std::is_final_v<Allocator>>
		struct _AllocatorPair final : private Allocator {
		public:
			_AllocatorPair() : Allocator(), value() {}

			_AllocatorPair(const _AllocatorPair& other) : Allocator(other), value(other.value) {}
			_AllocatorPair(_AllocatorPair&& other) noexcept : Allocator(std::move(other)), value(std::move(other.value)) {}

			template <typename _T>
			_AllocatorPair(_T&& value) : Allocator(), value(std::forward<_T>(value)) {}

			template <typename _T, typename _Allocator>
			_AllocatorPair(_T&& value, _Allocator&& allocator) :
			Allocator(std::forward<_Allocator>(allocator)), value(std::forward<_T>(value)) {}

		public:
			_AllocatorPair& operator=(const _AllocatorPair& other) {
				value = other.value;
				GetAllocator() = other.GetAllocator();
				return *this;
			}

			_AllocatorPair& operator=(_AllocatorPair&& other) noexcept {
				value = std::move(other.value);
				GetAllocator() = std::move(other.GetAllocator());
				return *this;
			}

		public:
			Allocator&		 GetAllocator()		  { return *this; }
			const Allocator& GetAllocator() const { return *this; }

		public:
			T value;
		}; // struct _AllocatorPair (empty allocator base)

		template <typename T, typename Allocator>
		struct _AllocatorPair<T, Allocator, false> {
		public:
			_AllocatorPair() : allocator(), value() {}

			_AllocatorPair(const _AllocatorPair& other) : value(other.value), allocator(other.allocator) {}
			_AllocatorPair(_AllocatorPair&& other) noexcept : value(std::move(other.value)), allocator(std::move(other.allocator)) {}

			template <typename _T>
			_AllocatorPair(_T&& value) : allocator(), value(std::forward<_T>(value)) {}

			template <typename _T, typename _Allocator>
			_AllocatorPair(_T&& value, _Allocator&& allocator) :
				allocator(std::forward<_Allocator>(allocator)), value(std::forward<_T>(value)) {}

		public:
			_AllocatorPair& operator=(const _AllocatorPair&) = default;
			_AllocatorPair& operator=(_AllocatorPair&&) noexcept = default;

		public:
			Allocator&	     GetAllocator()		  { return allocator; }
			const Allocator& GetAllocator() const { return allocator; }

		public:
			Allocator allocator;
			T         value;
		}; // struct _AllocatorPair

		class _UniqueDataBase {
		protected:
			struct _Helper {
				template <typename T, typename Allocator>
				void ForgetPtr(UniqueData<T, Allocator>& data) noexcept {
					data.m_pair.value = nullptr;
				}
			}; // struct _UniqueDataHelper
		}; // class _UniqueDataBase

		class _SharedDataBase {
		public:
			_SharedDataBase() noexcept : m_counterPair() {}
			_SharedDataBase(const _SharedDataBase& other) noexcept : m_counterPair(other.m_counterPair) {}
			_SharedDataBase(_SharedDataBase&& other) noexcept : m_counterPair(std::move(other.m_counterPair)) {
				other.m_counterPair.value = nullptr;
			}

		protected:
			struct _Helper {
				template <typename T, typename Allocator>
				void ForgetPtr(const SharedData<T, Allocator>& data) noexcept {
					data.m_pair.value = nullptr;
					data.m_counterPair.value = nullptr;
				}
			};

		protected:
			struct _Counter {
				size_t sharedCount   = 0;
				size_t observerCount = 0;
			};

			using _CounterAllocatorType = aqua::MemorySystem::GlobalAllocator::Proxy<_Counter>;
			using _CounterPointer		= typename _CounterAllocatorType::Pointer;

		protected:
			_AllocatorPair<_CounterPointer, _CounterAllocatorType> m_counterPair;
		}; // class _SharedDataBase
	} // namespace _memory

	template <typename T, typename Allocator = aqua::MemorySystem::GlobalAllocator::Proxy<T>, typename ... Types>
	Expected<UniqueData<T, Allocator>, Error> CreateUniqueData(Types&& ... args) {
		UniqueData<T, Allocator> data;
		typename UniqueData<T, Allocator>::Pointer ptr = data.GetAllocator().Allocate(1);
		if constexpr (noexcept(data.GetAllocator().Allocate(1))) {
			if (ptr == nullptr) {
				return Unexpected<Error>(Error::FAILED_TO_ALLOCATE_MEMORY);
			}
		}
		try {
			new (ptr) UniqueData<T, Allocator>::ValueType(std::forward<Types>(args)...);
		}
		catch (...) {
			data.GetAllocator().Deallocate(ptr, 1);
			throw;
		}
		data.m_pair.value = ptr;
		return Expected<UniqueData<T, Allocator>, Error>(std::move(data));
	}
	
	// Data, that cannot be copied (incapsulates pointer to this data and behaves like pointer)
	template <typename T, typename Allocator>
	class UniqueData : _memory::_UniqueDataBase {
	public:
		template <typename _T, typename _Allocator, typename ... Types>
		friend Expected<UniqueData<_T, _Allocator>, Error> CreateUniqueData(Types&& ...);

	public:
		using AllocatorType  = Allocator;

		using ValueType      = T;
		using Pointer        = typename AllocatorType::Pointer;
		using Reference	     = ValueType&;
		using ConstPointer   = const Pointer;
		using ConstReference = const ValueType&;

	private:
		friend struct _memory::_UniqueDataBase::_Helper;

	public:
		UniqueData() noexcept requires(std::is_nothrow_default_constructible_v<AllocatorType>) : m_pair() {};

		UniqueData(const UniqueData&) = delete;

		UniqueData(UniqueData&& other) noexcept requires(std::is_nothrow_move_constructible_v<AllocatorType>) :
		m_pair(std::move(other.m_pair)) {
			other.m_pair.value = nullptr;
		}

		template <typename U, typename AllocatorU>
		UniqueData(UniqueData<U, AllocatorU>&& other) noexcept requires(
		std::is_nothrow_constructible_v<AllocatorType, AllocatorU&&>) :
		m_pair(other.GetPtr(), std::move(other.GetAllocator())) {
			static_assert(std::is_base_of_v<T, U> && std::is_polymorphic_v<U>,
				"aqua::UniqueData - T is not a polymorphic base of U");

			_memory::_UniqueDataBase::_Helper().ForgetPtr(other);
		}

		template <typename U, typename AllocatorU>
		UniqueData(const UniqueData<U, AllocatorU>&) = delete;

		~UniqueData() { _DestroyDeallocate(); }

	public:
		UniqueData& operator=(const UniqueData&) = delete;

		UniqueData& operator=(UniqueData&& other) noexcept requires(std::is_nothrow_move_assignable_v<AllocatorType>) {
			_DestroyDeallocate();
			m_pair = std::move(other.m_pair);
			other.m_pair.value = nullptr;
			return *this;
		}

		template <typename U, typename AllocatorU>
		UniqueData& operator=(UniqueData<U, AllocatorU>&& other) requires(
		std::is_nothrow_assignable_v<AllocatorType, AllocatorU&&>) {
			static_assert(std::is_base_of_v<T, U> && std::is_polymorphic_v<U>,
				"aqua::UniqueData - T is not a polymorphic base of U");

			_DestroyDeallocate();
			m_pair.value = other.GetPtr();
			m_pair.GetAllocator() = std::move(other.GetAllocator());

			_memory::_UniqueDataBase::_Helper().ForgetPtr(other);
			return *this;
		}

		template <typename U, typename AllocatorU>
		UniqueData& operator=(const UniqueData<U, AllocatorU>&) = delete;

		UniqueData& operator=(nullptr_t) noexcept {
			Set(nullptr);
			return *this;
		}

		bool operator==(nullptr_t) const noexcept { return m_pair.value == nullptr; }
		bool operator!=(nullptr_t) const noexcept { return m_pair.value != nullptr; }

		Reference	   operator*()		 { return *m_pair.value; }
		ConstReference operator*() const { return *m_pair.value; }

		Pointer      operator->()		noexcept { return m_pair.value; }
		ConstPointer operator->() const noexcept { return m_pair.value; }

	public:
		AllocatorType&		 GetAllocator() noexcept	   { return m_pair.GetAllocator(); }
		const AllocatorType& GetAllocator() const noexcept { return m_pair.GetAllocator(); }

		Pointer	     GetPtr()		noexcept { return m_pair.value; }
		ConstPointer GetPtr() const noexcept { return m_pair.value; }

		bool HasData() const noexcept {
			return m_pair.value != nullptr;
		}

		void Set(nullptr_t) noexcept {
			_DestroyDeallocate();
			m_pair.value = nullptr;
		}

	private:
		void _DestroyDeallocate() noexcept {
			if (m_pair.value != nullptr) {
				if constexpr (!std::is_trivially_destructible_v<ValueType>) {
					m_pair.value->~ValueType();
				}
				m_pair.GetAllocator().Deallocate(m_pair.value, 1);
			}
		}

	private:
		_memory::_AllocatorPair<Pointer, AllocatorType> m_pair;
	}; // class UniqueData

	template <typename T, typename Allocator = aqua::MemorySystem::GlobalAllocator::Proxy<T>, typename ... Types>
	Expected<SharedData<T, Allocator>, Error> CreateSharedData(Types&& ... args) {
		SharedData<T, Allocator> data;
		typename SharedData<T, Allocator>::Pointer ptr = data.GetAllocator().Allocate(1);
		if constexpr (noexcept(data.GetAllocator().Allocate(1))) {
			if (ptr == nullptr) {
				return Unexpected<Error>(Error::FAILED_TO_ALLOCATE_MEMORY);
			}
		}
		typename SharedData<T, Allocator>::_CounterPointer counterPtr =
			data.m_counterPair.GetAllocator().Allocate(1);
		if (counterPtr == nullptr) {
			data.GetAllocator().Deallocate(ptr, 1);
			return Unexpected<Error>(Error::FAILED_TO_ALLOCATE_MEMORY);
		}
		counterPtr->sharedCount = 1;
		counterPtr->observerCount = 0;
		try {
			new (ptr) SharedData<T, Allocator>::ValueType(std::forward<Types>(args)...);
		} catch (...) {
			data.GetAllocator().Deallocate(ptr, 1);
			data.m_counterPair.GetAllocator().Deallocate(counterPtr, 1);
			throw;
		}
		data.m_pair.value = ptr;
		data.m_counterPair.value = counterPtr;
		return Expected<SharedData<T, Allocator>, Error>(std::move(data));
	}

	// Data, that can be shared and observed (incapsulates pointer to this data and behaves like pointer)
	template <typename T, typename Allocator>
	class SharedData : public _memory::_SharedDataBase {
	public:
		template <typename _T, typename _Allocator, typename ... Types>
		friend Expected<SharedData<_T, _Allocator>, Error> CreateSharedData(Types&& ... types);

		friend class DataObserver<T>;

	public:
		using AllocatorType  = Allocator;

		using ValueType      = T;
		using Pointer        = typename AllocatorType::Pointer;
		using Reference      = ValueType&;
		using ConstPointer   = const Pointer;
		using ConstReference = const ValueType&;

	private:
		using _BaseType	= _memory::_SharedDataBase;

		friend struct _memory::_SharedDataBase::_Helper;

	public:
		SharedData() noexcept requires(std::is_nothrow_default_constructible_v<AllocatorType>) : _BaseType(), m_pair() {};

		SharedData(const SharedData& other) noexcept requires(std::is_nothrow_copy_constructible_v<AllocatorType>) :
		_BaseType(other), m_pair(other.m_pair) {
			_CounterPointer counter = m_counterPair.value;
			if (counter != nullptr && counter->sharedCount > 0) {
				++counter->sharedCount;
			}
		}

		SharedData(SharedData&& other) noexcept requires(std::is_nothrow_move_constructible_v<AllocatorType>) :
		_BaseType(std::move(other)), m_pair(std::move(other.m_pair)) {
			other.m_pair.value = nullptr;
			other.m_counterPair.value = nullptr;
		}

		template <typename U, typename AllocatorU>
		SharedData(const SharedData<U, AllocatorU>& other) noexcept requires(
		std::is_nothrow_constructible_v<AllocatorType, const AllocatorU&>) :
		_BaseType(other), m_pair(other.GetPtr(), other.GetAllocator()) {
			static_assert(std::is_base_of_v<T, U> && std::is_polymorphic_v<U>,
				"aqua::SharedData - T is not a polymorphic base of U");

			_CounterPointer counter = m_counterPair.value;
			if (counter != nullptr && counter->sharedCount > 0) {
				++counter->sharedCount;
			}
		}

		template <typename U, typename AllocatorU>
		SharedData(SharedData<U, AllocatorU>&& other) noexcept requires(
		std::is_nothrow_constructible_v<AllocatorType, AllocatorU&&>) :
		_BaseType(std::move(other)), m_pair(other.GetPtr(), std::move(other.GetAllocator())) {
			static_assert(std::is_base_of_v<T, U> && std::is_polymorphic_v<U>,
				"aqua::SharedData - T is not a polymorphic base of U");

			_BaseType::_Helper().ForgetPtr(other);
		}

		~SharedData() { _DestroyDeallocate(); }

	public:
		SharedData& operator=(const SharedData& other) noexcept requires(std::is_nothrow_copy_assignable_v<AllocatorType>) {
			_DestroyDeallocate();
			m_pair = other.m_pair;
			m_counterPair = other.m_counterPair;

			_CounterPointer counter = m_counterPair.value;
			if (counter != nullptr && counter->sharedCount > 0) {
				++counter->sharedCount;
			}
			return *this;
		}

		SharedData& operator=(SharedData&& other) noexcept requires(std::is_nothrow_move_assignable_v<AllocatorType>) {
			_DestroyDeallocate();
			m_pair = std::move(other.m_pair);
			m_counterPair = std::move(other.m_counterPair);

			other.m_pair.value = nullptr;
			other.m_counterPair.value = nullptr;
			return *this;
		}

		template <typename U, typename AllocatorU>
		SharedData& operator=(const SharedData<U, AllocatorU>& other) noexcept requires(
		std::is_nothrow_assignable_v<AllocatorType, const AllocatorU&>) {
			static_assert(std::is_base_of_v<T, U> && std::is_polymorphic_v<U>,
				"aqua::SharedData - T is not a polymorphic base of U");

			_DestroyDeallocate();
			m_pair.value = other.GetPtr();
			m_pair.GetAllocator() = other.GetAllocator();
			m_counterPair = static_cast<const _BaseType&>(other).m_counterPair;

			_CounterPointer counter = m_counterPair.value;
			if (counter != nullptr && counter->sharedCount > 0) {
				++counter->sharedCount;
			}
			return *this;
		}

		template <typename U, typename AllocatorU>
		SharedData& operator=(SharedData<U, AllocatorU>&& other) noexcept requires(
		std::is_nothrow_assignable_v<AllocatorType, AllocatorU&&>) {
			static_assert(std::is_base_of_v<T, U> && std::is_polymorphic_v<U>,
				"aqua::SharedData - T is not a polymorphic base of U");

			_DestroyDeallocate();
			m_pair.value = other.GetPtr();
			m_pair.GetAllocator() = std::move(other.GetAllocator());
			m_counterPair = std::move(static_cast<_BaseType&&>(other).m_counterPair);

			_BaseType::_Helper().ForgetPtr(other);
			return *this;
		}

		SharedData& operator=(nullptr_t) noexcept {
			Set(nullptr);
			return *this;
		}

		bool operator==(nullptr_t) const noexcept {
			_CounterPointer counter = m_counterPair.value;
			return counter == nullptr || counter->sharedCount == 0;
		}
		bool operator!=(nullptr_t) const noexcept { return !(*this == nullptr); }

		Reference	   operator*()		 { return *m_pair.value; }
		ConstReference operator*() const { return *m_pair.value; }

		Pointer	     operator->()       noexcept { return m_pair.value; }
		ConstPointer operator->() const noexcept { return m_pair.value; }

	public:
		AllocatorType&		 GetAllocator()		  noexcept { return m_pair.GetAllocator(); }
		const AllocatorType& GetAllocator() const noexcept { return m_pair.GetAllocator(); }

		Pointer		 GetPtr()		noexcept { return m_pair.value; }
		ConstPointer GetPtr() const noexcept { return m_pair.value; }

		bool HasValue() const noexcept {
			_CounterPointer counter = m_counterPair.value;
			return counter != nullptr && counter->sharedCount > 0;
		}

		void Set(nullptr_t) noexcept {
			_DestroyDeallocate();
			m_pair.value = nullptr;
		}

	private:
		void _DestroyDeallocate() noexcept {
			Pointer			ptr     = m_pair.value;
			_CounterPointer counter = m_counterPair.value;

			if (counter != nullptr && counter->sharedCount == 1) {
				if constexpr (!std::is_trivially_destructible_v<ValueType>) {
					ptr->~ValueType();
				}
				m_pair.GetAllocator().Deallocate(ptr, 1);

				--counter->sharedCount;
				if (counter->sharedCount == 0 && counter->observerCount == 0) {
					m_counterPair.GetAllocator().Deallocate(counter, 1);
					m_counterPair.value = nullptr;
				}
			}
		}

	private:
		_memory::_AllocatorPair<Pointer, AllocatorType> m_pair;
	}; // class SharedData

	// Observes shared data and check, whether it's alive or not
	template <typename T>
	class DataObserver : private _memory::_SharedDataBase {
	public:
		using ValueType = T;

	private:
		using _BaseType = _memory::_SharedDataBase;

	public:
		DataObserver() noexcept = default;

		DataObserver(const DataObserver& other) noexcept : _BaseType(other), m_ptr(other.m_ptr) {
			if (m_counterPair.value != nullptr) {
				++m_counterPair.value->observerCount;
			}
		}

		DataObserver(DataObserver&& other) noexcept : _BaseType(std::move(other)), m_ptr(other.m_ptr) {
			other.m_ptr = nullptr;
		}

		template <typename Allocator>
		DataObserver(const SharedData<T, Allocator>& other) noexcept : _BaseType(other), m_ptr(other.m_pair.value) {
			if (m_counterPair.value != nullptr) {
				++m_counterPair.value->observerCount;
			}
		}

		~DataObserver() { _DestroyDeallocate(); }

	public:
		DataObserver& operator=(const DataObserver& other) noexcept {
			_DestroyDeallocate();
			m_ptr = other.m_ptr;
			m_counterPair = other.m_counterPair;

			if (m_counterPair.value != nullptr) {
				++m_counterPair.value->observerCount;
			}
			return *this;
		}

		DataObserver& operator=(DataObserver&& other) noexcept {
			_DestroyDeallocate();
			m_ptr = other.m_ptr;
			m_counterPair = std::move(other.m_counterPair);

			other.m_ptr = nullptr;
			return *this;
		}

		template <typename Allocator>
		DataObserver& operator=(const SharedData<T, Allocator>& other) noexcept {
			_DestroyDeallocate();
			m_ptr = other.m_pair.value;
			m_counterPair = other.m_counterPair;

			if (m_counterPair.value != nullptr) {
				++m_counterPair.value->observerCount;
			}
			return *this;
		}

	public:
		bool IsExpired() const noexcept {
			return m_counterPair.value == nullptr || m_counterPair.value->sharedCount == 0;
		}

		T*		 GetPtr()	    noexcept { return m_ptr; }
		const T* GetPtr() const noexcept { return m_ptr; }

	private:
		void _DestroyDeallocate() noexcept {
			if (m_counterPair.value != nullptr) {
				--m_counterPair.value->observerCount;
				if (m_counterPair.value->sharedCount == 0 && m_counterPair.value->observerCount == 0) {
					m_counterPair.GetAllocator().Deallocate(m_counterPair.value, 1);
				}
			}
		}

	private:
		T* m_ptr = nullptr;
	}; // class DataObserver
} // namespace aqua

#endif // !AQUA_UTILITY_MEMORY_HEADER