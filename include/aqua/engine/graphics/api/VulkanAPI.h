#ifndef AQUA_VULKAN_API_HEADER
#define AQUA_VULKAN_API_HEADER

#include <aqua/Build.h>
#include <aqua/Error.h>
#include <aqua/Config.h>
#include <aqua/engine/ForwardSystems.h>
#include <aqua/datastructures/Array.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace aqua {
	class RenderHardwareInterface;

	class VulkanAPI {
	public:
		using GPU				   = VkPhysicalDevice;
		using GPU_Properties	   = VkPhysicalDeviceProperties;
		using GPU_Features		   = VkPhysicalDeviceFeatures;
		using GPU_MemoryProperties = VkPhysicalDeviceMemoryProperties;

	public:
		class QueueFamilyIndices {
		public:
			enum class Family {
				GRAPHICS,
				PRESENT,
				
				FAMILY_COUNT
			}; // enum Family

			enum : uint32_t {
				GRAPHICS_BIT = 1u << (uint32_t)Family::GRAPHICS,
				PRESENT_BIT  = 1u << (uint32_t)Family::PRESENT,
			};

		public:
			void SetFamilyIndex(Family family, uint32_t index) noexcept;
			uint32_t GetFamilyIndex(Family family) const noexcept;
			bool IsComplete(uint32_t familyBits) const noexcept;

		private:
			uint32_t m_isComplete						  = 0;
			uint32_t m_indices[(int)Family::FAMILY_COUNT] = {};
		}; // class QueueFamilyIndices

		struct SwapchainSupportDetails {
		public:
			SwapchainSupportDetails() = default;

			SwapchainSupportDetails(
				const VkSurfaceCapabilitiesKHR& capabilities,
				aqua::SafeArray<VkSurfaceFormatKHR>&& formats,
				aqua::SafeArray<VkPresentModeKHR>&& presentModes) :
			surfaceCapabilites(capabilities),
			surfaceFormats(std::move(formats)),
			presentModes(std::move(presentModes)) {}

		public:
			VkSurfaceCapabilitiesKHR			surfaceCapabilites;
			aqua::SafeArray<VkSurfaceFormatKHR> surfaceFormats;
			aqua::SafeArray<VkPresentModeKHR>   presentModes;
		}; // struct SwapchainSupportDetails

	public:
		~VulkanAPI();

		VulkanAPI(const VulkanAPI&) = delete;
		VulkanAPI(VulkanAPI&&) noexcept = delete;

		VulkanAPI& operator=(const VulkanAPI&) = delete;
		VulkanAPI& operator=(VulkanAPI&&) noexcept = delete;

	public:
		static void SetMainWindowHints() noexcept;

	public:
		Expected<QueueFamilyIndices, Error> FindQueueFamilies(const GPU& gpu, uint32_t familyBits) const noexcept;
		Expected<SwapchainSupportDetails, Error> QuerySwapchainSupport(const GPU gpu) const noexcept;

	private:
		friend class RenderHardwareInterface;
		VulkanAPI(const Config& config, Status& status);

	private:
		aqua::Status _CreateVulkanInstance() noexcept;
		aqua::Status _CreateSurface() noexcept;
		aqua::Status _PickGPU() noexcept;

#if AQUA_VULKAN_ENABLE_VALIDATION_LAYERS
		aqua::Status _CreateDebugMessenger() noexcept;
		void _DestroyDebugMessenger() noexcept;
#endif // AQUA_VULKAN_ENABLE_VALIDATION_LAYERS

		void _DestroyVulkanInstance() noexcept;
		void _DestroySurface() noexcept;
		void _UnbindGPU() noexcept;

		void _Terminate() noexcept;

	private:
		Expected<bool, Error> _IsGPUsuitable(const GPU& gpu) const noexcept;
		Expected<bool, Error> _CheckGPUextensionSupport(const GPU& gpu) const noexcept;
		uint64_t _ScoreGPU(const GPU& gpu, const GPU_Properties* properties) const noexcept;

	private:
		const Config&			 m_config;
		size_t					 m_initializeCount = 0;

		VkAllocationCallbacks*   m_allocator	   = nullptr;

		VkInstance			     m_instance		   = VK_NULL_HANDLE;
		VkDebugUtilsMessengerEXT m_debugMessenger  = VK_NULL_HANDLE;
		VkSurfaceKHR             m_surface		   = VK_NULL_HANDLE;
		GPU                      m_GPU			   = VK_NULL_HANDLE;
	}; // class VulkanAPI
} // namespace aqua

#endif // !AQUA_VULKAN_API_HEADER