#ifndef AQUA_ENGINE_HEADER
#define AQUA_ENGINE_HEADER

#include <aqua/engine/MemorySystem.h>
#include <aqua/engine/WindowSystem.h>
#include <aqua/engine/LayerSystem.h>
#include <aqua/Logger.h>

namespace aqua {
	struct EngineInfo {
	public:
		enum Flag : uint64_t {
			WINDOW_FULLSCREEN_BIT = 0x1
		}; // enum Flag

	public:
		// Required fields
		WindowSystemInfo windowSystem{};

		// Optional fields
		uint64_t flags = 0;
	}; // struct EngineInfo

	// Engine main class
	class Engine {
	public:
		enum class State {
			CREATED,
			STARTED,
			STOPPED,
			CRASHED
		}; // enum State

	public:
		Engine(const EngineInfo& info, Status& status) noexcept;
		~Engine();

	public:
		static Engine&       Get()      noexcept;
		static const Engine& GetConst() noexcept;

		void StopMainLoopRequest() noexcept;

	public:
		Status MainLoop();

	private:
		Config	     m_config;
		EngineInfo   m_info;
		State	     m_state;

		MemorySystem m_memorySystem;
		Logger	     m_logger;
		WindowSystem m_windowSystem;
		EventSystem  m_eventSystem;
		LayerSystem  m_layerSystem;
	}; // class Engine
} // namespace aqua

#endif // !AQUA_ENGINE_HEADER