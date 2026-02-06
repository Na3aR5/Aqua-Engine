#ifndef AQUA_APP_WINDOW_HEADER
#define AQUA_APP_WINDOW_HEADER

#include <GLFW/glfw3.h>

#include <aqua/Error.h>
#include <aqua/math/Vector.h>

namespace aqua {
	class Engine;

	struct WindowInfo {
		const char* title = nullptr;
		Vec2i       size{};
	}; // struct WindowInfo

	class Window {
	private:
		friend class Engine;
		Window() = default;

		Status _Create(const WindowInfo& info) noexcept;
		void   _Destroy() noexcept;

	private:
		GLFWwindow* m_window = nullptr;
	}; // class Window
} // namespace aqua

#endif // !AQUA_APP_WINDOW_HEADER