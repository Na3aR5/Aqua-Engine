#ifndef AQUA_RENDER_HARDWARE_INTERFACE_HEADER
#define AQUA_RENDER_HARDWARE_INTERFACE_HEADER

#include <aqua/Build.h>
#include <aqua/Error.h>
#include <aqua/Config.h>
#include <aqua/engine/ForwardSystems.h>
#include <aqua/engine/MemorySystem.h>

namespace aqua {
	// Render interface for all render APIs
	class RenderHardwareInterface {
	public:
		struct Static {
			static void SetMainWindowHints(RenderAPI API) noexcept;
		};

	public:
		~RenderHardwareInterface();

		RenderHardwareInterface(const RenderHardwareInterface&) = delete;
		RenderHardwareInterface(RenderHardwareInterface&&) noexcept = delete;

		RenderHardwareInterface& operator=(const RenderHardwareInterface&) = delete;
		RenderHardwareInterface& operator=(RenderHardwareInterface&&) noexcept = delete;

	public:
		RenderHardwareInterface& Get() noexcept;
		const RenderHardwareInterface& GetConst() noexcept;

	private:
		friend class Engine;
		friend class VulkanAPI;

		RenderHardwareInterface(const Config&, Status&);

	private:
		RenderAPI                 m_renderAPI;
		MemorySystem::VoidPointer m_API = nullptr;
	}; // class RenderHardwareInterface

	using RHI = RenderHardwareInterface;
} // namespace aqua

#endif // !AQUA_RENDER_HARDWARE_INTERFACE_HEADER