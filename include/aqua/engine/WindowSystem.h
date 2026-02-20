#ifndef AQUA_WINDOW_SYSTEM_HEADER
#define AQUA_WINDOW_SYSTEM_HEADER

#include <aqua/Error.h>
#include <aqua/engine/Defines.h>
#include <aqua/math/Vector.h>
#include <aqua/engine/ForwardSystems.h>

namespace aqua {
	class WindowSystem {
	public:
		~WindowSystem();

		WindowSystem(const WindowSystem&) = delete;
		WindowSystem(WindowSystem&&) noexcept = delete;

		WindowSystem& operator=(const WindowSystem&) = delete;
		WindowSystem& operator=(WindowSystem&&) noexcept = delete;

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