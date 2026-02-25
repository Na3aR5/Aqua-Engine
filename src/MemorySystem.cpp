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

aqua::MemorySystem::Handle aqua::MemorySystem::GlobalAllocator::Allocate(size_t bytes) const noexcept {
	AQUA_LOG_MEMORY(Literal("Allocated {} bytes"), bytes);
	return Handle(BootstrapAllocator().Allocate(bytes));
}

void aqua::MemorySystem::GlobalAllocator::Deallocate(Handle handle, size_t bytes) const noexcept {
	AQUA_LOG_WARNING_IF(handle == nullptr, Literal("Attempt to deallocate nullptr"));

#if AQUA_DEBUG_ENABLE_REFERENCE_COUNT
	if (handle != nullptr) {
		auto* counter = handle.GetCounter();
		if (counter != nullptr && !counter->IsAlive()) {
			AQUA_ASSERT(false, Literal("Call deallocate on unallocated memory"));
			return;
		}
		if (counter != nullptr) {
			counter->SetAlive(false);
		}
	}
#endif // AQUA_DEBUG_ENABLE_REFERENCE_COUNT
	BootstrapAllocator().Deallocate(handle, bytes);

	AQUA_LOG_MEMORY_IF(ptr != nullptr, Literal("Deallocated {} bytes"), bytes);
}

aqua::MemorySystem::MemorySystem(Status& status) {
	AQUA_ASSERT(g_MemorySystem == nullptr, Literal("Attempt to create another MemorySystem instance"));

	if (!status.IsSuccess() || g_MemorySystem != nullptr) {
		return;
	}
	g_MemorySystem = this;

	AQUA_LOG(Literal("Engine memory system is initialized"));
}

aqua::MemorySystem::~MemorySystem() {
	g_MemorySystem = nullptr;
}

aqua::MemorySystem::GlobalAllocator& aqua::MemorySystem::_GetGlobalAllocator() noexcept {
	return g_MemorySystem->m_globalAllocator;
}

const aqua::MemorySystem::GlobalAllocator& aqua::MemorySystem::_GetConstGlobalAllocator() noexcept {
	return g_MemorySystem->m_globalAllocator;
}

// -------------------------------------------------- Vulkan --------------------------------------------------

#if AQUA_SUPPORT_VULKAN_RENDER_API
#include <vulkan/vulkan_core.h>

namespace {
	VkAllocationCallbacks g_VulkanAllocatorCallbacks = {
		.pfnAllocation = [](void* userData, size_t bytes, size_t alignment, VkSystemAllocationScope) {
			return _aligned_malloc(bytes, alignment);
		},
		.pfnReallocation = [](void* userData, void* current, size_t bytes, size_t alignment, VkSystemAllocationScope) {
			return _aligned_realloc(current, bytes, alignment);
		},
		.pfnFree = [](void* userData, void* ptr) {
			_aligned_free(ptr);
		},
		.pfnInternalAllocation = nullptr,
		.pfnInternalFree       = nullptr
	};
}

void* aqua::MemorySystem::_GetVulkanAllocationCallbacks() noexcept {
	return &g_VulkanAllocatorCallbacks;
}

#endif // AQUA_SUPPORT_VULKAN_RENDER_API