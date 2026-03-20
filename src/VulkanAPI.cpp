#include <aqua/pch.h>
#include <aqua/engine/graphics/api/VulkanAPI.h>
#include <aqua/engine/WindowSystem.h>
#include <aqua/datastructures/Array.h>
#include <aqua/math/Random.h>
#include <aqua/Reflection.h>

#include <aqua/Assert.h>
#include <aqua/Logger.h>

namespace {
	aqua::VulkanAPI* g_VulkanAPI = nullptr;

	struct {
		VkAllocationCallbacks* allocator	 = nullptr;
		VkDevice			   logicalDevice = VK_NULL_HANDLE;
	} g_VulkanAPI_CreateContext;
}

namespace {
	static const char* VALIDATION_LAYERS[] = {
		"VK_LAYER_KHRONOS_validation"
	};
	static const uint32_t VALIDATION_LAYER_COUNT = sizeof(VALIDATION_LAYERS) / sizeof(const char*);

	const char* GPU_REQUIRED_EXTENSIONS[] = {
		 VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};
	uint32_t GPU_REQUIRED_EXTENSION_COUNT = sizeof(GPU_REQUIRED_EXTENSIONS) / sizeof(const char*);
}

namespace {
	aqua::Expected<aqua::SafeArray<const char*>, aqua::Error> GetRequiredExtensions() noexcept {
		AQUA_TRY(aqua::WindowSystem::Get().GetVulkanRequiredInstanceExtensions(), extensions);

#if AQUA_VULKAN_ENABLE_VALIDATION_LAYERS
		AQUA_TRY(extensions.GetValue().Reserve(1));
		extensions.GetValue().PushUnchecked(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif // AQUA_VULKAN_ENABLE_VALIDATION_LAYERS

		return extensions;
	}

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
}

namespace {
	VkDescriptorType ConvertReflectionDescriptorType(aqua::ShaderReflection::DescriptorType type) noexcept {
		switch (type) {
			case aqua::ShaderReflection::UNIFORM_BUFFER:
				return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

			default:
				break;
		}
		return (VkDescriptorType)0;
	}

	VkShaderStageFlagBits ConvertReflectionBindingShaderStages(uint32_t stages) noexcept {
		uint32_t bits = 0;
		if (stages & (uint32_t)aqua::ShaderReflection::Stage::VERTEX) {
			bits |= (uint32_t)VK_SHADER_STAGE_VERTEX_BIT;
		}
		if (stages & (uint32_t)aqua::ShaderReflection::Stage::FRAGMENT) {
			bits |= (uint32_t)VK_SHADER_STAGE_FRAGMENT_BIT;
		}
		return (VkShaderStageFlagBits)bits;
	}
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
********************************************** VulkanAPI::RenderPipeline *****************************************************
/****************************************************************************************************************************/

aqua::VulkanAPI::RenderPipeline::RenderPipeline(RenderPipeline&& other) noexcept :
m_pipeline(other.m_pipeline),
m_pipelineLayout(other.m_pipelineLayout),
m_renderPass(other.m_renderPass),
m_descriptorSetLayouts(std::move(m_descriptorSetLayouts)) {
	other.m_pipeline	   = VK_NULL_HANDLE;
	other.m_pipelineLayout = VK_NULL_HANDLE;
	other.m_renderPass     = VK_NULL_HANDLE;
}

aqua::VulkanAPI::RenderPipeline::~RenderPipeline() {
	Destroy();
}

aqua::Status aqua::VulkanAPI::RenderPipeline::Create(const RenderPipelineCreateInfo& info) noexcept {
	AQUA_TRY(_CreateDescriptorSetLayouts(info));
	return Success{};
}

aqua::Status aqua::VulkanAPI::RenderPipeline::_CreateDescriptorSetLayouts(const RenderPipelineCreateInfo& info) noexcept {
	// in future add set levels by update frequency

	AQUA_TRY(DeserializeShaderReflection(info.vertexShaderAsset.reflectionPath), expectedVertexReflection);
	AQUA_TRY(DeserializeShaderReflection(info.fragmentShaderAsset.reflectionPath), expectedFragmentReflection);
	const ShaderReflection& vertexReflection   = expectedVertexReflection.GetValue();
	const ShaderReflection& fragmentReflection = expectedFragmentReflection.GetValue();

	uint32_t vertexDescriptorSetCount = (uint32_t)vertexReflection.descriptorSets.GetSize();
	uint32_t fragmentDescriptorSetCount = (uint32_t)fragmentReflection.descriptorSets.GetSize();
	AQUA_TRY(m_descriptorSetLayouts.Reserve(vertexDescriptorSetCount + fragmentDescriptorSetCount));

	auto EmplaceBindings = [](const decltype(vertexReflection.descriptorSets[0].bindings)& array,
	SafeArray< VkDescriptorSetLayoutBinding>& bindings) {
		for (const ShaderReflection::DescriptorBinding& binding : array) {
			VkDescriptorSetLayoutBinding descriporSetBinding{};
			descriporSetBinding.binding         = binding.binding;
			descriporSetBinding.descriptorCount = binding.count;
			descriporSetBinding.descriptorType  = ConvertReflectionDescriptorType(binding.type);
			descriporSetBinding.stageFlags      = ConvertReflectionBindingShaderStages(binding.stages);

			bindings.EmplaceBackUnchecked(descriporSetBinding);
		}
	};

	uint32_t i = 0, j = 0;
	while (i < vertexDescriptorSetCount && j < fragmentDescriptorSetCount) {
		uint32_t setIndex;
		bool merge = false;

		if (vertexReflection.descriptorSets[i].set < fragmentReflection.descriptorSets[j].set) {
			setIndex = i++;
		}
		else if (vertexReflection.descriptorSets[i].set > fragmentReflection.descriptorSets[j].set) {
			setIndex = j++;
		}
		else {
			merge = true;
			setIndex = i++;
			++j;
		}
		uint32_t bindingCount = (uint32_t)vertexReflection.descriptorSets[setIndex].bindings.GetSize();
		if (merge) {
			bindingCount += (uint32_t)fragmentReflection.descriptorSets[setIndex].bindings.GetSize();
		}
		SafeArray<VkDescriptorSetLayoutBinding> bindings;
		AQUA_TRY(bindings.Reserve(bindingCount));

		EmplaceBindings(vertexReflection.descriptorSets[setIndex].bindings, bindings);
		if (merge) {
			EmplaceBindings(fragmentReflection.descriptorSets[setIndex].bindings, bindings);
		}
		VkDescriptorSetLayoutCreateInfo createInfo{};
		createInfo.sType		= VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		createInfo.bindingCount = bindingCount;
		createInfo.pBindings    = bindings.GetPtr();

		m_descriptorSetLayouts.EmplaceBackUnchecked();
		if (vkCreateDescriptorSetLayout(g_VulkanAPI_CreateContext.logicalDevice, &createInfo,
			g_VulkanAPI_CreateContext.allocator, &m_descriptorSetLayouts.Last()) != VK_SUCCESS) {
			return Error::VULKAN_FAILED_TO_CREATE_DESCRIPTOR_SET_LAYOUT;
		}
	}

	auto EmplaceDescriptorSetLayouts = [this, EmplaceBindings](
	const decltype(vertexReflection.descriptorSets)& sets, uint32_t i) -> Status {
		uint32_t setCount = sets.GetSize();
		for (; i < setCount; ++i) {
			uint32_t bindingCount = (uint32_t)sets[i].bindings.GetSize();

			SafeArray<VkDescriptorSetLayoutBinding> bindings;
			AQUA_TRY(bindings.Reserve(bindingCount));

			EmplaceBindings(sets[i].bindings, bindings);

			VkDescriptorSetLayoutCreateInfo createInfo{};
			createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			createInfo.bindingCount = bindingCount;
			createInfo.pBindings = bindings.GetPtr();

			m_descriptorSetLayouts.EmplaceBackUnchecked();
			if (vkCreateDescriptorSetLayout(g_VulkanAPI_CreateContext.logicalDevice, &createInfo,
				g_VulkanAPI_CreateContext.allocator, &m_descriptorSetLayouts.Last()) != VK_SUCCESS) {
				return Error::VULKAN_FAILED_TO_CREATE_DESCRIPTOR_SET_LAYOUT;
			}
		}
		return Success{};
	};
	AQUA_TRY(EmplaceDescriptorSetLayouts(vertexReflection.descriptorSets, i));
	AQUA_TRY(EmplaceDescriptorSetLayouts(fragmentReflection.descriptorSets, j));

	AQUA_LOG_VULKAN(Literal("VULKAN: {} unique descriptor set layouts are created"), m_descriptorSetLayouts.GetSize());

	return Success{};
}

void aqua::VulkanAPI::RenderPipeline::Destroy() noexcept {
	for (const VkDescriptorSetLayout& layout : m_descriptorSetLayouts) {
		vkDestroyDescriptorSetLayout(g_VulkanAPI_CreateContext.logicalDevice, layout, g_VulkanAPI_CreateContext.allocator);
	}
	m_descriptorSetLayouts.DeepClear();
}

/*****************************************************************************************************************************
******************************************************* VulkanAPI ************************************************************
/****************************************************************************************************************************/

aqua::VulkanAPI::VulkanAPI(const Config& config, const RenderAPICreateInfo& info, Status& status) : m_config(config) {
	AQUA_ASSERT(g_VulkanAPI == nullptr, Literal("Attempt to create another graphics API instance"));

	if (!status.IsSuccess() || g_VulkanAPI != nullptr) {
		return;
	}
	m_allocator = (VkAllocationCallbacks*)MemorySystem::_GetVulkanAllocationCallbacks();

	Status(VulkanAPI::*initFuncs[])(const RenderAPICreateInfo&) = {
		&VulkanAPI::_CreateVulkanInstance,
		&VulkanAPI::_CreateDebugMessenger,
		&VulkanAPI::_CreateSurface,
		&VulkanAPI::_PickGPU,
		&VulkanAPI::_CreateLogicalDevice,
		&VulkanAPI::_CreateSwapchain,
		&VulkanAPI::_CreateSwapchainImageViews,
		&VulkanAPI::_CreateRenderPipelines
	};
	for (Status(VulkanAPI::*initFunc)(const RenderAPICreateInfo&) : initFuncs) {
		Status initStatus = ((*this).*initFunc)(info);
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

aqua::Expected<aqua::VulkanAPI::QueueFamilyIndices, aqua::Error> aqua::VulkanAPI::FindQueueFamilies(
const GPU& GPU, uint32_t families) const noexcept {
	QueueFamilyIndices foundIndices;

	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(GPU, &queueFamilyCount, nullptr);

	if (queueFamilyCount == 0) {
		return Error::VULKAN_NO_AVAILABLE_QUEUE_FAMILIES_FOUND;
	}
	aqua::SafeArray<VkQueueFamilyProperties> queueFamilies;
	AQUA_TRY(queueFamilies.Resize(queueFamilyCount));

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
				return Error::VULKAN_FAILED_TO_ACQUIRE_QUEUE_FAMILY_PRESENT_SUPPORT;
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
		AQUA_TRY(details.surfaceFormats.Resize(formatCount));
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
		AQUA_TRY(details.presentModes.Resize(presentModeCount));
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

aqua::Status aqua::VulkanAPI::_CreateVulkanInstance(const RenderAPICreateInfo&) noexcept {
	const auto& engineInfo = m_config.GetEngineInfo();

	VkApplicationInfo appInfo{};
	appInfo.sType			 = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pEngineName		 = engineInfo.internal.name.GetPtr();

	StringBuffer<char, 15> randomApplicationName;
	if (engineInfo.external.applicationName == nullptr) {
		AQUA_LOG_WARNING(Literal("Application name was not specified"));
		randomApplicationName = Random::GenerateApplicationName();
		appInfo.pApplicationName = randomApplicationName.GetPtr();
		AQUA_LOG_SYNC(Literal("Generated application name is: {}"), randomApplicationName.GetPtr());
	}
	else {
		appInfo.pApplicationName = engineInfo.external.applicationName;
	}
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

aqua::Status aqua::VulkanAPI::_CreateDebugMessenger(const RenderAPICreateInfo&) noexcept {
	VkDebugUtilsMessengerCreateInfoEXT createInfo{};
	PopulateDebugMessengerCreateInfo(createInfo);

	if (CreateDebugUtilsMessengerEXT(m_instance, &createInfo, m_allocator, &m_debugMessenger) != VK_SUCCESS) {
		return Error::VULKAN_FAILED_TO_CREATE_DEBUG_MESSENGER;
	}

	AQUA_LOG_VULKAN(Literal("VULKAN: Debug messenger is created"));

	return Success{};
}

aqua::Status aqua::VulkanAPI::_CreateSurface(const RenderAPICreateInfo&) noexcept {
	AQUA_TRY(WindowSystem::Get().CreateVulkanWindowSurface(m_instance, m_allocator), surface);
	m_surface = (VkSurfaceKHR)surface.GetValue();

	AQUA_LOG_VULKAN(Literal("VULKAN: Window surface is created"));

	return Success{};
}

aqua::Status aqua::VulkanAPI::_PickGPU(const RenderAPICreateInfo&) noexcept {
	uint32_t GPUcount = 0;
	aqua::SafeArray<GPU> availableGPUs;
	VkResult enumerateResult = VK_SUCCESS;

	do {
		if (vkEnumeratePhysicalDevices(m_instance, &GPUcount, nullptr) != VK_SUCCESS) {
			return Error::VULKAN_FAILED_TO_ENUMERATE_GPUS;
		}
		AQUA_TRY(availableGPUs.Resize(GPUcount));
		enumerateResult = vkEnumeratePhysicalDevices(m_instance, &GPUcount, availableGPUs.GetPtr());
	} while (enumerateResult == VK_INCOMPLETE);

	if (enumerateResult != VK_SUCCESS) {
		return Error::VULKAN_FAILED_TO_ENUMERATE_GPUS;
	}
	if (GPUcount == 0) {
		return Error::VULKAN_NO_AVAILABLE_GPU_FOUND;
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

aqua::Status aqua::VulkanAPI::_CreateLogicalDevice(const RenderAPICreateInfo&) noexcept {
	uint32_t requiredFamilies = QueueFamilyIndices::GRAPHICS_BIT | QueueFamilyIndices::PRESENT_BIT;
	AQUA_TRY(FindQueueFamilies(m_GPU, requiredFamilies), queueFamilyIndices);

	if (!queueFamilyIndices.GetValue().IsComplete(requiredFamilies)) {
		return Error::VULKAN_NO_SUITABLE_QUEUE_FAMILIES_FOUND;
	}
	aqua::SafeArray<uint32_t> uniqueIndices;
	AQUA_TRY(uniqueIndices.Resize(2));

	uint32_t graphicsFamily = queueFamilyIndices.GetValue().GetFamilyIndex(QueueFamilyIndices::Family::GRAPHICS);
	uint32_t presentFamily = queueFamilyIndices.GetValue().GetFamilyIndex(QueueFamilyIndices::Family::PRESENT);

	uniqueIndices[0] = graphicsFamily;
	uniqueIndices[1] = presentFamily;

	if (uniqueIndices[0] == uniqueIndices[1]) {
		uniqueIndices.Pop();
	}

	AQUA_LOG_DEBUG(Literal("VULKAN: Unique queue family indices: {}"), uniqueIndices.GetSize());

	aqua::SafeArray<VkDeviceQueueCreateInfo> queueCreateInfos;
	AQUA_TRY(queueCreateInfos.Reserve(uniqueIndices.GetSize()));

	float queuePriority = 1.0f;
	for (uint32_t queueFamilyIndex : uniqueIndices) {
		VkDeviceQueueCreateInfo queueCreateInfo{};
		queueCreateInfo.sType			 = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueFamilyIndex;
		queueCreateInfo.queueCount		 = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;

		queueCreateInfos.PushUnchecked(queueCreateInfo);
	}
	GPU_Features GPUfeatures{};

	VkDeviceCreateInfo createInfo{};
	createInfo.sType			       = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.pQueueCreateInfos       = queueCreateInfos.GetPtr();
	createInfo.queueCreateInfoCount    = static_cast<uint32_t>(queueCreateInfos.GetSize());
	createInfo.pEnabledFeatures		   = &GPUfeatures;
	createInfo.ppEnabledExtensionNames = GPU_REQUIRED_EXTENSIONS;
	createInfo.enabledExtensionCount   = GPU_REQUIRED_EXTENSION_COUNT;

#if AQUA_VULKAN_ENABLE_VALIDATION_LAYERS
	createInfo.ppEnabledLayerNames = VALIDATION_LAYERS;
	createInfo.enabledLayerCount   = VALIDATION_LAYER_COUNT;
#else
	createInfo.ppEnabledLayerNames = nullptr;
	createInfo.enabledLayerCount   = 0;
#endif // AQUA_VULKAN_ENABLE_VALIDATION_LAYERS

	if (vkCreateDevice(m_GPU, &createInfo, m_allocator, &m_logicalDevice) != VK_SUCCESS) {
		return Error::VULKAN_FAILED_TO_CREATE_LOGICAL_DEVICE;
	}
	vkGetDeviceQueue(m_logicalDevice, graphicsFamily, 0, &m_graphicsQueue);
	vkGetDeviceQueue(m_logicalDevice, presentFamily, 0, &m_presentQueue);

	_LoadCreateContext();

	AQUA_LOG_VULKAN(Literal("VULKAN: Logical device is created"));

	return Success{};
}

aqua::Status aqua::VulkanAPI::_CreateSwapchain(const RenderAPICreateInfo&) noexcept {
	AQUA_TRY(QuerySwapchainSupport(m_GPU), swapchainSupport);
	const SwapchainSupportDetails& swapchainSupportDetails = swapchainSupport.GetValue();

	VkSurfaceFormatKHR surfaceFormat = _SelectSwapchainSurfaceFormat(swapchainSupportDetails.surfaceFormats);
	AQUA_LOG_DEBUG(Literal("VULKAN: Selected swapchain format: format = {}, colorSpace = {}"),
		(int)surfaceFormat.format, (int)surfaceFormat.colorSpace);

	VkPresentModeKHR presentMode = _SelectSwapchainPresentMode(swapchainSupportDetails.presentModes);
	AQUA_LOG_DEBUG(Literal("VULKAN: Selected swapchain present mode: {}"), (int)presentMode);

	VkExtent2D extent = _SelectSwapchainExtent(swapchainSupportDetails.surfaceCapabilites);
	AQUA_LOG_DEBUG(Literal("VULKAN: Selected swapchain extent: width = {}, height = {}"), extent.width, extent.height);

	uint32_t imageCount = swapchainSupportDetails.surfaceCapabilites.minImageCount + 1;
	if (swapchainSupportDetails.surfaceCapabilites.maxImageCount > 0 &&
		imageCount > swapchainSupportDetails.surfaceCapabilites.maxImageCount) {
		imageCount = swapchainSupportDetails.surfaceCapabilites.maxImageCount;
	}
	
	AQUA_LOG_DEBUG(Literal("VULKAN: Swapchain surface image count: {}"), imageCount);

	VkSwapchainCreateInfoKHR createInfo{};
	createInfo.sType		    = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface		    = m_surface;
	createInfo.minImageCount    = imageCount;
	createInfo.imageFormat      = surfaceFormat.format;
	createInfo.imageColorSpace  = surfaceFormat.colorSpace;
	createInfo.imageExtent      = extent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	uint32_t requiredFamilies = QueueFamilyIndices::GRAPHICS_BIT | QueueFamilyIndices::PRESENT_BIT;
	AQUA_TRY(FindQueueFamilies(m_GPU, requiredFamilies), queueFamilies);

	if (!queueFamilies.GetValue().IsComplete(requiredFamilies)) {
		return Error::VULKAN_NO_SUITABLE_QUEUE_FAMILIES_FOUND;
	}
	uint32_t queueFamilyIndices[] = {
		queueFamilies.GetValue().GetFamilyIndex(QueueFamilyIndices::Family::GRAPHICS),
		queueFamilies.GetValue().GetFamilyIndex(QueueFamilyIndices::Family::PRESENT)
	};
	if (queueFamilyIndices[0] != queueFamilyIndices[1]) {
		createInfo.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices   = &queueFamilyIndices[0];
	}
	else {
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	}
	createInfo.preTransform   = swapchainSupportDetails.surfaceCapabilites.currentTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.presentMode    = presentMode;
	createInfo.clipped		  = VK_TRUE;
	createInfo.oldSwapchain   = VK_NULL_HANDLE;

	if (vkCreateSwapchainKHR(m_logicalDevice, &createInfo, m_allocator, &m_swapchain) != VK_SUCCESS) {
		return Error::VULKAN_FAILED_TO_CREATE_SWAPCHAIN;
	}

	VkResult enumerateResult = VK_SUCCESS;
	SafeArray<VkImage> images;
	do {
		if (vkGetSwapchainImagesKHR(m_logicalDevice, m_swapchain, &imageCount, nullptr) != VK_SUCCESS) {
			_DestroySwapchain();
			return Error::VULKAN_FAILED_TO_ENUMERATE_SWAPCHAIN_IMAGES;
		}
		AQUA_TRY(images.Resize(imageCount));
		enumerateResult = vkGetSwapchainImagesKHR(m_logicalDevice, m_swapchain, &imageCount, images.GetPtr());
	} while (enumerateResult == VK_INCOMPLETE);

	if (enumerateResult != VK_SUCCESS) {
		_DestroySwapchain();
		return Error::VULKAN_FAILED_TO_ENUMERATE_SWAPCHAIN_IMAGES;
	}
	m_swapchainImageFormat = surfaceFormat.format;
	m_swapchainExtent      = extent;

	AQUA_LOG_VULKAN(Literal("VULKAN: Swapchain is created"));

	return Success{};
}

aqua::Status aqua::VulkanAPI::_CreateSwapchainImageViews(const RenderAPICreateInfo&) noexcept {
	size_t swapchainImageCount = m_swapchainImages.GetSize();
	AQUA_TRY(m_swapchainImageViews.Resize(swapchainImageCount));

	size_t i = 0;
	for (; i < swapchainImageCount; ++i) {
		VkImageViewCreateInfo createInfo{};
		createInfo.sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		createInfo.image    = m_swapchainImages[i];
		createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		createInfo.format   = m_swapchainImageFormat;

		createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

		createInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
		createInfo.subresourceRange.baseMipLevel   = 0;
		createInfo.subresourceRange.levelCount     = 1;
		createInfo.subresourceRange.baseArrayLayer = 0;
		createInfo.subresourceRange.layerCount     = 1;

		if (vkCreateImageView(m_logicalDevice, &createInfo, m_allocator, &m_swapchainImageViews[i]) != VK_SUCCESS) {
			break;
		}
	}
	if (i < swapchainImageCount) {
		for (size_t j = 0; j < i; ++i) {
			vkDestroyImageView(m_logicalDevice, m_swapchainImageViews[i], m_allocator);
		}
		m_swapchainImages.DeepClear();
	}

	AQUA_LOG_VULKAN(Literal("VULKAN: Swapchain image views are created"));

	return Success{};
}

aqua::Status aqua::VulkanAPI::_CreateRenderPipelines(const RenderAPICreateInfo& info) noexcept {
	AQUA_TRY(m_renderPipelines.Reserve(info.renderPipelineCount));

	for (uint32_t i = 0; i < info.renderPipelineCount; ++i) {
		m_renderPipelines.EmplaceBackUnchecked();
		AQUA_TRY(m_renderPipelines.Last().Create(info.renderPipelineCreateInfos[i]));
	}

	AQUA_LOG_VULKAN(Literal("VULKAN: {} render pipelines are created"), info.renderPipelineCount);

	return Success{};
}

/*****************************************************************************************************************************
*********************************************** VulkanAPI 'destroy' methods **************************************************
/****************************************************************************************************************************/

void aqua::VulkanAPI::_DestroyVulkanInstance() noexcept {
	vkDestroyInstance(m_instance, m_allocator);
	m_instance = VK_NULL_HANDLE;

	AQUA_LOG_VULKAN(Literal("VULKAN: Vulkan instance is destroyed"));
}

void aqua::VulkanAPI::_DestroyDebugMessenger() noexcept {
	DestroyDebugUtilsMessengerEXT(m_instance, m_debugMessenger, m_allocator);
	m_debugMessenger = VK_NULL_HANDLE;

	AQUA_LOG_VULKAN(Literal("VULKAN: Debug messenger is destroyed"));
}

void aqua::VulkanAPI::_DestroySurface() noexcept {
	vkDestroySurfaceKHR(m_instance, m_surface, m_allocator);
	m_surface = VK_NULL_HANDLE;

	AQUA_LOG_VULKAN(Literal("VULKAN: Window surface is destroyed"));
}

void aqua::VulkanAPI::_UnbindGPU() noexcept {
	m_GPU = VK_NULL_HANDLE;
}

void aqua::VulkanAPI::_DestroyLogicalDevice() noexcept {
	vkDestroyDevice(m_logicalDevice, m_allocator);
	m_logicalDevice = VK_NULL_HANDLE;

	AQUA_LOG_VULKAN(Literal("VULKAN: Logical device is destroyed"));
}

void aqua::VulkanAPI::_DestroySwapchain() noexcept {
	vkDestroySwapchainKHR(m_logicalDevice, m_swapchain, m_allocator);
	m_swapchain = VK_NULL_HANDLE;

	AQUA_LOG_VULKAN(Literal("VULKAN: Swapchain is destroyed"));
}

void aqua::VulkanAPI::_DestroySwapchainImageViews() noexcept {
	for (const VkImageView& imageView : m_swapchainImageViews) {
		vkDestroyImageView(m_logicalDevice, imageView, m_allocator);
	}
	m_swapchainImageViews.DeepClear();

	AQUA_LOG_VULKAN(Literal("VULKAN: Swapchain image views are destroyed"));
}

void aqua::VulkanAPI::_DestroyRenderPipelines() noexcept {
	for (RenderPipeline& pipeline : m_renderPipelines) {
		pipeline.Destroy();
	}
	m_renderPipelines.DeepClear();
}

void aqua::VulkanAPI::_Terminate() noexcept {
	void(VulkanAPI::*terminateFuncs[])() = {
		&VulkanAPI::_DestroyRenderPipelines,
		&VulkanAPI::_DestroySwapchainImageViews,
		&VulkanAPI::_DestroySwapchain,
		&VulkanAPI::_DestroyLogicalDevice,
		&VulkanAPI::_UnbindGPU,
		&VulkanAPI::_DestroySurface,
		&VulkanAPI::_DestroyDebugMessenger,
		&VulkanAPI::_DestroyVulkanInstance
	};

	for (size_t i = 0; i < m_initializeCount; ++i) {
		((*this).*terminateFuncs[i])();
	}
	m_initializeCount = 0;

	m_allocator = nullptr;
	_UnloadCreateContext();
	g_VulkanAPI = nullptr;
}

void aqua::VulkanAPI::_LoadCreateContext() noexcept {
	g_VulkanAPI_CreateContext.allocator = m_allocator;
	g_VulkanAPI_CreateContext.logicalDevice = m_logicalDevice;
}

void aqua::VulkanAPI::_UnloadCreateContext() noexcept {
	g_VulkanAPI_CreateContext = {
		.allocator = nullptr,
		.logicalDevice = VK_NULL_HANDLE
	};
}

aqua::Expected<bool, aqua::Error> aqua::VulkanAPI::_IsGPUsuitable(const GPU& gpu) const noexcept {
	uint32_t requiredFamilies = QueueFamilyIndices::GRAPHICS_BIT | QueueFamilyIndices::PRESENT_BIT;
	AQUA_TRY(FindQueueFamilies(gpu, requiredFamilies), queueFamilies);

	if (!queueFamilies.GetValue().IsComplete(requiredFamilies)) {
		return Error::VULKAN_NO_SUITABLE_QUEUE_FAMILIES_FOUND;
	}

	AQUA_LOG_DEBUG_SYNC(Literal("VULKAN: GPU queue familes: graphics = {}, present = {}"),
		queueFamilies.GetValue().GetFamilyIndex(QueueFamilyIndices::Family::GRAPHICS),
		queueFamilies.GetValue().GetFamilyIndex(QueueFamilyIndices::Family::PRESENT)
	);

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
		AQUA_TRY(availableExtensions.Resize(extensionCount));
		enumerateResult = vkEnumerateDeviceExtensionProperties(gpu, nullptr, &extensionCount, availableExtensions.GetPtr());
	} while (enumerateResult == VK_INCOMPLETE);

	if (enumerateResult != VK_SUCCESS) {
		return Error::VULKAN_FAILED_TO_ENUMERATE_GPU_EXTENSION_PROPERTIES;
	}

	AQUA_LOG_DEBUG(Literal("VULKAN: Available GPU extensions found: {}"), extensionCount);

	for (const char* required : GPU_REQUIRED_EXTENSIONS) {
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

VkSurfaceFormatKHR aqua::VulkanAPI::_SelectSwapchainSurfaceFormat(const aqua::SafeArray<VkSurfaceFormatKHR>& formats) const noexcept {
	if (formats.GetSize() == 1 && formats.First().format == VK_FORMAT_UNDEFINED) { // any format
		return VkSurfaceFormatKHR{
			.format		= VK_FORMAT_B8G8R8A8_SRGB,
			.colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR
		};
	}
	VkFormat prefferedFormats[] = {
		VK_FORMAT_B8G8R8A8_SRGB,
		VK_FORMAT_R8G8B8A8_SRGB,
		VK_FORMAT_B8G8R8A8_UNORM,
		VK_FORMAT_R8G8B8A8_UNORM
	};
	for (const VkFormat& prefferedFormat : prefferedFormats) {
		for (const VkSurfaceFormatKHR& format : formats) {
			if (format.format == prefferedFormat && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
				return format;
			}
		}
	}
	// fallback
	return formats.First();
}

VkPresentModeKHR aqua::VulkanAPI::_SelectSwapchainPresentMode(const aqua::SafeArray<VkPresentModeKHR>& modes) const noexcept {
	for (const VkPresentModeKHR& mode : modes) {
		if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {
			return mode;
		}
	}
	// always available
	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D aqua::VulkanAPI::_SelectSwapchainExtent(const VkSurfaceCapabilitiesKHR& capabilities) const noexcept {
	if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
		return capabilities.currentExtent;
	}
	Vec2i windowSize = WindowSystem::Get().GetRenderWindowFramebufferSize();
	VkExtent2D actualExtent = {
		.width  = static_cast<uint32_t>(windowSize.x),
		.height = static_cast<uint32_t>(windowSize.y)
	};
	actualExtent.width = std::clamp(
		actualExtent.width,
		capabilities.minImageExtent.width,
		capabilities.maxImageExtent.width
	);
	actualExtent.height = std::clamp(
		actualExtent.height,
		capabilities.minImageExtent.height,
		capabilities.maxImageExtent.height
	);
	return actualExtent;
}