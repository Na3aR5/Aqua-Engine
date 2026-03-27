#ifndef AQUA_MEMORY_SYSTEM_HEADER
#define AQUA_MEMORY_SYSTEM_HEADER

#include <aqua/Build.h>
#include <aqua/Error.h>
#include <aqua/Assert.h>
// #include <aqua/engine/Defines.h>
#include <aqua/engine/ForwardSystems.h>

#include <new>
#include <type_traits>

namespace aqua {
#if AQUA_DEBUG_ENABLE_REFERENCE_COUNT
	namespace _memory {
		struct _Counter {
			uint32_t info = 0;
			size_t references = 0;

			void SetAlive(bool alive) noexcept {
				info &= ~1;
				info |= (uint32_t)alive;
			}
			void SetAllocated() noexcept {
				info |= 2;
			}
			bool IsAlive() const noexcept { return info & 1; }
			bool IsAllocated() const noexcept { return info & 2; }
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
				_IncrementCounter();
			}

			DebugPointerBase(DebugPointerBase&& other) noexcept : m_counter(other.m_counter) {
				other.m_counter = nullptr;
			}

		protected:
			_Counter* _AllocateCounter() const noexcept {
				return (_Counter*)MemorySystem::BootstrapAllocator().AllocatePtr(sizeof(_Counter));
			}

			void _Destroy() {
				if (m_counter != nullptr && m_counter->references > 1) {
					--m_counter->references;
					return;
				}
				if (m_counter != nullptr && m_counter->IsAllocated() && m_counter->IsAlive() && m_counter->references == 1) {
					AQUA_ASSERT(false, Literal("Memory leak"));
					return;
				}
				if (m_counter != nullptr) {
					MemorySystem::BootstrapAllocator().DeallocatePtr(m_counter, sizeof(_Counter));
					m_counter = nullptr;
				}
			}

			void _IncrementCounter() noexcept {
				if (m_counter != nullptr && m_counter->IsAllocated() && m_counter->references > 0) {
					++m_counter->references;
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

		explicit DebugPointer(Pointer ptr, bool isAllocated = true) noexcept : BaseType(), m_ptr(ptr) {
			if (m_ptr == nullptr) {
				return;
			}
			this->m_counter = this->_AllocateCounter();
			if (isAllocated) {
				this->m_counter->references = 1;
				this->m_counter->SetAllocated();
				this->m_counter->SetAlive(true);
			}
		}

		explicit DebugPointer(Pointer ptr, _memory::_Counter* counter) noexcept : BaseType(), m_ptr(ptr) {
			if (m_ptr == nullptr) {
				return;
			}
			this->m_counter = counter;
			this->_IncrementCounter();
		}

		template <typename U>
		DebugPointer(const DebugPointer<U>& other) noexcept requires(std::is_base_of_v<T, U>) :
		BaseType(), m_ptr(other.GetPtr()) {
			this->m_counter = other.GetCounter();
			this->_IncrementCounter();
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
			this->_IncrementCounter();

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
			this->_IncrementCounter();

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
		Reference operator*() {
			CheckAccess();
			return *m_ptr;
		}
		ConstReference operator*() const {
			CheckAccess();
			return *m_ptr;
		}

		Pointer operator->() noexcept {
			CheckAccess();
			return m_ptr;
		}
		ConstPointer operator->() const noexcept {
			CheckAccess();
			return m_ptr;
		}

		Reference operator[](ptrdiff_t index) {
			CheckAccess();
			return m_ptr[index];
		}
		ConstReference operator[](ptrdiff_t index) const {
			CheckAccess();
			return m_ptr[index];
		}

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
		bool operator==(const Pointer ptr) const noexcept { return m_ptr == ptr; }
		bool operator!=(const Pointer ptr) const noexcept { return m_ptr != ptr; }

		bool operator==(nullptr_t) const noexcept { return m_ptr == nullptr; }
		bool operator!=(nullptr_t) const noexcept { return m_ptr != nullptr; }

		bool operator>(const DebugPointer& other)  const noexcept { return m_ptr > other.m_ptr; }
		bool operator<(const DebugPointer& other)  const noexcept { return m_ptr < other.m_ptr; }
		bool operator>=(const DebugPointer& other) const noexcept { return m_ptr >= other.m_ptr; }
		bool operator<=(const DebugPointer& other) const noexcept { return m_ptr <= other.m_ptr; }

		bool operator>(const Pointer ptr)  const noexcept { return m_ptr > ptr; }
		bool operator<(const Pointer ptr)  const noexcept { return m_ptr < ptr; }
		bool operator>=(const Pointer ptr) const noexcept { return m_ptr >= ptr; }
		bool operator<=(const Pointer ptr) const noexcept { return m_ptr <= ptr; }

	public:
		Pointer GetPtr() const noexcept { return m_ptr; }
		_memory::_Counter* GetCounter() const noexcept { return this->m_counter; }

		void Forget() noexcept {
			this->m_counter = nullptr;
			m_ptr = nullptr;
		}

		void CheckAccess() const noexcept {
			if (this->m_counter == nullptr || (this->m_counter->IsAllocated() && !this->m_counter->IsAlive())) {
				if (m_ptr == nullptr) {
					AQUA_ASSERT(false, Literal("Segmentation fault: dereferencing nullptr"));
				}
				if (this->m_counter != nullptr && this->m_counter->IsAllocated()) {
					AQUA_ASSERT(false, Literal("Segmentation fault: access violation"));
				}
			}
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

		explicit DebugPointer(Pointer ptr, bool isAllocated = true) noexcept : BaseType(), m_ptr(ptr) {
			if (m_ptr == nullptr) {
				return;
			}
			m_counter = this->_AllocateCounter();
			if (isAllocated) {
				m_counter->references = 1;
				m_counter->SetAllocated();
				m_counter->SetAlive(true);
				return;
			}
		}

		explicit DebugPointer(Pointer ptr, _memory::_Counter* counter) noexcept : BaseType(), m_ptr(ptr) {
			if (m_ptr == nullptr) {
				return;
			}
			m_counter = counter;
			this->_IncrementCounter();
		}

		~DebugPointer() { this->_Destroy(); }

	public:
		DebugPointer& operator=(const DebugPointer& other) noexcept {
			this->_Destroy();
			m_ptr = other.m_ptr;
			m_counter = other.m_counter;
			this->_IncrementCounter();

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
		bool operator==(const Pointer ptr) const noexcept { return m_ptr == ptr; }
		bool operator!=(const Pointer ptr) const noexcept { return m_ptr != ptr; }

		bool operator==(nullptr_t) const noexcept { return m_ptr == nullptr; }
		bool operator!=(nullptr_t) const noexcept { return m_ptr != nullptr; }

		bool operator>(const DebugPointer& other)  const noexcept { return m_ptr > other.m_ptr; }
		bool operator<(const DebugPointer& other)  const noexcept { return m_ptr < other.m_ptr; }
		bool operator>=(const DebugPointer& other) const noexcept { return m_ptr >= other.m_ptr; }
		bool operator<=(const DebugPointer& other) const noexcept { return m_ptr <= other.m_ptr; }

		bool operator>(const Pointer ptr)  const noexcept { return m_ptr > ptr; }
		bool operator<(const Pointer ptr)  const noexcept { return m_ptr < ptr; }
		bool operator>=(const Pointer ptr) const noexcept { return m_ptr >= ptr; }
		bool operator<=(const Pointer ptr) const noexcept { return m_ptr <= ptr; }

	public:
		_memory::_Counter* GetCounter() const noexcept { return m_counter; }

	private:
		Pointer	m_ptr = nullptr;
	}; // class DebugPointer<void>
#endif // AQUA_DEBUG_ENABLE_REFERENCE_COUNT

	// System for managing heap memory requsted by engine
	class MemorySystem {
	public:
#if AQUA_DEBUG_ENABLE_REFERENCE_COUNT
		template <typename T>
		friend class _memory::DebugPointerBase;

		template <typename T>
		friend class DebugPointer;

		using Handle = DebugPointer<void>;

		template <typename T>
		using Pointer = DebugPointer<T>;

		template <typename T>
		using ConstPointer = const DebugPointer<T>;
#else
		using Handle = void*;

		template <typename T>
		using Pointer = T*;

		template <typename T>
		using ConstPointer = const T*;
#endif // AQUA_DEBUG_ENABLE_REFERENCE_COUNT

	public:
		// Unchecked nothrow operator new/delete
		template <typename T>
		class NewDeleteAllocator {
		public:
			using Pointer	   = T*;
			using ConstPointer = const T*;

			template <typename U>
			struct Rebind {
				using AllocatorType = NewDeleteAllocator<U>;
			};


		public:
			Pointer Allocate(size_t count) noexcept {
				return (Pointer)::operator new (count * sizeof(T), std::nothrow);
			}

			void Deallocate(Pointer ptr, size_t count) noexcept {
				::operator delete (ptr, count * sizeof(T));
			}

			NewDeleteAllocator OnDataStructureCopy() const noexcept {
				return NewDeleteAllocator();
			}
		};

		// Nothrow operator new/delete
		class BootstrapAllocator {
		public:
			template <typename T>
			struct Proxy {
			public:
				using Pointer	   = typename MemorySystem::template Pointer<T>;
				using ConstPointer = typename MemorySystem::template ConstPointer<T>;

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
					return static_cast<Pointer>(BootstrapAllocator().Allocate(sizeof(T) * count));
				}

				void Deallocate(Pointer ptr, size_t count) const noexcept {
					return BootstrapAllocator().Deallocate(ptr, sizeof(T) * count);
				}

				void DeallocateBytes(Pointer ptr, size_t bytes) const noexcept {
					return BootstrapAllocator().Deallocate(ptr, bytes);
				}

				Proxy OnDataStructureCopy() const noexcept { return Proxy(); }
			}; // struct Proxy

		public:
			Handle Allocate(size_t bytes) const noexcept;
			void Deallocate(Handle handle, size_t bytes) const noexcept;

			// In release build 'AllocatePtr' is the same as 'Allocate'
			// It used only in 'DebugPointer' implementation in debug build
			void* AllocatePtr(size_t bytes) const noexcept;

			// In release build 'DeallocatePtr' is the same as 'Deallocate'
			// It used only in 'DebugPointer' implementation in debug build
			void DeallocatePtr(void* ptr, size_t bytes) const noexcept;
		}; // class BootstrapAllocator

		// Main engine allocator
		class GlobalAllocator {
		public:
			// Empty proxy-allocator object to access 'GlobalAllocator'
			template <typename T>
			struct Proxy {
			public:
				using Pointer	   = typename MemorySystem::template Pointer<T>;
				using ConstPointer = typename MemorySystem::template ConstPointer<T>;

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
						MemorySystem::_GetConstGlobalAllocator().Allocate(sizeof(T) * count));
				}

				void Deallocate(Pointer ptr, size_t count) const noexcept {
					return MemorySystem::_GetConstGlobalAllocator().Deallocate(ptr, sizeof(T) * count);
				}

				void DeallocateBytes(Pointer ptr, size_t bytes) const noexcept {
					return MemorySystem::_GetConstGlobalAllocator().Deallocate(ptr, bytes);
				}

				Proxy OnDataStructureCopy() const noexcept { return Proxy(); }
			}; // struct Proxy

		public:
			Handle Allocate(size_t bytes) const noexcept;
			void Deallocate(Handle handle, size_t bytes) const noexcept;
		}; // class GlobalAllocator

		// Store first 'InlineSize' objects inline
		// If object count > 'InlineSize' store in allocated memory
		template <typename T, size_t InlineSize>
		class InlineAllocator {
		public:
			static_assert(InlineSize > 0, "aqua::MemorySystem::InlineAllocator - InlineSize must be > 0");

		public:
			using Pointer	   = typename MemorySystem::template Pointer<T>;
			using ConstPointer = typename MemorySystem::template ConstPointer<T>;

			template <typename U>
			struct Rebind {
				using AllocatorType = InlineAllocator<U, InlineSize>;
			};

		public:
			InlineAllocator()			            noexcept = default;
			InlineAllocator(const InlineAllocator&) noexcept = default;
			InlineAllocator(InlineAllocator&&)      noexcept = default;

			InlineAllocator& operator=(const InlineAllocator&) noexcept = default;
			InlineAllocator& operator=(InlineAllocator&&)	   noexcept = default;

			template <typename U>
			InlineAllocator(const InlineAllocator<U, InlineSize>&) noexcept
				requires(std::is_base_of_v<T, U>&& std::is_polymorphic_v<U>) {};

			template <typename U>
			InlineAllocator(InlineAllocator<U, InlineSize>&&) noexcept
				requires(std::is_base_of_v<T, U>&& std::is_polymorphic_v<U>) {};

			template <typename U>
			InlineAllocator& operator=(const InlineAllocator<U, InlineSize>&) noexcept
				requires(std::is_base_of_v<T, U>&& std::is_polymorphic_v<U>) {
				return *this;
			}

			template <typename U>
			InlineAllocator& operator=(InlineAllocator<U, InlineSize>&&) noexcept
				requires(std::is_base_of_v<T, U>&& std::is_polymorphic_v<U>) {
				return *this;
			}

		public:
			Pointer Allocate(size_t count) const noexcept {
				if (m_inlineUsed + count > InlineSize) {
					return static_cast<Pointer>(
						MemorySystem::_GetConstGlobalAllocator().Allocate(sizeof(T) * count));
				}
#if AQUA_DEBUG_ENABLE_REFERENCE_COUNT
				return Pointer((T*)m_inlineStorage + m_inlineUsed, false);
#else
				return (T*)m_inlineStorage + m_inlineUsed;
#endif // AQUA_DEBUG_ENABLE_REFERENCE_COUNT
			}

			void Deallocate(Pointer ptr, size_t count) const noexcept {
				if (!(ptr >= (T*)m_inlineStorage && ptr < ((T*)m_inlineStorage + InlineSize))) {
					MemorySystem::_GetConstGlobalAllocator().Deallocate(ptr, sizeof(T) * count);
				}
			}

			void DeallocateBytes(Pointer ptr, size_t bytes) const noexcept {
				if (!(ptr >= (T*)m_inlineStorage && ptr < ((T*)m_inlineStorage + InlineSize))) {
					MemorySystem::_GetConstGlobalAllocator().Deallocate(ptr, bytes);
				}
			}

			InlineAllocator OnDataStructureCopy() const noexcept { return InlineAllocator(); }

		private:
			size_t m_inlineUsed = 0;
			alignas(T) std::byte m_inlineStorage[sizeof(T) * InlineSize] = {};
		}; // class InlineAllocator

		template <typename T>
		friend struct GlobalAllocator::Proxy;

		template <typename T, size_t InlineSize>
		friend class InlineAllocator;

	public:
		~MemorySystem();

		MemorySystem(const MemorySystem&) = delete;
		MemorySystem(MemorySystem&&) noexcept = delete;

		MemorySystem& operator=(const MemorySystem&) = delete;
		MemorySystem& operator=(MemorySystem&&) noexcept = delete;

	private:
		static MemorySystem::GlobalAllocator&       _GetGlobalAllocator()	  noexcept;
		static const MemorySystem::GlobalAllocator& _GetConstGlobalAllocator() noexcept;

#if AQUA_SUPPORT_VULKAN_RENDER_API
		friend class VulkanAPI;
		static void* _GetVulkanAllocationCallbacks() noexcept;
#endif // AQUA_SUPPORT_VULKAN_RENDER_API

	private:
		friend class Engine;
		MemorySystem(Status& status);

	private:
		GlobalAllocator m_globalAllocator;
	};
	// class EngineMemorySystem
} // namespace aqua

#endif // AQUA_MEMORY_SYSTEM_HEADER