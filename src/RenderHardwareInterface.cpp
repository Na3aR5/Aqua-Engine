#include <aqua/pch.h>
#include <aqua/engine/RenderHardwareInterface.h>
#include <aqua/engine/WindowSystem.h>
#include <aqua/utility/Memory.h>
#include <aqua/datastructures/Array.h>

#include <aqua/Assert.h>
#include <aqua/Logger.h>

#if AQUA_SUPPORT_VULKAN_RENDER_API
	#include <aqua/engine/graphics/api/VulkanAPI.h>
#endif // AQUA_SUPPORT_VULKAN_RENDER_API

namespace {
	template <typename RenderAPI>
	void LoadRenderHardwareInterfaceFunctions() {

	}

	aqua::RenderHardwareInterface* g_RHI = nullptr;
}

const aqua::RenderHardwareInterface& aqua::RenderHardwareInterface::Get() noexcept { return *g_RHI; }

aqua::RenderHardwareInterface::RenderHardwareInterface(const Config& config, const RenderAPICreateInfo& info, Status& status) :
m_renderAPI(config.GetEngineInfo().external.renderAPI) {
	AQUA_ASSERT(g_RHI == nullptr, Literal("Attempt to create another RHI instance"));

	if (!status.IsSuccess() || g_RHI != nullptr) {
		return;
	}

	switch (m_renderAPI) {
#if AQUA_SUPPORT_VULKAN_RENDER_API
		case RenderAPI::VULKAN: {
			MemorySystem::Pointer<VulkanAPI> vulkanAPI =
				MemorySystem::GlobalAllocator::Proxy<VulkanAPI>().Allocate(1);

			if (vulkanAPI == nullptr) {
				status.EmplaceError(Error::FAILED_TO_ALLOCATE_MEMORY);
				return;
			}
			new (vulkanAPI) VulkanAPI(config, info, status);

			if (!status.IsSuccess()) {
				vulkanAPI->~VulkanAPI();
				MemorySystem::GlobalAllocator::Proxy<VulkanAPI>().Deallocate(vulkanAPI, 1);
				return;
			}
			m_API = vulkanAPI;
			LoadRenderHardwareInterfaceFunctions<VulkanAPI>();

			break;
		}
#endif // AQUA_SUPPORT_VULKAN_RENDER_API
	}

	g_RHI = this;
}

aqua::RenderHardwareInterface::~RenderHardwareInterface() {
	if (m_API != nullptr) {
		switch (m_renderAPI) {
#if AQUA_SUPPORT_VULKAN_RENDER_API
			case RenderAPI::VULKAN: {
				MemorySystem::Pointer<VulkanAPI> vulkanAPI =
					static_cast<MemorySystem::Pointer<VulkanAPI>>(m_API);

				vulkanAPI->~VulkanAPI();
				MemorySystem::GlobalAllocator::Proxy<VulkanAPI>().Deallocate(vulkanAPI, 1);
				m_API = nullptr;

				break;
			}
#endif AQUA_SUPPORT_VULKAN_RENDER_API
		}
	}
}