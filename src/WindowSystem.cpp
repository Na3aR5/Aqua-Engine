#include <aqua/engine/WindowSystem.h>
#include <aqua/engine/EventSystem.h>

#include <aqua/Logger.h>
#include <aqua/Assert.h>

#if AQUA_SUPPORT_GLFW_WINDOW_API
	#include <aqua/engine/graphics/wapi/GLFWAPI.h>
#endif // AQUA_SUPPORT_GLFW_WINDOW_API

namespace {
	template <typename WindowAPI>
	void LoadWindowSystemFunctions(aqua::WindowSystem* instance) {
		instance->CreateVulkanWindowSurface			  = WindowAPI::CreateVulkanWindowSurface;
		instance->GetVulkanRequiredInstanceExtensions = WindowAPI::GetVulkanRequiredInstanceExtensions;
		instance->GetRenderWindowFramebufferSize	  = WindowAPI::GetRenderWindowFramebufferSize;
	}

	template <typename WindowAPI, typename PrivateInterface>
	void LoadPrivateInterface(PrivateInterface* privateInterface) {
		privateInterface->PollEvents = WindowAPI::PollEvents;
	}

	aqua::WindowSystem* g_WindowSystem = nullptr;
}

aqua::WindowSystem::WindowSystem(const Config& config, Status& status) :
m_windowAPI(config.GetEngineInfo().external.windowSystem.windowAPI) {
	AQUA_ASSERT(g_WindowSystem == nullptr, Literal("Attempt to create another WindowSystem instance"));

	if (!status.IsSuccess() || g_WindowSystem != nullptr) {
		return;
	}
	
	switch (m_windowAPI) {
#if AQUA_SUPPORT_GLFW_WINDOW_API
		case WindowAPI::GLFW: {
			MemorySystem::Pointer<GLFW_API> glfwAPI =
				MemorySystem::GlobalAllocator::Proxy<GLFW_API>().Allocate(1);

			if (glfwAPI == nullptr) {
				status.EmplaceError(Error::FAILED_TO_ALLOCATE_MEMORY);
				return;
			}
			m_API = glfwAPI;
			new (m_API) GLFW_API(config, status);

			LoadWindowSystemFunctions<GLFW_API>(this);
			LoadPrivateInterface<GLFW_API>(&m_private);

			break;
		}
	}

	g_WindowSystem = this;
#endif AQUA_SUPPORT_GLFW_WINDOW_API
}

aqua::WindowSystem::~WindowSystem() {
	if (m_API != nullptr) {
		switch (m_windowAPI) {
#if AQUA_SUPPORT_GLFW_WINDOW_API
			case WindowAPI::GLFW: {
				MemorySystem::Pointer<GLFW_API> glfwAPI =
					static_cast<MemorySystem::Pointer<GLFW_API>>(m_API);

				glfwAPI->~GLFW_API();
				MemorySystem::GlobalAllocator::Proxy<GLFW_API>().Deallocate(glfwAPI, 1);
				glfwAPI = nullptr;

				break;
			}
#endif // AQUA_SUPPORT_GLFW_WINDOW_API
		}
	}
}

const aqua::WindowSystem& aqua::WindowSystem::Get() noexcept {
	return *g_WindowSystem;
}