#ifndef AQUA_VULKAN_API_HEADER
#define AQUA_VULKAN_API_HEADER

#include <aqua/Build.h>
#include <aqua/Error.h>
#include <aqua/Config.h>
#include <aqua/engine/ForwardSystems.h>
#include <aqua/engine/RenderHardwareInterface.h>
#include <aqua/datastructures/Array.h>

#include <vulkan/vulkan.h>

namespace aqua {
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
				PRESENT_BIT  = 1u << (uint32_t)Family::PRESENT
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
			VkSurfaceCapabilitiesKHR			surfaceCapabilites = {};
			aqua::SafeArray<VkSurfaceFormatKHR> surfaceFormats;
			aqua::SafeArray<VkPresentModeKHR>   presentModes;
		}; // struct SwapchainSupportDetails

		class RenderPipeline {
		public:
			RenderPipeline() noexcept = default;
			RenderPipeline(const RenderPipeline&) = delete;
			RenderPipeline(RenderPipeline&& other) noexcept;
			~RenderPipeline();

		public:
			Status Create(const RenderPipelineCreateInfo&) noexcept;
			void Destroy() noexcept;

		private:
			struct _ShaderReflections {
				const ShaderReflection* vertex   = nullptr;
				const ShaderReflection* fragment = nullptr;
			};

		private:
			Status _CreateDescriptorSetLayouts(const RenderPipelineCreateInfo&, const _ShaderReflections&) noexcept;
			Status _CreateRenderPass(const RenderPipelineCreateInfo&) noexcept;
			Status _CreatePipeline(const RenderPipelineCreateInfo&, const _ShaderReflections&) noexcept;

			uint32_t _GetStageCount(const RenderPipelineCreateInfo&) const noexcept;

		private:
			VkPipeline						 m_pipeline		  = VK_NULL_HANDLE;
			VkPipelineLayout			     m_pipelineLayout = VK_NULL_HANDLE;
			VkRenderPass                     m_renderPass	  = VK_NULL_HANDLE;
			SafeArray<VkDescriptorSetLayout> m_descriptorSetLayouts;
		}; // class RenderPipeline

	public:
		friend class RenderPipeline;

	public:
		~VulkanAPI();

		VulkanAPI(const VulkanAPI&) = delete;
		VulkanAPI(VulkanAPI&&) noexcept = delete;

		VulkanAPI& operator=(const VulkanAPI&) = delete;
		VulkanAPI& operator=(VulkanAPI&&) noexcept = delete;

	public:
		Expected<QueueFamilyIndices, Error> FindQueueFamilies(const GPU& gpu, uint32_t familyBits) const noexcept;
		Expected<SwapchainSupportDetails, Error> QuerySwapchainSupport(const GPU gpu) const noexcept;

	private:
		Status _CreateVulkanInstance(const RenderAPICreateInfo&) noexcept;
		Status _CreateDebugMessenger(const RenderAPICreateInfo&) noexcept;
		Status _CreateSurface(const RenderAPICreateInfo&) noexcept;
		Status _PickGPU(const RenderAPICreateInfo&) noexcept;
		Status _CreateLogicalDevice(const RenderAPICreateInfo&) noexcept;
		Status _CreateSwapchain(const RenderAPICreateInfo&) noexcept;
		Status _CreateSwapchainImageViews(const RenderAPICreateInfo&) noexcept;
		Status _CreateRenderPipelines(const RenderAPICreateInfo&) noexcept;

		void _DestroyVulkanInstance() noexcept;
		void _DestroyDebugMessenger() noexcept;
		void _DestroySurface() noexcept;
		void _UnbindGPU() noexcept;
		void _DestroyLogicalDevice() noexcept;
		void _DestroySwapchain() noexcept;
		void _DestroySwapchainImageViews() noexcept;
		void _DestroyRenderPipelines() noexcept;

		void _Terminate() noexcept;

	private:
		Expected<bool, Error> _IsGPUsuitable(const GPU& gpu) const noexcept;
		Expected<bool, Error> _CheckGPUextensionSupport(const GPU& gpu) const noexcept;
		uint64_t _ScoreGPU(const GPU& gpu, const GPU_Properties* properties) const noexcept;

		VkSurfaceFormatKHR _SelectSwapchainSurfaceFormat(const aqua::SafeArray<VkSurfaceFormatKHR>& formats) const noexcept;
		VkPresentModeKHR _SelectSwapchainPresentMode(const aqua::SafeArray<VkPresentModeKHR>& modes) const noexcept;
		VkExtent2D _SelectSwapchainExtent(const VkSurfaceCapabilitiesKHR& capabilities) const noexcept;

	private:
		friend class RenderPipeline;

		friend class RenderHardwareInterface;
		VulkanAPI(const Config&, const RenderAPICreateInfo&, Status&);

	private:
		const Config&			  m_config;
		size_t					  m_initializeCount      = 0;

		VkAllocationCallbacks*	  m_allocator			 = nullptr;

		VkInstance				  m_instance			 = VK_NULL_HANDLE;
		VkDebugUtilsMessengerEXT  m_debugMessenger	     = VK_NULL_HANDLE;
		VkSurfaceKHR			  m_surface			     = VK_NULL_HANDLE;
		GPU						  m_GPU				     = VK_NULL_HANDLE;
		VkDevice				  m_logicalDevice		 = VK_NULL_HANDLE;

		VkQueue					  m_graphicsQueue	     = VK_NULL_HANDLE;
		VkQueue					  m_presentQueue		 = VK_NULL_HANDLE;

		VkSwapchainKHR			  m_swapchain            = VK_NULL_HANDLE;
		VkFormat				  m_swapchainImageFormat = {};
		VkExtent2D				  m_swapchainExtent	     = {};

		SafeArray<VkImage>        m_swapchainImages;
		SafeArray<VkImageView>    m_swapchainImageViews;
		SafeArray<RenderPipeline> m_renderPipelines;
	}; // class VulkanAPI
} // namespace aqua

#endif // !AQUA_VULKAN_API_HEADER