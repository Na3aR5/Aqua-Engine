#include <aqua/engine/MemorySystem.h>
#include <aqua/Logger.h>
#include <aqua/Assert.h>

namespace {
	aqua::MemorySystem* g_MemorySystem = nullptr;
}

void* aqua::MemorySystem::BootstrapAllocator::Allocate(size_t bytes) const noexcept {
	return ::operator new (bytes, std::nothrow);
}

void aqua::MemorySystem::BootstrapAllocator::Deallocate(void* ptr, size_t bytes) const noexcept {
	::operator delete (ptr, bytes);
}

aqua::MemorySystem::VoidPointer aqua::MemorySystem::GlobalAllocator::Allocate(size_t bytes) const noexcept {
	AQUA_LOG_MEMORY(Literal("Allocated {} bytes"), bytes);
	return VoidPointer(BootstrapAllocator().Allocate(bytes));
}

void aqua::MemorySystem::GlobalAllocator::Deallocate(VoidPointer ptr, size_t bytes) const noexcept {
	AQUA_LOG_WARNING_IF(ptr == nullptr, Literal("Attempt to deallocate nullptr"));

#if AQUA_DEBUG_ENABLE_REFERENCE_COUNT
	if (ptr != nullptr) {
		auto* counter = ptr.GetCounter();
		if (counter != nullptr && !counter->alive && counter->references > 0) {
			AQUA_ASSERT(false, Literal("Call deallocate on deallocated memory"));
			return;
		}
		if (counter != nullptr) {
			counter->alive = false;
		}
	}
#endif // AQUA_DEBUG_ENABLE_REFERENCE_COUNT
	BootstrapAllocator().Deallocate(ptr, bytes);

	AQUA_LOG_MEMORY_IF(ptr != nullptr, Literal("Deallocated {} bytes"), bytes);
}

aqua::MemorySystem::MemorySystem(Status& status) {
	AQUA_ASSERT(g_MemorySystem == nullptr, Literal("Attempt to create another MemorySystem instance"));

	if (!status.IsSuccess()) {
		return;
	}
	if (g_MemorySystem != nullptr) {
		return;
	}
	g_MemorySystem = this;

	AQUA_LOG(Literal("Engine memory system is initialized"));
}

aqua::MemorySystem::~MemorySystem() {
	g_MemorySystem = nullptr;
}

aqua::MemorySystem::GlobalAllocator& aqua::MemorySystem::GetGlobalAllocator() noexcept {
	return g_MemorySystem->m_globalAllocator;
}

const aqua::MemorySystem::GlobalAllocator& aqua::MemorySystem::GetConstGlobalAllocator() noexcept {
	return g_MemorySystem->m_globalAllocator;
}