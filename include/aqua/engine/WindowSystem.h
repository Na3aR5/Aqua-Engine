#ifndef AQUA_WINDOW_SYSTEM_HEADER
#define AQUA_WINDOW_SYSTEM_HEADER

#include <aqua/Error.h>
#include <aqua/Config.h>
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

	public:
		static WindowSystem& Get() noexcept;
		static const WindowSystem& GetConst() noexcept;

	private:
		void _Terminate() noexcept;
		Status _CreateMainWindow() noexcept;

		void* _GetMainWindowPtr() const noexcept;

	private:
		friend class Engine;
		friend class VulkanAPI;

		WindowSystem(const Config& config, Status& status);

	private:
		const Config& m_config;
		void*         m_mainWindow = nullptr;
	}; // class WindowSystem
} // namespace aqua

#endif // !AQUA_WINDOW_SYSTEM_HEADER