#include <aqua/engine/Engine.h>
#include <aqua/Assert.h>

aqua::Engine* g_Engine = nullptr;

aqua::Engine::Engine(const EngineInfo& info, Status& status) noexcept :
m_config(),
m_info(info),
m_state(_State::CRASHED), // temporary
m_memorySystem(status),
m_logger(m_config, status) {
	AQUA_ASSERT(g_Engine == nullptr, Literal("Attempt to create another instance of Engine"));

	if (g_Engine != nullptr) {
		status.EmplaceError(Error::ATTEMPT_TO_CREATE_ANOTHER_ENGINE_INSTANCE);
		return;
	}

	if (!status.IsSuccess()) {
		return;
	}

	if (!glfwInit()) {
		status.EmplaceError(Error::FAILED_TO_INITIALIZE_GLFW_LIBRARY);
		return;
	}

	Status windowCreationStatus = m_window._Create(m_info.window);
	if (!windowCreationStatus.IsSuccess()) {
		glfwTerminate();
		status = windowCreationStatus;
		return;
	}

	m_state  = _State::CREATED;
	g_Engine = this;
	status.EmplaceValue();
}

aqua::Engine::~Engine() {
	m_window._Destroy();

	glfwTerminate();
}

aqua::Engine&       aqua::Engine::Get()      noexcept { return *g_Engine; }
const aqua::Engine& aqua::Engine::GetConst() noexcept { return *g_Engine; }

aqua::Status aqua::Engine::MainLoop() {
	if (m_state == _State::CRASHED) {
		return Unexpected<Error>(Error::ATTEMPT_TO_START_CRASHED_ENGINE);
	}
	if (m_state == _State::STARTED) {
		AQUA_LOG_WARNING(Literal("Attempt to start already started engine"));
		return Unexpected<Error>(Error::ATTEMPT_TO_START_ALREADY_STARTED_ENGINE);
	}

	m_state = _State::STARTED;
	while (m_state == _State::STARTED) {

	}

	return Success{};
}