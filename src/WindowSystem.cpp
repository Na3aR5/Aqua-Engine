#include <aqua/engine/WindowSystem.h>
#include <aqua/engine/EventSystem.h>
#include <aqua/engine/RenderHardwareInterface.h>

#include <aqua/Logger.h>
#include <aqua/Assert.h>

#include <GLFW/glfw3.h>

namespace {
	aqua::WindowSystem* g_WindowSystem = nullptr;
}

aqua::WindowSystem::WindowSystem(const WindowSystemInfo& info, Status& status) : m_info(info) {
	AQUA_ASSERT(g_WindowSystem == nullptr, Literal("Attempt to create another WindowSystem instance"));

	if (!status.IsSuccess() || g_WindowSystem != nullptr) {
		return;
	}
	if (!glfwInit()) {
		status.EmplaceError(Error::FAILED_TO_INITIALIZE_GLFW_LIBRARY);
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

void aqua::WindowSystem::_Terminate() noexcept {
	glfwTerminate();
	g_WindowSystem = nullptr;
}

aqua::Status aqua::WindowSystem::_CreateMainWindow() noexcept {
	if (m_info.mainWindowSize.x == 0 || m_info.mainWindowSize.y == 0 || m_info.mainWindowTitle == nullptr) {
		return Error::INCORRECT_WINDOW_RESOLUTION_OR_TITLE_IS_NULLPTR;
	}
	
	RHI::SetMainWindowHints();
	GLFWwindow* newMainWindow = glfwCreateWindow(m_info.mainWindowSize.x, m_info.mainWindowSize.y,
		m_info.mainWindowTitle, nullptr, nullptr);

	if (newMainWindow == nullptr) {
		return Error::FAILED_TO_CREATE_WINDOW_INSTANCE;
	}

	AQUA_LOG(Literal("Main window is created"));

	EventSystem::Get()._SetWindowCallbacks(newMainWindow, m_info.m_mainWindowEventSet);
	m_mainWindow = newMainWindow;

	return Success{};
}