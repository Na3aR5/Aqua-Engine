#include <aqua/engine/graphics/api/VulkanAPI.h>
#include <aqua/datastructures/Array.h>

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
		void* userData) noexcept {
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
******************************************************* VulkanAPI ************************************************************
/****************************************************************************************************************************/

void aqua::VulkanAPI::SetMainWindowHints() noexcept {
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
}

aqua::VulkanAPI::VulkanAPI(const Config& config, Status& status) : m_config(config) {
	AQUA_ASSERT(g_VulkanAPI == nullptr, Literal("Attempt to create another graphics API instance"));

	if (!status.IsSuccess() || g_VulkanAPI != nullptr) {
		return;
	}

	Status(VulkanAPI::*initFuncs[])() = {
		&VulkanAPI::_CreateVulkanInstance,
		&VulkanAPI::_CreateDebugMessenger,
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

/*****************************************************************************************************************************
*********************************************** VulkanAPI 'create' methods ***************************************************
/****************************************************************************************************************************/

aqua::Status aqua::VulkanAPI::_CreateVulkanInstance() noexcept {
	VkApplicationInfo appInfo{};
	appInfo.sType			 = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pEngineName		 = m_config.engine.name.GetPtr();
	appInfo.pApplicationName = m_config.application.name;

	appInfo.applicationVersion = VK_MAKE_VERSION(
		m_config.application.version.major,
		m_config.application.version.minor,
		m_config.application.version.patch
	);
	appInfo.engineVersion = VK_MAKE_VERSION(
		m_config.engine.version.major,
		m_config.engine.version.minor,
		m_config.engine.version.patch
	);

	VkInstanceCreateInfo createInfo{};
	createInfo.sType			= VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
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

aqua::Status aqua::VulkanAPI::_PickGPU() noexcept {
	uint32_t GPUcount = 0;
	vkEnumeratePhysicalDevices(m_instance, &GPUcount, nullptr);

	if (GPUcount == 0) {
		return Error::VULKAN_NO_SUITABLE_GPU_FOUND;
	}

	AQUA_LOG(Literal("GPUs found: {}"), GPUcount);

	aqua::SafeArray<GPU> availableGPUs;
	AQUA_TRY(availableGPUs.Resize(GPUcount), _);

	if (vkEnumeratePhysicalDevices(m_instance, &GPUcount, availableGPUs.GetPtr()) != VK_SUCCESS) {
		return Error::VULKAN_FAILED_TO_PICK_GPU;
	}
	for (const GPU& GPU : availableGPUs) {
		VkPhysicalDeviceProperties properties;
		vkGetPhysicalDeviceProperties(GPU, &properties);

		AQUA_LOG_SYNC(Literal("\t{}"), StringLiteral(properties.deviceName));
	}

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

	AQUA_LOG_VULKAN(Literal("VULKAN: Debug messenger is destroyed"));
}
#endif // AQUA_VULKAN_ENABLE_VALIDATION_LAYERS

/*****************************************************************************************************************************
*********************************************** VulkanAPI 'destroy' methods **************************************************
/****************************************************************************************************************************/

void aqua::VulkanAPI::_DestroyVulkanInstance() noexcept {
	vkDestroyInstance(m_instance, m_allocator);

	AQUA_LOG_VULKAN(Literal("VULKAN: Vulkan instance is destroyed"));
}

void aqua::VulkanAPI::_UnbindGPU() noexcept {
	m_GPU = VK_NULL_HANDLE;
}

void aqua::VulkanAPI::_Terminate() noexcept {
	void(VulkanAPI::*terminateFuncs[])() = {
		&VulkanAPI::_DestroyVulkanInstance,
		&VulkanAPI::_DestroyDebugMessenger,
		&VulkanAPI::_UnbindGPU
	};

	for (size_t i = 0; i < m_initializeCount; ++i) {
		((*this).*terminateFuncs[m_initializeCount - i - 1])();
	}
	m_initializeCount = 0;

	g_VulkanAPI = nullptr;
}