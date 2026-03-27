#ifndef AQUA_DEFINES_HEADER
#define AQUA_DEFINES_HEADER

#include <aqua/math/Vector.h>
#include <aqua/engine/Event.h>
#include <aqua/utility/StringLiteral.h>

#include <cstdint>

namespace aqua {
	struct Version {
		Version(uint16_t major, uint16_t minor, uint32_t patch) :
			major(major), minor(minor), patch(patch) {}

		uint16_t major = 1;
		uint16_t minor = 0;
		uint32_t patch = 0;
	}; // struct Version


	// All engine supported window APIs
	enum class WindowAPI {
		GLFW
	};

	// All engine supported render APIs
	enum class RenderAPI {
		VULKAN
	};

	struct RenderPipelineCreateInfo {
	public:
		struct ShaderAssetInfo {
			const char* sourcePath	   = nullptr; // API specific
			const char* reflectionPath = nullptr;
		};

	public:
		ShaderAssetInfo vertexShaderAsset = {};
		ShaderAssetInfo fragmentShaderAsset = {};
	}; // struct RenderPipelineCreateInfo

	struct RenderAPICreateInfo {
	public:
		uint32_t                  renderPipelineCount	    = 0;
		RenderPipelineCreateInfo* renderPipelineCreateInfos = nullptr;
	}; // struct RenderAPICreateInfo

	struct WindowSystemInfo {
		// Required
		const char* renderWindowTitle = nullptr;
		Vec2i       renderWindowSize  = {};

		// Optional
		WindowAPI windowAPI            = WindowAPI::GLFW;
		EventSet  renderWindowEventSet = {};
	}; // struct WindowInfo

	struct EngineInfo {
	public:
		enum Flag : uint64_t {
			WINDOW_FULLSCREEN_BIT = 0x1
		}; // enum Flag

	public:
		// Required
		WindowSystemInfo	windowSystem  = {};
		RenderAPICreateInfo renderAPIinfo = {};

		// Optional
		RenderAPI   renderAPI		   = RenderAPI::VULKAN;
		uint64_t    flags              = 0;
		const char* applicationName    = nullptr;
		Version     applicationVersion = Version(1, 0, 0);
	}; // struct EngineInfo

	struct EngineInternalInfo {
	public:
		StringLiteralPointer name;
		Version              version;
	}; // struct EngineInternalInfo
} // namespace aqua

namespace aqua {
	enum Constants {
		MAX_RENDER_PIPELINE_SHADER_STAGE_COUNT = 2
	}; // enum Constants
} // namespace aqua

#endif // !AQUA_DEFINES_HEADER