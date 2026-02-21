#include <aqua/engine/WindowSystem.h>
#include <aqua/engine/EventSystem.h>
#include <aqua/engine/RenderHardwareInterface.h>

#include <aqua/Logger.h>
#include <aqua/Assert.h>

#include <GLFW/glfw3.h>

namespace {
	aqua::WindowSystem* g_WindowSystem = nullptr;
}

aqua::WindowSystem::WindowSystem(const Config& config, Status& status) : m_config(config) {
	AQUA_ASSERT(g_WindowSystem == nullptr, Literal("Attempt to create another WindowSystem instance"));

	if (!status.IsSuccess() || g_WindowSystem != nullptr) {
		return;
	}
	if (!glfwInit()) {
		status.EmplaceError(Error::FAILED_TO_INITIALIZE_GLFW_LIBRARY);
		return;
	}
	aqua::Status creationStatus = _CreateMainWindow();
	if (!creationStatus.IsSuccess()) {
		_Terminate();
		status.EmplaceError(creationStatus.GetError());
		return;
	}
	g_WindowSystem = this;

	AQUA_LOG(Literal("Engine window system is initialized"));
}

aqua::WindowSystem::~WindowSystem() {
	if (m_mainWindow != nullptr) {
		glfwDestroyWindow((GLFWwindow*)m_mainWindow);
		m_mainWindow = nullptr;
	}
	_Terminate();
}

aqua::WindowSystem& aqua::WindowSystem::Get() noexcept { return *g_WindowSystem; }
const aqua::WindowSystem& aqua::WindowSystem::GetConst() noexcept { return *g_WindowSystem; }

void aqua::WindowSystem::_Terminate() noexcept {
	glfwTerminate();
	g_WindowSystem = nullptr;
}

aqua::Status aqua::WindowSystem::_CreateMainWindow() noexcept {
	const WindowSystemInfo& info = m_config.GetEngineInfo().external.windowSystem;

	if (info.mainWindowSize.x == 0 || info.mainWindowSize.y == 0 || info.mainWindowTitle == nullptr) {
		return Error::INCORRECT_WINDOW_RESOLUTION_OR_TITLE_IS_NULLPTR;
	}
	
	RHI::Static::SetMainWindowHints(m_config.GetEngineInfo().external.renderAPI);
	GLFWwindow* newMainWindow = glfwCreateWindow(info.mainWindowSize.x, info.mainWindowSize.y,
		info.mainWindowTitle, nullptr, nullptr);

	if (newMainWindow == nullptr) {
		return Error::FAILED_TO_CREATE_WINDOW_INSTANCE;
	}

	AQUA_LOG(Literal("Main window is created"));

	EventSystem::Get()._SetWindowCallbacks(newMainWindow, info.m_mainWindowEventSet);
	m_mainWindow = newMainWindow;

	return Success{};
}

void* aqua::WindowSystem::_GetMainWindowPtr() const noexcept {
	return m_mainWindow;
}