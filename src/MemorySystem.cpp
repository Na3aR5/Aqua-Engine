#include <aqua/engine/MemorySystem.h>
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

void* aqua::MemorySystem::GlobalAllocator::Allocate(size_t bytes) const noexcept {
	return BootstrapAllocator().Allocate(bytes);
}

void aqua::MemorySystem::GlobalAllocator::Deallocate(void* ptr, size_t bytes) const noexcept {
	BootstrapAllocator().Deallocate(ptr, bytes);
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
}

aqua::MemorySystem::GlobalAllocator& aqua::MemorySystem::GetGlobalAllocator() noexcept {
	return g_MemorySystem->m_globalAllocator;
}

const aqua::MemorySystem::GlobalAllocator& aqua::MemorySystem::GetConstGlobalAllocator() noexcept {
	return g_MemorySystem->m_globalAllocator;
}