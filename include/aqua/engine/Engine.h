#ifndef AQUA_ENGINE_HEADER
#define AQUA_ENGINE_HEADER

#include <aqua/engine/MemorySystem.h>
#include <aqua/engine/WindowSystem.h>
#include <aqua/engine/LayerSystem.h>
#include <aqua/engine/RenderHardwareInterface.h>

#include <aqua/Logger.h>

namespace aqua {
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
		
		Engine(const Engine&) = delete;
		Engine(Engine&&) noexcept = delete;

		Engine& operator=(const Engine&) = delete;
		Engine& operator=(Engine&&) noexcept = delete;

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
		EventSystem  m_eventSystem;
		WindowSystem m_windowSystem;
		LayerSystem  m_layerSystem;
		RHI          m_RHI;
	}; // class Engine
} // namespace aqua

#endif // !AQUA_ENGINE_HEADER