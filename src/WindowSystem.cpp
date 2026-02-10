#include <aqua/engine/WindowSystem.h>
#include <aqua/engine/LayerSystem.h>
#include <aqua/engine/GraphicsAPI.h>

#include <aqua/Logger.h>
#include <aqua/Assert.h>

#include <GLFW/glfw3.h>

namespace {
	aqua::WindowSystem* g_WindowSystem = nullptr;
}

aqua::WindowSystem::WindowSystem(const WindowSystemInfo& info, Status& status) : m_info(info) {
	AQUA_ASSERT(g_WindowSystem == nullptr, Literal("Attempt to create another WindowSystem instance"));

	if (!status.IsSuccess()) {
		return;
	}
	if (g_WindowSystem != nullptr) {
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
	}
	_Terminate();
	g_WindowSystem = nullptr;
}

void aqua::WindowSystem::_Terminate() noexcept {
	glfwTerminate();
}

aqua::Status aqua::WindowSystem::_CreateMainWindow() noexcept {
	if (m_info.mainWindowSize.x == 0 || m_info.mainWindowSize.y == 0 || m_info.mainWindowTitle == nullptr) {
		return Unexpected<Error>(Error::INCORRECT_WINDOW_RESOLUTION_OR_TITLE_IS_NULLPTR);
	}

#if AQUA_OPENGL_GRAPHICS_API
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, GraphicsAPI_OpenGL::OPENGL_VERSION_MAJOR);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, GraphicsAPI_OpenGL::OPENGL_VERSION_MINOR);
#endif // AQUA_OPENGL_GRAPHICS_API

	GLFWwindow* newMainWindow = glfwCreateWindow(m_info.mainWindowSize.x, m_info.mainWindowSize.y,
		m_info.mainWindowTitle, nullptr, nullptr);
	if (newMainWindow == nullptr) {
		return Unexpected<Error>(Error::FAILED_TO_CREATE_WINDOW_INSTANCE);
	}
	glfwMakeContextCurrent(newMainWindow);

	AQUA_LOG(Literal("Main window is created"));

#if AQUA_OPENGL_GRAPHICS_API
	if (!GraphicsAPI_OpenGL::LoadOpenGL()) {
		glfwDestroyWindow(newMainWindow);
		return Unexpected<Error>(Error::FAILED_TO_LOAD_OPENGL_LIBRARY);
	}
	AQUA_LOG(Literal("OpenGL is loaded"));
#endif // AQUA_OPENGL_GRAPHICS_API

	EventSystem::Get()._SetWindowCallbacks(newMainWindow, m_info.m_mainWindowEventSet);

	m_mainWindow = newMainWindow;

	return Success{};
}