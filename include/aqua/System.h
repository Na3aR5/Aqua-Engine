#ifndef AQUA_SYSTEM_HEADER
#define AQUA_SYSTEM_HEADER

#include <aqua/Error.h>
#include <aqua/engine/ForwardSystems.h>
#include <aqua/datastructures/HandleSet.h>

#include <cstdint>

namespace aqua {
	// OS API
	class System {
	public:
		template <typename T>
		using AllocatorType = aqua::MemorySystem::GlobalAllocator::Proxy<T>;

		template <typename T>
		class Handle {
		public:
			Handle() noexcept = default;
			Handle(aqua::HandleSet<int>::Handle handle, aqua::MemorySystem::Pointer<T> ptr) : m_handle(handle), m_ptr(ptr) {}

		public:
			T& operator*() noexcept { return *m_ptr; }
			const T& operator*() const noexcept { return *m_ptr; }

			aqua::MemorySystem::Pointer<T> operator->() noexcept { return m_ptr; }
			const aqua::MemorySystem::Pointer<T> operator->() const noexcept { return m_ptr; }

			bool operator==(const Handle& other) const noexcept {
				return m_handle == other.m_handle && m_ptr == other.m_ptr;
			}

		private:
			friend class System;
			aqua::HandleSet<int>::Handle   m_handle;
			aqua::MemorySystem::Pointer<T> m_ptr = nullptr;
		};

	public:
		struct ChildProcessCreateInfo {
		public:
			enum {
				ENABLE_WRITE_BIT = 0x1,
				ENABLE_READ_BIT  = 0x2
			};

		public:
			const char* executablePath = nullptr;
			uint32_t    flags		   = 0;
		}; // struct ChildProcessCreateInfo

		class IChildProcess {
		public:
			virtual ~IChildProcess() = default;

			virtual Status Create(const ChildProcessCreateInfo&) noexcept = 0;
			virtual void Destroy() noexcept = 0;
			virtual Status Write(const char* buffer, size_t size) noexcept = 0;
		}; // class IChildProcess

	public:
		template <typename ResourceType>
		aqua::MemorySystem::Pointer<ResourceType> Get(Handle<ResourceType> handle) noexcept {
			return handle.m_ptr;
		}

		template <typename ResourceType>
		bool IsValid(const Handle<ResourceType>&) const noexcept = delete;

		template <>
		bool IsValid(const Handle<IChildProcess>& handle) const noexcept {
			return m_childProcessRegister.IsValid(handle.m_handle);
		}

		template <typename ResourceType, typename ResourceTypeCreateInfo>
		Expected<Handle<ResourceType>, Error> Create(const ResourceTypeCreateInfo& info) noexcept {
			return _Create(info);
		}

		template <typename ResourceType>
		void Destroy(const Handle<ResourceType>&) noexcept = delete;

		template <>
		void Destroy(const Handle<IChildProcess>& handle) noexcept {
			m_childProcessRegister.Remove(handle.m_handle);
		}

	public:
		~System();

		static System& Get() noexcept;
		static const System& GetConst() noexcept;

	private:
		Expected<Handle<IChildProcess>, Error> _Create(const ChildProcessCreateInfo& info) noexcept;

	private:
		friend class Engine;
		System(Status&);

	private:
		template <typename T>
		using _AbstractDataRegister = aqua::HandleSet<aqua::UniqueData<T, AllocatorType<T>>,
			AllocatorType<aqua::UniqueData<T, AllocatorType<T>>>>;

		_AbstractDataRegister<IChildProcess> m_childProcessRegister;
	}; // class System
} // namespace System

#endif // !AQUA_SYSTEM_HEADER