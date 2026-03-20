#ifndef AQUA_RENDER_HARDWARE_INTERFACE_HEADER
#define AQUA_RENDER_HARDWARE_INTERFACE_HEADER

#include <aqua/Build.h>
#include <aqua/Error.h>
#include <aqua/Config.h>
#include <aqua/engine/ForwardSystems.h>
#include <aqua/engine/MemorySystem.h>
#include <aqua/datastructures/Array.h>

namespace aqua {
	// Render interface for any render APIs
	class RenderHardwareInterface {
	public:
		~RenderHardwareInterface();

		RenderHardwareInterface(const RenderHardwareInterface&) = delete;
		RenderHardwareInterface(RenderHardwareInterface&&) noexcept = delete;

		RenderHardwareInterface& operator=(const RenderHardwareInterface&) = delete;
		RenderHardwareInterface& operator=(RenderHardwareInterface&&) noexcept = delete;

	public:
		static const RenderHardwareInterface& Get() noexcept;

	private:
		friend class Engine;
		friend class VulkanAPI;

		RenderHardwareInterface(const Config&, const RenderAPICreateInfo&, Status&);

	private:
		RenderAPI            m_renderAPI;
		MemorySystem::Handle m_API = nullptr;
	}; // class RenderHardwareInterface

	using RHI = RenderHardwareInterface;
} // namespace aqua

#endif // !AQUA_RENDER_HARDWARE_INTERFACE_HEADER