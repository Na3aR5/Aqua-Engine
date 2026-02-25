#ifndef AQUA_GLFW_API_HEADER
#define AQUA_GLFW_API_HEADER

#include <aqua/Error.h>
#include <aqua/Config.h>
#include <aqua/math/Vector.h>
#include <aqua/engine/ForwardSystems.h>

#include <aqua/datastructures/Array.h>

#if AQUA_SUPPORT_VULKAN_RENDER_API
	#define GLFW_INCLUDE_VULKAN
#endif // AQUA_SUPPORT_VULKAN_RENDER_API

#include <GLFW/glfw3.h>

namespace aqua {
	class GLFW_API {
	public:
		~GLFW_API();

		GLFW_API(const GLFW_API&) = delete;
		GLFW_API(GLFW_API&&) noexcept = delete;

		GLFW_API& operator=(const GLFW_API&) = delete;
		GLFW_API& operator=(GLFW_API&&) noexcept = delete;

	public:
		static Expected<void*, Error> CreateVulkanWindowSurface(void* vkInstance, void* allocator) noexcept;
		static Expected<aqua::SafeArray<const char*>, Error> GetVulkanRequiredInstanceExtensions() noexcept;

		static void PollEvents() noexcept;
		static Vec2i GetRenderWindowFramebufferSize() noexcept;


	private:
		aqua::Status _CreateRenderWindow() noexcept;
		void _PollEvents() const noexcept;
		void _Terminate() noexcept;

	private:
		static void _WindowCloseCallback(GLFWwindow*);

	private:
		friend WindowSystem;
		GLFW_API(const Config& config, Status& status);

	private:
		const Config& m_config;
		GLFWwindow*   m_renderWindow = nullptr;
	}; // class GLFW_API
} // namespace aqua

#endif // !AQUA_GLFW_API_HEADER