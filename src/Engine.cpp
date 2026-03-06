#include <aqua/pch.h>
#include <aqua/engine/Engine.h>
#include <aqua/Assert.h>

namespace {
	aqua::Engine* g_Engine = nullptr;
}

aqua::Engine::Engine(const EngineInfo& info, Status& status) noexcept :
m_config(info),
m_state(State::CRASHED), // temporary
m_memorySystem(status),
m_system(status),
m_logger(m_config, status),
m_eventSystem(status),
m_windowSystem(m_config, status),
m_layerSystem(status),
m_RHI(m_config, status) {
	AQUA_ASSERT(g_Engine == nullptr, Literal("Attempt to create another instance of Engine"));

	if (g_Engine != nullptr) {
		status.EmplaceError(Error::ATTEMPT_TO_CREATE_ANOTHER_ENGINE_INSTANCE);
		return;
	}
	if (!status.IsSuccess()) {
		return;
	}

	m_state  = State::CREATED;
	g_Engine = this;
}

aqua::Engine::~Engine() {
	g_Engine = nullptr;
}

aqua::Engine&       aqua::Engine::Get()      noexcept { return *g_Engine; }
const aqua::Engine& aqua::Engine::GetConst() noexcept { return *g_Engine; }

void aqua::Engine::StopMainLoopRequest() noexcept {
	m_state = State::STOPPED;
}

aqua::Status aqua::Engine::MainLoop() {
	if (m_state == State::CRASHED) {
		return Error::ATTEMPT_TO_START_CRASHED_ENGINE;
	}
	if (m_state == State::STARTED) {
		AQUA_LOG_WARNING(Literal("Attempt to start already started engine"));
		return Error::ATTEMPT_TO_START_ALREADY_STARTED_ENGINE;
	}

	m_state = State::STARTED;
	while (m_state == State::STARTED) {
		m_layerSystem._Update();
		m_layerSystem._Render();

		m_eventSystem._PollEvents();
		m_layerSystem._HandleEvents();
	}

	return Success{};
}