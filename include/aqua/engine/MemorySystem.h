#ifndef AQUA_MEMORY_SYSTEM_HEADER
#define AQUA_MEMORY_SYSTEM_HEADER

#include <aqua/Error.h>

#include <new>
#include <type_traits>

namespace aqua {
	class Engine;

	// System for managing heap memory, which Engine needs
	class EngineMemorySystem {
	public:
		// Main Engine allocator
		class GlobalAllocator {
		public:
			// Empty proxy-allocator object to access 'GlobalAllocator'
			template <typename T>
			struct Proxy {
			public:
				using Pointer = T*;

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
					return reinterpret_cast<Pointer>(
						EngineMemorySystem::_GetConstGlobalAllocator().Allocate(sizeof(T) * count));
				}

				void Deallocate(Pointer ptr, size_t count) const noexcept {
					return EngineMemorySystem::_GetConstGlobalAllocator().Deallocate(ptr, sizeof(T) * count);
				}

				Proxy OnDataStructureCopy() const noexcept { return Proxy(); }
			}; // class Proxy

		public:
			void* Allocate(size_t bytes) const noexcept;
			void Deallocate(void* ptr, size_t bytes) const noexcept;
		}; // class GlobalAllocator

		template <typename T>
		friend struct GlobalAllocator::Proxy;

	public:
		EngineMemorySystem(const EngineMemorySystem&) = delete;
		EngineMemorySystem(EngineMemorySystem&&) noexcept = delete;

		EngineMemorySystem& operator=(const EngineMemorySystem&) = delete;
		EngineMemorySystem& operator=(EngineMemorySystem&&) noexcept = delete;

	private:
		static EngineMemorySystem::GlobalAllocator&       _GetGlobalAllocator()	     noexcept;
		static const EngineMemorySystem::GlobalAllocator& _GetConstGlobalAllocator() noexcept;

	private:
		// Nothrow operator new/delete
		class _BootstrapAllocator {
		public:
			void* Allocate(size_t bytes) const noexcept;
			void Deallocate(void* ptr, size_t bytes) const noexcept;
		}; // class _BootstrapAllocator

	private:
		friend class Engine;
		EngineMemorySystem(Status& status);

	private:
		GlobalAllocator m_globalAllocator;
	}; // class EngineMemorySystem
} // namespace aqua

#endif // AQUA_MEMORY_SYSTEM_HEADER