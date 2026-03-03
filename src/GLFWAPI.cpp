#include <aqua/pch.h>
#include <aqua/engine/graphics/wapi/GLFWAPI.h>
#include <aqua/engine/EventSystem.h>
#include <aqua/engine/WindowSystem.h>

#include <aqua/Assert.h>
#include <aqua/Logger.h>

namespace {
	aqua::GLFW_API* g_GLFWAPI = nullptr;
}

aqua::GLFW_API::GLFW_API(const Config& config, Status& status) : m_config(config) {
	AQUA_ASSERT(g_GLFWAPI == nullptr, Literal("Attempt to create another graphics API instance"));

	if (!status.IsSuccess() || g_GLFWAPI != nullptr) {
		return;
	}

	if (!glfwInit()) {
		status.EmplaceError(Error::FAILED_TO_INITIALIZE_GLFW_LIBRARY);
		return;
	}
	Status creationStatus = _CreateRenderWindow();
	if (!creationStatus.IsSuccess()) {
		_Terminate();
		status.EmplaceError(creationStatus.GetError());
		return;
	}

	AQUA_LOG(Literal("Engine window system is initialized"));

	g_GLFWAPI = this;
}

aqua::GLFW_API::~GLFW_API() { _Terminate(); }

aqua::Status aqua::GLFW_API::_CreateRenderWindow() noexcept {
	const WindowSystemInfo& info = m_config.GetEngineInfo().external.windowSystem;

	if (info.renderWindowSize.x == 0 || info.renderWindowSize.y == 0 || info.renderWindowTitle == nullptr) {
		return Error::INCORRECT_WINDOW_RESOLUTION_OR_TITLE_IS_NULLPTR;
	}
	
	switch (m_config.GetEngineInfo().external.renderAPI) {
		case RenderAPI::VULKAN:
			glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
			break;
	}

	GLFWwindow* newWindow = glfwCreateWindow(
		info.renderWindowSize.x,
		info.renderWindowSize.y,
		info.renderWindowTitle,
		nullptr,
		nullptr
	);
	if (newWindow == nullptr) {
		return Error::FAILED_TO_CREATE_WINDOW_INSTANCE;
	}
	m_renderWindow = newWindow;

	glfwSetWindowCloseCallback(m_renderWindow, _WindowCloseCallback);

	AQUA_LOG(Literal("Render window is created"));

	return Success{};
}

void aqua::GLFW_API::_PollEvents() const noexcept {

}

void aqua::GLFW_API::_Terminate() noexcept {
	if (m_renderWindow != nullptr) {
		glfwDestroyWindow(m_renderWindow);
		m_renderWindow = nullptr;

		AQUA_LOG(Literal("Render window is destroyed"));
	}
	glfwTerminate();
}

void aqua::GLFW_API::PollEvents() noexcept {
	glfwPollEvents();
}

aqua::Expected<void*, aqua::Error> aqua::GLFW_API::CreateVulkanWindowSurface(void* vkInstance, void* allocator) noexcept {
#if AQUA_SUPPORT_VULKAN_RENDER_API
	VkSurfaceKHR surface = VK_NULL_HANDLE;

	if (glfwCreateWindowSurface((VkInstance)vkInstance, g_GLFWAPI->m_renderWindow, (VkAllocationCallbacks*)allocator, &surface)) {
		return Error::VULKAN_FAILED_TO_CREATE_SURFACE;
	}
	return (void*)surface;
#else // not supported
	return (void*)nullptr;
#endif // !AQUA_SUPPORT_VULKAN_RENDER_API
}

aqua::Expected<aqua::SafeArray<const char*>, aqua::Error> aqua::GLFW_API::GetVulkanRequiredInstanceExtensions() noexcept {
#if AQUA_SUPPORT_VULKAN_RENDER_API
	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	aqua::SafeArray<const char*> extensions;
	AQUA_TRY(extensions.Set(glfwExtensions, glfwExtensions + glfwExtensionCount), _);

	return extensions;
#else // not supported
	return aqua::SafeArray<const char*>();
#endif // !AQUA_SUPPORT_VULKAN_RENDER_API
}

aqua::Vec2i aqua::GLFW_API::GetRenderWindowFramebufferSize() noexcept {
	Vec2i size;
	glfwGetFramebufferSize(g_GLFWAPI->m_renderWindow, &size.x, &size.y);
	return size;
}

void aqua::GLFW_API::_WindowCloseCallback(GLFWwindow* window) {
	EventSystem::Get()._RegisterEvent(Event::WINDOW_CLOSE, EventData());
}