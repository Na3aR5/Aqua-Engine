#ifndef AQUA_MEMORY_SYSTEM_HEADER
#define AQUA_MEMORY_SYSTEM_HEADER

#include <aqua/Build.h>
#include <aqua/Error.h>
#include <aqua/Assert.h>
#include <aqua/engine/ForwardSystems.h>

#include <new>
#include <type_traits>

namespace aqua {
#if AQUA_DEBUG_ENABLE_REFERENCE_COUNT
	namespace _memory {
		struct _Counter {
			bool   alive = true;
			size_t references = 0;
		};

		template <typename T>
		class DebugPointerBase {
		public:
			template <typename T>
			struct DeduceReference {
				using Reference = T&;
			};
			template <>
			struct DeduceReference<void> {
				using Reference = void*;
			};

		public:
			DebugPointerBase() noexcept = default;

			DebugPointerBase(const DebugPointerBase& other) noexcept : m_counter(other.m_counter) {
				if (m_counter != nullptr && m_counter->references > 0) {
					++m_counter->references;
				}
			}

			DebugPointerBase(DebugPointerBase&& other) noexcept : m_counter(other.m_counter) {
				other.m_counter = nullptr;
			}

		protected:
			_Counter* _AllocateCounter() const noexcept {
				return (_Counter*)MemorySystem::BootstrapAllocator().Allocate(sizeof(_Counter));
			}

			void _Destroy() {
				if (m_counter != nullptr && m_counter->references > 1) {
					--m_counter->references;
					return;
				}
				if (m_counter != nullptr && m_counter->alive && m_counter->references == 1) {
					AQUA_ASSERT(false, Literal("Memory leak"));
					return;
				}
				if (m_counter != nullptr) {
					MemorySystem::BootstrapAllocator().Deallocate(m_counter, sizeof(_Counter));
					m_counter = nullptr;
				}
			}

		protected:
			_Counter* m_counter = nullptr;
		}; // class DebugPointerBase
	}

	template <typename T>
	class DebugPointer : public _memory::DebugPointerBase<T> {
	public:
		using BaseType = _memory::DebugPointerBase<T>;

		using ValueType		 = T;
		using Pointer		 = T*;
		using Reference      = typename BaseType::template DeduceReference<T>::Reference;
		using ConstPointer   = const Pointer;
		using ConstReference = const Reference;

	public:
		DebugPointer() noexcept = default;
		DebugPointer(const DebugPointer& other) noexcept : BaseType(other), m_ptr(other.m_ptr) {}

		DebugPointer(DebugPointer&& other) noexcept : BaseType(std::move(other)), m_ptr(other.m_ptr) {
			other.m_ptr = nullptr;
		}

		DebugPointer(nullptr_t) noexcept : BaseType(), m_ptr(nullptr) {}

		explicit DebugPointer(Pointer ptr) noexcept : BaseType(), m_ptr(ptr) {
			if (m_ptr == nullptr) {
				return;
			}
			this->m_counter = this->_AllocateCounter();
			this->m_counter->references = 1;
		}

		explicit DebugPointer(Pointer ptr, _memory::_Counter* counter) noexcept : BaseType(), m_ptr(ptr) {
			if (m_ptr == nullptr) {
				return;
			}
			this->m_counter = counter;
			if (this->m_counter != nullptr && this->m_counter->references > 0) {
				++this->m_counter->references;
			}
		}

		template <typename U>
		DebugPointer(const DebugPointer<U>& other) noexcept requires(std::is_base_of_v<T, U>) :
		BaseType(), m_ptr(other.GetPtr()) {
			this->m_counter = other.GetCounter();
		}

		template <typename U>
		DebugPointer(DebugPointer<U>&& other) noexcept requires(std::is_base_of_v<T, U>) :
		BaseType(), m_ptr(other.GetPtr()) {
			this->m_counter = other.GetCounter();
			other.Forget();
		}

		~DebugPointer() { this->_Destroy(); }

	public:
		DebugPointer& operator=(const DebugPointer& other) noexcept {
			this->_Destroy();
			m_ptr = other.m_ptr;
			this->m_counter = other.m_counter;

			if (this->m_counter != nullptr && this->m_counter->references > 0) {
				++this->m_counter->references;
			}
			return *this;
		}

		DebugPointer& operator=(DebugPointer&& other) noexcept {
			this->_Destroy();
			m_ptr = other.m_ptr;
			this->m_counter = other.m_counter;
			other.m_ptr = nullptr;
			other.m_counter = nullptr;

			return *this;
		}

		template <typename U>
		DebugPointer& operator=(const DebugPointer<U>& other) noexcept requires(std::is_base_of_v<T, U>) {
			this->_Destroy();
			m_ptr = other.GetPtr();
			this->m_counter = other.GetCounter();

			if (this->m_counter != nullptr && this->m_counter->references > 0) {
				++this->m_counter->references;
			}
			return *this;
		}

		template <typename U>
		DebugPointer& operator=(DebugPointer<U>&& other) noexcept requires(std::is_base_of_v<T, U>) {
			this->_Destroy();
			m_ptr = other.GetPtr();
			this->m_counter = other.GetCounter();
			other.Forget();
			
			return *this;
		}

		DebugPointer& operator=(nullptr_t) noexcept {
			this->_Destroy();
			m_ptr = nullptr;
			this->m_counter = nullptr;

			return *this;
		}

	public:
		operator Pointer() noexcept { return m_ptr; }
		operator const Pointer() const noexcept { return m_ptr; }

		operator DebugPointer<void>() noexcept {
			return DebugPointer<void>(m_ptr, this->m_counter);
		}

		operator const DebugPointer<void>() const noexcept {
			return DebugPointer<void>(m_ptr, this->m_counter);
		}

	public:
		Reference      operator*()       { return *m_ptr; }
		ConstReference operator*() const { return *m_ptr; }

		Pointer      operator->()       noexcept { return m_ptr; }
		ConstPointer operator->() const noexcept { return m_ptr; }

		Reference	   operator[](ptrdiff_t index)		 { return m_ptr[index]; }
		ConstReference operator[](ptrdiff_t index) const { return m_ptr[index]; }

		DebugPointer& operator++() noexcept { ++m_ptr; return *this; }
		DebugPointer& operator--() noexcept { --m_ptr; return *this; }

		DebugPointer operator++(int) noexcept {
			DebugPointer temp{ *this };
			++m_ptr;
			return temp;
		}
		DebugPointer operator--(int) noexcept {
			DebugPointer temp{ *this };
			--m_ptr;
			return temp;
		}

		DebugPointer& operator+=(ptrdiff_t x) noexcept { m_ptr += x; return *this; }
		DebugPointer& operator-=(ptrdiff_t x) noexcept { m_ptr -= x; return *this; }

		DebugPointer operator+(ptrdiff_t x) const noexcept {
			DebugPointer res{ *this };
			return res += x;
		}
		DebugPointer operator-(ptrdiff_t x) const noexcept {
			DebugPointer res{ *this };
			return res -= x;
		}

		ptrdiff_t operator-(const DebugPointer& other) const noexcept { return m_ptr - other.m_ptr; }

		bool operator==(const DebugPointer& other) const noexcept { return m_ptr == other.m_ptr; }
		bool operator!=(const DebugPointer& other) const noexcept { return m_ptr != other.m_ptr; }

		bool operator==(nullptr_t) const noexcept { return m_ptr == nullptr; }
		bool operator!=(nullptr_t) const noexcept { return m_ptr != nullptr; }

		bool operator>(const DebugPointer& other)  const noexcept { return m_ptr > other.m_ptr; }
		bool operator<(const DebugPointer& other)  const noexcept { return m_ptr < other.m_ptr; }
		bool operator>=(const DebugPointer& other) const noexcept { return m_ptr >= other.m_ptr; }
		bool operator<=(const DebugPointer& other) const noexcept { return m_ptr <= other.m_ptr; }

	public:
		T* GetPtr() const noexcept { return m_ptr; }
		_memory::_Counter* GetCounter() const noexcept { return this->m_counter; }

		void Forget() noexcept {
			this->m_counter = nullptr;
			m_ptr = nullptr;
		}

	private:
		Pointer m_ptr = nullptr;
	}; // class DebugPointer

	template <>
	class DebugPointer<void> : public _memory::DebugPointerBase<void> {
	public:
		using BaseType = _memory::DebugPointerBase<void>;

		using ValueType      = void;
		using Pointer        = void*;
		using Reference      = typename BaseType::template DeduceReference<void>::Reference;
		using ConstPointer   = const Pointer;
		using ConstReference = const Reference;

	public:
		DebugPointer() noexcept = default;
		DebugPointer(const DebugPointer& other) noexcept : BaseType(other), m_ptr(other.m_ptr) {}

		DebugPointer(DebugPointer&& other) noexcept : BaseType(std::move(other)), m_ptr(other.m_ptr) {
			other.m_ptr = nullptr;
		}

		DebugPointer(nullptr_t) noexcept : BaseType(), m_ptr(nullptr) {}

		explicit DebugPointer(Pointer ptr) noexcept : BaseType(), m_ptr(ptr) {
			if (m_ptr == nullptr) {
				return;
			}
			m_counter = this->_AllocateCounter();
			m_counter->references = 1;
		}

		explicit DebugPointer(Pointer ptr, _memory::_Counter* counter) noexcept : BaseType(), m_ptr(ptr) {
			if (m_ptr == nullptr) {
				return;
			}
			m_counter = counter;
			if (m_counter != nullptr && m_counter->references > 0) {
				++m_counter->references;
			}
		}

		~DebugPointer() { this->_Destroy(); }

	public:
		DebugPointer& operator=(const DebugPointer& other) noexcept {
			this->_Destroy();
			m_ptr = other.m_ptr;
			m_counter = other.m_counter;

			if (m_counter != nullptr && m_counter->references > 0) {
				++m_counter->references;
			}
			return *this;
		}

		DebugPointer& operator=(DebugPointer&& other) noexcept {
			this->_Destroy();
			m_ptr = other.m_ptr;
			m_counter = other.m_counter;
			other.m_ptr = nullptr;
			other.m_counter = nullptr;

			return *this;
		}

		DebugPointer& operator=(nullptr_t) noexcept {
			this->_Destroy();
			m_ptr = nullptr;
			m_counter = nullptr;

			return *this;
		}

	public:
		operator Pointer() noexcept { return m_ptr; }
		operator const Pointer() const noexcept { return m_ptr; }

		template <typename T> // static_cast<T*>(void*)
		explicit operator DebugPointer<T>() noexcept {
			return DebugPointer<T>(static_cast<T*>(m_ptr), m_counter);
		}

		template <typename T> // static_cast<const T*>(const void*)
		explicit operator const DebugPointer<T>() const noexcept {
			return DebugPointer<T>(static_cast<T*>(m_ptr), m_counter);
		}

	public:
		bool operator==(const DebugPointer& other) const noexcept { return m_ptr == other.m_ptr; }
		bool operator!=(const DebugPointer& other) const noexcept { return m_ptr != other.m_ptr; }

		bool operator==(nullptr_t) const noexcept { return m_ptr == nullptr; }
		bool operator!=(nullptr_t) const noexcept { return m_ptr != nullptr; }

		bool operator>(const DebugPointer& other)  const noexcept { return m_ptr > other.m_ptr; }
		bool operator<(const DebugPointer& other)  const noexcept { return m_ptr < other.m_ptr; }
		bool operator>=(const DebugPointer& other) const noexcept { return m_ptr >= other.m_ptr; }
		bool operator<=(const DebugPointer& other) const noexcept { return m_ptr <= other.m_ptr; }

	public:
		_memory::_Counter* GetCounter() const noexcept { return m_counter; }

	private:
		Pointer	m_ptr = nullptr;
	}; // class DebugPointer<void>
#endif // AQUA_DEBUG_ENABLE_REFERENCE_COUNT

	// System for managing heap memory, which Engine needs
	class MemorySystem {
#if AQUA_DEBUG_ENABLE_REFERENCE_COUNT
	public:
		template <typename T>
		friend class _memory::DebugPointerBase;

		template <typename T>
		friend class DebugPointer;

		using VoidPointer = DebugPointer<void>;
#else
		using VoidPointer = void*;
#endif // AQUA_DEBUG_ENABLE_REFERENCE_COUNT

	public:
		// Main Engine allocator
		class GlobalAllocator {
		public:
			// Empty proxy-allocator object to access 'GlobalAllocator'
			template <typename T>
			struct Proxy {
			public:
#if AQUA_DEBUG_ENABLE_REFERENCE_COUNT
				using Pointer = DebugPointer<T>;
#else
				using Pointer = T*;
#endif // AQUA_DEBUG_ENABLE_REFERENCE_COUNT

				template <typename U>
				struct Rebind {
					using AllocatorType = Proxy<U>;
				};

			public:
				Proxy()			    noexcept = default;
				Proxy(const Proxy&) noexcept = default;
				Proxy(Proxy&&)      noexcept = default;

				Proxy& operator=(const Proxy&) noexcept = default;
				Proxy& operator=(Proxy&&)	   noexcept = default;

				template <typename U>
				Proxy(const Proxy<U>&) noexcept
					requires(std::is_base_of_v<T, U>&& std::is_polymorphic_v<U>) {};

				template <typename U>
				Proxy(Proxy<U>&&) noexcept
					requires(std::is_base_of_v<T, U>&& std::is_polymorphic_v<U>) {};

				template <typename U>
				Proxy& operator=(const Proxy<U>&) noexcept
				requires(std::is_base_of_v<T, U>&& std::is_polymorphic_v<U>) {
					return *this;
				}

				template <typename U>
				Proxy& operator=(Proxy<U>&&) noexcept
				requires(std::is_base_of_v<T, U>&& std::is_polymorphic_v<U>) {
					return *this;
				}

			public:
				Pointer Allocate(size_t count) const noexcept {
					return static_cast<Pointer>(
						MemorySystem::GetConstGlobalAllocator().Allocate(sizeof(T) * count));
				}

				void Deallocate(Pointer ptr, size_t count) const noexcept {
					return MemorySystem::GetConstGlobalAllocator().Deallocate(ptr, sizeof(T) * count);
				}

				void DeallocateBytes(Pointer ptr, size_t bytes) const noexcept {
					return MemorySystem::GetConstGlobalAllocator().Deallocate(ptr, bytes);
				}

				Proxy OnDataStructureCopy() const noexcept { return Proxy(); }
			}; // class Proxy

		public:
			VoidPointer Allocate(size_t bytes) const noexcept;
			void Deallocate(VoidPointer ptr, size_t bytes) const noexcept;
		}; // class GlobalAllocator

		template <typename T>
		friend struct GlobalAllocator::Proxy;

	public:
		~MemorySystem();

		MemorySystem(const MemorySystem&) = delete;
		MemorySystem(MemorySystem&&) noexcept = delete;

		MemorySystem& operator=(const MemorySystem&) = delete;
		MemorySystem& operator=(MemorySystem&&) noexcept = delete;

	private:
		static MemorySystem::GlobalAllocator&       GetGlobalAllocator()	  noexcept;
		static const MemorySystem::GlobalAllocator& GetConstGlobalAllocator() noexcept;

	private:
		// Nothrow operator new/delete
		class BootstrapAllocator {
		public:
			void* Allocate(size_t bytes) const noexcept;
			void Deallocate(void* ptr, size_t bytes) const noexcept;
		}; // class _BootstrapAllocator

	private:
		friend class Engine;
		MemorySystem(Status& status);

	private:
		GlobalAllocator m_globalAllocator;
	};
	// class EngineMemorySystem
} // namespace aqua

#endif // AQUA_MEMORY_SYSTEM_HEADER