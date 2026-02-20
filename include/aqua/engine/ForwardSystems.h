#ifndef AQUA_FORWARD_SYSTEMS_HEADER
#define AQUA_FORWARD_SYSTEMS_HEADER

#include <aqua/Build.h>

namespace aqua {
	class Engine;

	class MemorySystem;
	class WindowSystem;
	class EventSystem;
	class LayerSystem;

#if AQUA_VULKAN_GRAPHICS_API
	class VulkanAPI;
#endif // AQUA_VULKAN_GRAPHICS_API
} // namespace aqua

#endif // !AQUA_FORWARD_SYSTEMS_HEADER