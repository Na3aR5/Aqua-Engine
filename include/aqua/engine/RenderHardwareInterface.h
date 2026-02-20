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
		using RHI_SetMainWindowHintsFunction = void(*)();

	public:
		~RenderHardwareInterface();

		RenderHardwareInterface(const RenderHardwareInterface&) = delete;
		RenderHardwareInterface(RenderHardwareInterface&&) noexcept = delete;

		RenderHardwareInterface& operator=(const RenderHardwareInterface&) = delete;
		RenderHardwareInterface& operator=(RenderHardwareInterface&&) noexcept = delete;

	public:
		RenderHardwareInterface& Get() noexcept;
		const RenderHardwareInterface& GetConst() noexcept;

	public:
		static RHI_SetMainWindowHintsFunction SetMainWindowHints;

	private:
		friend class Engine;
		RenderHardwareInterface(const Config&, RenderAPI, Status&);

	private:
		RenderAPI                 m_renderAPI;
		MemorySystem::VoidPointer m_API = nullptr;
	}; // class RenderHardwareInterface

	using RHI = RenderHardwareInterface;
} // namespace aqua

#endif // !AQUA_RENDER_HARDWARE_INTERFACE_HEADER