#include <glad/glad.h>

#include <aqua/engine/Window.h>
#include <aqua/Assert.h>

aqua::Status aqua::Window::_Create(const WindowInfo& info) noexcept {
	if (info.size.x == 0 || info.size.y == 0 || info.title == nullptr) {
		return Unexpected<Error>(Error::INCORRECT_WINDOW_RESOLUTION_OR_TITLE_IS_NULLPTR);
	}
	
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, AQUA_OPENGL_VERSION_MAJOR);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, AQUA_OPENGL_VERSION_MINOR);
	m_window = glfwCreateWindow(info.size.x, info.size.y, info.title, nullptr, nullptr);

	if (m_window == nullptr) {
		return Unexpected<Error>(Error::FAILED_TO_CREATE_WINDOW_INSTANCE);
	}
	glfwMakeContextCurrent(m_window);

	if (!gladLoadGL((GLADloadfunc)glfwGetProcAddress)) {
		glfwDestroyWindow(m_window);
		m_window = nullptr;
		return Unexpected<Error>(Error::FAILED_TO_LOAD_OPENGL_LIBRARY);
	}

	return Success{};
}

void aqua::Window::_Destroy() noexcept {
	if (m_window != nullptr) {
		gladLoaderUnloadGL();
		glfwDestroyWindow(m_window);
	}
}