#ifndef AQUA_WINDOW_SYSTEM_HEADER
#define AQUA_WINDOW_SYSTEM_HEADER

#include <aqua/engine/EventSystem.h>
#include <aqua/math/Vector.h>

namespace aqua {
	struct WindowSystemInfo {
		const char* mainWindowTitle = nullptr;
		Vec2i       mainWindowSize{};

		// Optional
		EventSet    m_mainWindowEventSet{};
	}; // struct WindowInfo

	class WindowSystem {
	public:
		~WindowSystem();

	private:
		void _Terminate() noexcept;
		Status _CreateMainWindow() noexcept;

	private:
		friend class Engine;
		WindowSystem(const WindowSystemInfo& info, Status& status);

	private:
		WindowSystemInfo m_info;
		void*            m_mainWindow = nullptr;
	}; // class WindowSystem
} // namespace aqua

#endif // !AQUA_WINDOW_SYSTEM_HEADER