#ifndef AQUA_ENGINE_HEADER
#define AQUA_ENGINE_HEADER

#include <aqua/engine/MemorySystem.h>
#include <aqua/engine/Window.h>
#include <aqua/Logger.h>

#include <aqua/datastructures/Array.h>

namespace aqua {
	struct EngineInfo {
	public:
		enum Flag : uint64_t {
			WINDOW_FULLSCREEN_BIT = 0x1
		}; // enum Flag

	public:
		// Required fields
		WindowInfo  window{};

		// Optional fields
		uint64_t    flags = 0;
	}; // struct EngineInfo

	// Interface for layer, that describes what main application actually does (from top layer to bottom)
	class ILayer {
	public:
		virtual void OnEvent()  = 0;
		virtual void OnUpdate() = 0;
		virtual void OnRender() = 0;
	}; // class ILayer

	// Engine main class
	class Engine {
	public:
		Engine(const EngineInfo& info, Status& status) noexcept;
		~Engine();

	public:
		static Engine&       Get()      noexcept;
		static const Engine& GetConst() noexcept;

		Status MainLoop();

	private:
		enum class _State {
			CREATED,
			STARTED,
			STOPPED,
			CRASHED
		}; // enum _State

	private:
		Config			   m_config;
		EngineInfo		   m_info;
		_State			   m_state;

		EngineMemorySystem m_memorySystem;
		Logger			   m_logger;
		Window			   m_window;
	}; // class Engine
} // namespace aqua

#endif // !AQUA_ENGINE_HEADER