#ifndef AQUA_VULKAN_API_HEADER
#define AQUA_VULKAN_API_HEADER

#include <aqua/Build.h>
#include <aqua/Error.h>
#include <aqua/Config.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace aqua {
	class RenderHardwareInterface;

	class VulkanAPI {
	public:
		using GPU = VkPhysicalDevice;

	public:
		~VulkanAPI();

		VulkanAPI(const VulkanAPI&) = delete;
		VulkanAPI(VulkanAPI&&) noexcept = delete;

		VulkanAPI& operator=(const VulkanAPI&) = delete;
		VulkanAPI& operator=(VulkanAPI&&) noexcept = delete;

	public:
		static void SetMainWindowHints() noexcept;

	private:
		friend class RenderHardwareInterface;
		VulkanAPI(
			const Config& config,
			Status& status
		);

	private:
		aqua::Status _CreateVulkanInstance() noexcept;
		aqua::Status _PickGPU() noexcept;

#if AQUA_VULKAN_ENABLE_VALIDATION_LAYERS
		aqua::Status _CreateDebugMessenger() noexcept;
		void _DestroyDebugMessenger() noexcept;
#endif // AQUA_VULKAN_ENABLE_VALIDATION_LAYERS

		void _DestroyVulkanInstance() noexcept;
		void _UnbindGPU() noexcept;

		void _Terminate() noexcept;

	private:
		const Config&			 m_config;
		size_t					 m_initializeCount = 0;

		VkAllocationCallbacks*   m_allocator	   = nullptr;

		VkInstance			     m_instance		   = VK_NULL_HANDLE;
		VkDebugUtilsMessengerEXT m_debugMessenger  = VK_NULL_HANDLE;
		GPU                      m_GPU			   = VK_NULL_HANDLE;
	}; // class VulkanAPI
} // namespace aqua

#endif // !AQUA_VULKAN_API_HEADER