#include <aqua/engine/graphics/api/VulkanAPI.h>
#include <aqua/datastructures/Array.h>

#include <aqua/engine/WindowSystem.h>

#include <aqua/Assert.h>
#include <aqua/Logger.h>

namespace {
	aqua::VulkanAPI* g_VulkanAPI = nullptr;
}

namespace {
	 aqua::Expected<aqua::SafeArray<const char*>, aqua::Error> GetRequiredExtensions() noexcept {
		uint32_t glfwExtensionCount = 0;
		const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

		aqua::SafeArray<const char*> extensions;

#if AQUA_VULKAN_ENABLE_VALIDATION_LAYERS
		AQUA_TRY(extensions.Reserve(glfwExtensionCount + 1), _);
		extensions.PushUnchecked(glfwExtensions, glfwExtensions + glfwExtensionCount);
		extensions.PushUnchecked(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#else
		AQUA_TRY(extensions.Reserve(glfwExtensionCount), _);
		extensions.PushUnchecked(glfwExtensions, glfwExtensions + glfwExtensionCount);
#endif // AQUA_VULKAN_ENABLE_VALIDATION_LAYERS

		return std::move(extensions);
	}

#if AQUA_VULKAN_ENABLE_VALIDATION_LAYERS
	static const char* VALIDATION_LAYERS[] = {
		"VK_LAYER_KHRONOS_validation"
	};
	static const uint32_t VALIDATION_LAYER_COUNT = sizeof(VALIDATION_LAYERS) / sizeof(const char*);

	static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT severity,
		VkDebugUtilsMessageTypeFlagsEXT type,
		const VkDebugUtilsMessengerCallbackDataEXT* callbackData,
		void* userData
	) noexcept {
		aqua::String256 message;
		message.Set(callbackData->pMessage);

		if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
			AQUA_LOG_WARNING(message);
			return VK_FALSE;
		}
		if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
			AQUA_LOG_FATAL_ERROR(message);
			return VK_FALSE;
		}

		return VK_FALSE;
	}

	static void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) noexcept {
		createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		createInfo.pfnUserCallback = DebugCallback;
		createInfo.pUserData = nullptr;
	}

	static VkResult CreateDebugUtilsMessengerEXT(
		VkInstance instance,
		const VkDebugUtilsMessengerCreateInfoEXT* createInfo,
		const VkAllocationCallbacks* allocator,
		VkDebugUtilsMessengerEXT* debugMessenger
	) noexcept {
		auto createFunction = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
			instance, "vkCreateDebugUtilsMessengerEXT");
		if (createFunction != nullptr) {
			return createFunction(instance, createInfo, allocator, debugMessenger);
		}
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}

	static void DestroyDebugUtilsMessengerEXT(
		VkInstance instance,
		VkDebugUtilsMessengerEXT debugMessenger,
		const VkAllocationCallbacks* allocator
	) noexcept {
		auto destroyFunc = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
			instance, "vkDestroyDebugUtilsMessengerEXT");
		if (destroyFunc != nullptr) {
			destroyFunc(instance, debugMessenger, allocator);
		}
	}
#endif // AQUA_VULKAN_ENABLE_VALIDATION_LAYERS
}

/*****************************************************************************************************************************
********************************************* VulkanAPI::QueueFamilyIndices **************************************************
/****************************************************************************************************************************/

void aqua::VulkanAPI::QueueFamilyIndices::SetFamilyIndex(Family family, uint32_t index) noexcept {
	uint32_t familyIndex = (uint32_t)family;

	m_indices[familyIndex] = index;
	m_isComplete |= (1u << familyIndex);
}

uint32_t aqua::VulkanAPI::QueueFamilyIndices::GetFamilyIndex(Family family) const noexcept {
	return m_indices[(uint32_t)family];
}

bool aqua::VulkanAPI::QueueFamilyIndices::IsComplete(uint32_t families) const noexcept {
	return (m_isComplete & families) == families;
}

/*****************************************************************************************************************************
******************************************************* VulkanAPI ************************************************************
/****************************************************************************************************************************/

aqua::VulkanAPI::VulkanAPI(const Config& config, Status& status) : m_config(config) {
	AQUA_ASSERT(g_VulkanAPI == nullptr, Literal("Attempt to create another graphics API instance"));

	if (!status.IsSuccess() || g_VulkanAPI != nullptr) {
		return;
	}

	Status(VulkanAPI::*initFuncs[])() = {
		&VulkanAPI::_CreateVulkanInstance,
		&VulkanAPI::_CreateDebugMessenger,
		&VulkanAPI::_CreateSurface,
		&VulkanAPI::_PickGPU
	};

	for (Status(VulkanAPI::* initFunc)() : initFuncs) {
		Status initStatus = ((*this).*initFunc)();
		if (!initStatus.IsSuccess()) {
			_Terminate();
			status.EmplaceError(initStatus.GetError());
			return;
		}
		++m_initializeCount;
	}

	g_VulkanAPI = this;
}

aqua::VulkanAPI::~VulkanAPI() {
	_Terminate();
}

void aqua::VulkanAPI::SetMainWindowHints() noexcept {
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
}

aqua::Expected<aqua::VulkanAPI::QueueFamilyIndices, aqua::Error> aqua::VulkanAPI::FindQueueFamilies(
const GPU& GPU, uint32_t families) const noexcept {
	QueueFamilyIndices foundIndices;

	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(GPU, &queueFamilyCount, nullptr);

	if (queueFamilyCount == 0) {
		return Error::VULKAN_NO_AVAILABLE_QUEUE_FAMILIES_FOUND;
	}
	aqua::SafeArray<VkQueueFamilyProperties> queueFamilies;
	AQUA_TRY(queueFamilies.Resize(queueFamilyCount), _);

	vkGetPhysicalDeviceQueueFamilyProperties(GPU, &queueFamilyCount, queueFamilies.GetPtr());

	QueueFamilyIndices indices;
	uint32_t i = 0;
	for (; i < queueFamilyCount; ++i) {
		const VkQueueFamilyProperties queueFamily = queueFamilies[i];

		if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			indices.SetFamilyIndex(QueueFamilyIndices::Family::GRAPHICS, i);
			foundIndices.SetFamilyIndex(QueueFamilyIndices::Family::GRAPHICS, i);
		}

		if (families & QueueFamilyIndices::PRESENT_BIT) {
			VkBool32 presentQueueSupported = VK_FALSE;
			if (vkGetPhysicalDeviceSurfaceSupportKHR(GPU, i, m_surface, &presentQueueSupported) != VK_SUCCESS) {
				return Error::VULKAN_FAILED_TO_GET_QUEUE_FAMILY_PRESENT_SUPPORT;
			}
			if (presentQueueSupported) {
				indices.SetFamilyIndex(QueueFamilyIndices::Family::PRESENT, i);
				foundIndices.SetFamilyIndex(QueueFamilyIndices::Family::PRESENT, i);
			}
		}

		if (indices.IsComplete(families)) {
			return indices;
		}

		indices = QueueFamilyIndices();
	}
	return foundIndices;
}

aqua::Expected<aqua::VulkanAPI::SwapchainSupportDetails, aqua::Error> aqua::VulkanAPI::QuerySwapchainSupport(
const GPU gpu) const noexcept {
	SwapchainSupportDetails details;

	if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(gpu, m_surface, &details.surfaceCapabilites) != VK_SUCCESS) {
		return Error::VULKAN_FAILED_TO_ACQUIRE_GPU_SURFACE_CAPABILITIES;
	}

	uint32_t formatCount = 0;
	VkResult result = VK_SUCCESS;
	do {
		if (vkGetPhysicalDeviceSurfaceFormatsKHR(gpu, m_surface, &formatCount, nullptr) != VK_SUCCESS) {
			return Error::VULKAN_FAILED_TO_ACQUIRE_GPU_SURFACE_FORMATS;
		}
		AQUA_TRY(details.surfaceFormats.Resize(formatCount), _);
		result = vkGetPhysicalDeviceSurfaceFormatsKHR(gpu, m_surface, &formatCount, details.surfaceFormats.GetPtr());
	} while (result == VK_INCOMPLETE);

	if (result != VK_SUCCESS) {
		return Error::VULKAN_FAILED_TO_ACQUIRE_GPU_SURFACE_FORMATS;
	}

	AQUA_LOG_DEBUG_SYNC(Literal("VULKAN: GPU surface formats found: {}"), formatCount);

	uint32_t presentModeCount = 0;
	do {
		if (vkGetPhysicalDeviceSurfacePresentModesKHR(gpu, m_surface, &presentModeCount, nullptr) != VK_SUCCESS) {
			return Error::VULKAN_FAILED_TO_ACQUIRE_GPU_PRESENT_MODES;
		}
		AQUA_TRY(details.presentModes.Resize(presentModeCount), _);
		result = vkGetPhysicalDeviceSurfacePresentModesKHR(gpu, m_surface, &presentModeCount, details.presentModes.GetPtr());
	} while (result == VK_INCOMPLETE);

	if (result != VK_SUCCESS) {
		return Error::VULKAN_FAILED_TO_ACQUIRE_GPU_PRESENT_MODES;
	}

	AQUA_LOG_DEBUG_SYNC(Literal("VULKAN: GPU surface present modes found: {}"), presentModeCount);

	return SwapchainSupportDetails(
		details.surfaceCapabilites,
		std::move(details.surfaceFormats),
		std::move(details.presentModes)
	);
}

/*****************************************************************************************************************************
*********************************************** VulkanAPI 'create' methods ***************************************************
/****************************************************************************************************************************/

aqua::Status aqua::VulkanAPI::_CreateVulkanInstance() noexcept {
	const auto& engineInfo = m_config.GetEngineInfo();

	VkApplicationInfo appInfo{};
	appInfo.sType			 = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pEngineName		 = engineInfo.internal.name.GetPtr();
	appInfo.pApplicationName = engineInfo.external.applicationName;

	appInfo.applicationVersion = VK_MAKE_VERSION(
		engineInfo.external.applicationVersion.major,
		engineInfo.external.applicationVersion.minor,
		engineInfo.external.applicationVersion.patch
	);
	appInfo.engineVersion = VK_MAKE_VERSION(
		engineInfo.internal.version.major,
		engineInfo.internal.version.minor,
		engineInfo.internal.version.patch
	);

	VkInstanceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;

	AQUA_TRY(GetRequiredExtensions(), requiredExtensions);

	createInfo.enabledExtensionCount   = static_cast<uint32_t>(requiredExtensions.GetValue().GetSize());
	createInfo.ppEnabledExtensionNames = requiredExtensions.GetValue().GetPtr();

#if AQUA_VULKAN_ENABLE_VALIDATION_LAYERS
	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
	createInfo.enabledLayerCount   = VALIDATION_LAYER_COUNT;
	createInfo.ppEnabledLayerNames = VALIDATION_LAYERS;

	PopulateDebugMessengerCreateInfo(debugCreateInfo);
	createInfo.pNext = &debugCreateInfo;
#else
	createInfo.enabledLayerCount   = 0;
	createInfo.ppEnabledLayerNames = nullptr;
	createInfo.pNext			   = nullptr;
#endif // AQUA_VULKAN_ENABLE_VALIDATION_LAYERS

	if (vkCreateInstance(&createInfo, m_allocator, &m_instance) != VK_SUCCESS) {
		return Error::VULKAN_FAILED_TO_CREATE_INSTANCE;
	}

	AQUA_LOG_VULKAN(Literal("VULKAN: Vulkan instance is created"));

	return Success{};
}

aqua::Status aqua::VulkanAPI::_CreateSurface() noexcept {
	GLFWwindow* window = (GLFWwindow*)WindowSystem::GetConst()._GetMainWindowPtr();
	if (glfwCreateWindowSurface(m_instance, window, m_allocator, &m_surface) != VK_SUCCESS) {
		return Error::VULKAN_FAILED_TO_CREATE_SURFACE;
	}

	AQUA_LOG_VULKAN(Literal("VULKAN: Window surface is created"));

	return Success{};
}

aqua::Status aqua::VulkanAPI::_PickGPU() noexcept {
	uint32_t GPUcount = 0;
	if (vkEnumeratePhysicalDevices(m_instance, &GPUcount, nullptr) != VK_SUCCESS) {
		return Error::VULKAN_FAILED_TO_ENUMERATE_GPUS;
	}

	if (GPUcount == 0) {
		return Error::VULKAN_NO_AVAILABLE_GPU_FOUND;
	}
	aqua::SafeArray<GPU> availableGPUs;
	AQUA_TRY(availableGPUs.Resize(GPUcount), _);

	if (vkEnumeratePhysicalDevices(m_instance, &GPUcount, availableGPUs.GetPtr()) != VK_SUCCESS) {
		return Error::VULKAN_FAILED_TO_ENUMERATE_GPUS;
	}

#if AQUA_DEBUG
	AQUA_LOG_DEBUG_SYNC(Literal("Available GPUs found: {}"), GPUcount);

	GPU_Properties __GPUproperties;
	for (const GPU& gpu : availableGPUs) {
		vkGetPhysicalDeviceProperties(gpu, &__GPUproperties);

		AQUA_LOG_DEBUG_SYNC(Literal("\t{}"), StringLiteral(__GPUproperties.deviceName));
	}
#endif // AQUA_DEBUG

	GPU			   pickedGPU = VK_NULL_HANDLE;
	uint64_t	   pickedGPUscore = 0;
	GPU_Properties pickedGPUproperties;

	GPU_Properties GPUproperties;
	for (size_t i = 0; i < GPUcount; ++i) {
		const GPU& gpu = availableGPUs[i];
		vkGetPhysicalDeviceProperties(gpu, &GPUproperties);

		auto isGPUsuitable = _IsGPUsuitable(gpu);
		if (!isGPUsuitable.HasValue() || !isGPUsuitable.GetValue()) {
			continue;
		}
		uint64_t score = _ScoreGPU(gpu, &GPUproperties);
		if (pickedGPU == VK_NULL_HANDLE || score > pickedGPUscore) {
			pickedGPU = gpu;
			pickedGPUscore = score;
			pickedGPUproperties = GPUproperties;
		}
	}
	if (pickedGPU == VK_NULL_HANDLE) {
		return Error::VULKAN_NO_SUITABLE_GPU_FOUND;
	}
	m_GPU = pickedGPU;

	AQUA_LOG_SYNC(Literal("Picked GPU: '{}' with score {}"),
		StringLiteral(pickedGPUproperties.deviceName), pickedGPUscore);

	return Success{};
}

#if AQUA_VULKAN_ENABLE_VALIDATION_LAYERS
aqua::Status aqua::VulkanAPI::_CreateDebugMessenger() noexcept {
	VkDebugUtilsMessengerCreateInfoEXT createInfo{};
	PopulateDebugMessengerCreateInfo(createInfo);

	if (CreateDebugUtilsMessengerEXT(m_instance, &createInfo, m_allocator, &m_debugMessenger) != VK_SUCCESS) {
		return Error::VULKAN_FAILED_TO_CREATE_DEBUG_MESSENGER;
	}

	AQUA_LOG_VULKAN(Literal("VULKAN: Debug messenger is created"));

	return Success{};
}

void aqua::VulkanAPI::_DestroyDebugMessenger() noexcept {
	DestroyDebugUtilsMessengerEXT(m_instance, m_debugMessenger, m_allocator);
	m_debugMessenger = VK_NULL_HANDLE;

	AQUA_LOG_VULKAN(Literal("VULKAN: Debug messenger is destroyed"));
}
#endif // AQUA_VULKAN_ENABLE_VALIDATION_LAYERS

/*****************************************************************************************************************************
*********************************************** VulkanAPI 'destroy' methods **************************************************
/****************************************************************************************************************************/

void aqua::VulkanAPI::_DestroyVulkanInstance() noexcept {
	vkDestroyInstance(m_instance, m_allocator);
	m_instance = VK_NULL_HANDLE;

	AQUA_LOG_VULKAN(Literal("VULKAN: Vulkan instance is destroyed"));
}

void aqua::VulkanAPI::_DestroySurface() noexcept {
	vkDestroySurfaceKHR(m_instance, m_surface, m_allocator);
	m_surface = VK_NULL_HANDLE;

	AQUA_LOG_VULKAN(Literal("VULKAN: Window surface is destroyed"));
}

void aqua::VulkanAPI::_UnbindGPU() noexcept {
	m_GPU = VK_NULL_HANDLE;
}

void aqua::VulkanAPI::_Terminate() noexcept {
	void(VulkanAPI::*terminateFuncs[])() = {
		&VulkanAPI::_UnbindGPU,
		&VulkanAPI::_DestroySurface,
		&VulkanAPI::_DestroyDebugMessenger,
		&VulkanAPI::_DestroyVulkanInstance,
	};

	for (size_t i = 0; i < m_initializeCount; ++i) {
		((*this).*terminateFuncs[i])();
	}
	m_initializeCount = 0;

	g_VulkanAPI = nullptr;
}

aqua::Expected<bool, aqua::Error> aqua::VulkanAPI::_IsGPUsuitable(const GPU& gpu) const noexcept {
	uint32_t queueFamilyBits = QueueFamilyIndices::GRAPHICS_BIT | QueueFamilyIndices::PRESENT_BIT;
	AQUA_TRY(FindQueueFamilies(gpu, queueFamilyBits), queueFamilies);

	if (!queueFamilies.GetValue().IsComplete(queueFamilyBits)) {
		return false;
	}
	AQUA_TRY(_CheckGPUextensionSupport(gpu), isExtensionsSupported);

	if (!isExtensionsSupported.GetValue()) {
		return false;
	}
	AQUA_TRY(QuerySwapchainSupport(gpu), swapchainSupportDetails);

	return !swapchainSupportDetails.GetValue().surfaceFormats.IsEmpty() &&
		   !swapchainSupportDetails.GetValue().presentModes.IsEmpty();
}

aqua::Expected<bool, aqua::Error> aqua::VulkanAPI::_CheckGPUextensionSupport(const GPU& gpu) const noexcept {
	uint32_t extensionCount = 0;
	aqua::SafeArray<VkExtensionProperties> availableExtensions;

	VkResult enumerateResult = VK_SUCCESS;
	do {
		if (vkEnumerateDeviceExtensionProperties(gpu, nullptr, &extensionCount, nullptr) != VK_SUCCESS) {
			return Error::VULKAN_FAILED_TO_ENUMERATE_GPU_EXTENSION_PROPERTIES;
		}
		AQUA_TRY(availableExtensions.Resize(extensionCount), _);
		enumerateResult = vkEnumerateDeviceExtensionProperties(gpu, nullptr, &extensionCount, availableExtensions.GetPtr());
	} while (enumerateResult == VK_INCOMPLETE);

	if (enumerateResult != VK_SUCCESS) {
		return Error::VULKAN_FAILED_TO_ENUMERATE_GPU_EXTENSION_PROPERTIES;
	}

	AQUA_LOG_DEBUG(Literal("VULKAN: Available GPU extensions found: {}"), extensionCount);

	const char* requiredExtensions[] = {
		 VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};
	uint32_t requiredExtensionCount = sizeof(requiredExtensions) / sizeof(const char*);

	for (const char* required : requiredExtensions) {
		bool found = false;

		for (const VkExtensionProperties& available : availableExtensions) {
			if (strcmp(required, available.extensionName) == 0) {
				found = true;
				break;
			}
		}
		if (!found) {
			return false;
		}
	}
	return true;
}

// requires better scoring in future
uint64_t aqua::VulkanAPI::_ScoreGPU(const GPU& gpu, const GPU_Properties* properties) const noexcept {
	uint64_t score = 0;

	GPU_Features features;
	GPU_MemoryProperties memoryProperties;
	vkGetPhysicalDeviceFeatures(gpu, &features);
	vkGetPhysicalDeviceMemoryProperties(gpu, &memoryProperties);

	if (properties->deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
		score += 1'000'000;
		for (uint32_t i = 0; i < memoryProperties.memoryHeapCount; i++) {
			if (memoryProperties.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) {
				score += memoryProperties.memoryHeaps[i].size >> 10;
			}
		}
	}
	else {
		score += 500'000;
	}

	return score;
}