#include <aqua/engine/MemorySystem.h>

namespace {
	aqua::EngineMemorySystem* g_MemorySystem = nullptr;
}

void* aqua::EngineMemorySystem::_BootstrapAllocator::Allocate(size_t bytes) const noexcept {
	return ::operator new (bytes, std::nothrow);
}

void aqua::EngineMemorySystem::_BootstrapAllocator::Deallocate(void* ptr, size_t bytes) const noexcept {
	::operator delete (ptr, bytes);
}

void* aqua::EngineMemorySystem::GlobalAllocator::Allocate(size_t bytes) const noexcept {
	return _BootstrapAllocator().Allocate(bytes);
}

void aqua::EngineMemorySystem::GlobalAllocator::Deallocate(void* ptr, size_t bytes) const noexcept {
	_BootstrapAllocator().Deallocate(ptr, bytes);
}

aqua::EngineMemorySystem::EngineMemorySystem(Status& status) {

}

aqua::EngineMemorySystem::GlobalAllocator& aqua::EngineMemorySystem::_GetGlobalAllocator() noexcept {
	return g_MemorySystem->m_globalAllocator;
}

const aqua::EngineMemorySystem::GlobalAllocator& aqua::EngineMemorySystem::_GetConstGlobalAllocator() noexcept {
	return g_MemorySystem->m_globalAllocator;
}