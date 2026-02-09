#include <aqua/engine/GraphicsAPI.h>

#if AQUA_OPENGL_GRAPHICS_API
	#include <glad/glad.h>
#endif // AQUA_OPENGL_GRAPHICS_API

#include <GLFW/glfw3.h>

aqua::GraphicsAPI_OpenGL aqua::GraphicsAPI_OpenGL::Get() noexcept {
	return GraphicsAPI_OpenGL();
}

const aqua::GraphicsAPI_OpenGL aqua::GraphicsAPI_OpenGL::GetConst() noexcept {
	return GraphicsAPI_OpenGL();
}

int aqua::GraphicsAPI_OpenGL::LoadOpenGL() {
	return gladLoadGL((GLADloadfunc)glfwGetProcAddress);
}

void aqua::GraphicsAPI_OpenGL::ClearColorBuffer() noexcept {
	glClear(GL_COLOR_BUFFER_BIT);
}