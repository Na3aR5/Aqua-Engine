#ifndef AQUA_DEFINES_HEADER
#define AQUA_DEFINES_HEADER

#include <aqua/math/Vector.h>
#include <aqua/engine/Event.h>

#include <cstdint>

namespace aqua {
	struct Version {
		Version(uint16_t major, uint16_t minor, uint32_t patch) :
			major(major), minor(minor), patch(patch) {}

		uint16_t major = 1;
		uint16_t minor = 0;
		uint32_t patch = 0;
	}; // struct Version

	enum class RenderAPI {
		VULKAN
	};

	struct WindowSystemInfo {
		// Required
		const char* mainWindowTitle = nullptr;
		Vec2i       mainWindowSize{};

		// Optional
		EventSet m_mainWindowEventSet{};
	}; // struct WindowInfo

	struct EngineInfo {
	public:
		enum Flag : uint64_t {
			WINDOW_FULLSCREEN_BIT = 0x1
		}; // enum Flag

	public:
		// Required
		WindowSystemInfo windowSystem{};

		// Optional
		RenderAPI renderAPI			   = RenderAPI::VULKAN;

		uint64_t    flags              = 0;
		const char* applicationName    = nullptr;
		Version     applicationVersion = Version(1, 0, 0);
	}; // struct EngineInfo
} // namespace aqua

#endif // !AQUA_DEFINES_HEADER