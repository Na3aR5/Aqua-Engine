#ifndef AQUA_WINDOW_SYSTEM_HEADER
#define AQUA_WINDOW_SYSTEM_HEADER

#include <aqua/Error.h>
#include <aqua/Config.h>
#include <aqua/math/Vector.h>
#include <aqua/engine/Defines.h>
#include <aqua/engine/ForwardSystems.h>
#include <aqua/engine/MemorySystem.h>

#include <aqua/datastructures/Array.h>

namespace aqua {
	class WindowSystem {
	public:
		using CreateVulkanWindowSurfaceFunction			  = Expected<void*, Error>(*)(void*, void*);
		using GetVulkanRequiredInstanceExtensionsFunction = Expected<SafeArray<const char*>, Error>(*)();
		using PollEventsFunction						  = void(*)();
		using GetRenderWindowFramebufferSizeFunction	  = Vec2i(*)();

	public:
		~WindowSystem();

		WindowSystem(const WindowSystem&) = delete;
		WindowSystem(WindowSystem&&) noexcept = delete;

		WindowSystem& operator=(const WindowSystem&) = delete;
		WindowSystem& operator=(WindowSystem&&) noexcept = delete;

	public:
		static const WindowSystem& Get() noexcept;

	public:
		CreateVulkanWindowSurfaceFunction		    CreateVulkanWindowSurface	        = nullptr;
		GetVulkanRequiredInstanceExtensionsFunction GetVulkanRequiredInstanceExtensions = nullptr;
		GetRenderWindowFramebufferSizeFunction		GetRenderWindowFramebufferSize	    = nullptr;

	private:
		struct _Private {
			PollEventsFunction PollEvents = nullptr;
		} m_private;

	private:
		friend class Engine;
		friend class EventSystem;

		WindowSystem(const Config& config, Status& status);

	private:
		WindowAPI			 m_windowAPI;
		MemorySystem::Handle m_API = nullptr;
	}; // class WindowSystem
} // namespace aqua

#endif // !AQUA_WINDOW_SYSTEM_HEADER