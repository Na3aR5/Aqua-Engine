#include <aqua/engine/EventSystem.h>
#include <aqua/Logger.h>
#include <aqua/Assert.h>

#include <GLFW/glfw3.h>

namespace {
	aqua::EventSystem* g_EventSystem = nullptr;
}

void(*aqua::EventSystem::s_SetWindowCallbackFuncs[(int)aqua::Event::EVENT_COUNT])(void*) = {
	[](void* w) {
		glfwSetWindowCloseCallback((GLFWwindow*)w, [](GLFWwindow* wnd) {
			EventSystem& eventSystem = EventSystem::Get();

			eventSystem.m_eventQueue.Push(_EventInfo{
				.event = Event::WINDOW_CLOSE,
				.data = EventData()
			});
		});
	}
};

void aqua::EventSystem::_PollEvents() const noexcept {
	glfwPollEvents();
}

aqua::Event aqua::EventSystem::_Dispatch() noexcept {
	const _EventInfo& info = m_eventQueue.Get();
	m_currentEventData = info.data;
	Event event = info.event;
	m_eventQueue.Pop();
	return event;
}

void aqua::EventSystem::_SetWindowCallbacks(void* wnd, const EventSet& events) const noexcept {
	for (int i = 0; i < (int)Event::EVENT_COUNT; ++i) {
		if (events.Has((Event)i)) {
			s_SetWindowCallbackFuncs[i](wnd);
		}
	}
}

aqua::EventSystem& aqua::EventSystem::Get() noexcept { return *g_EventSystem; }
const aqua::EventSystem& aqua::EventSystem::GetConst() noexcept { return *g_EventSystem; }

bool aqua::EventSystem::HasEvents() const noexcept {
	return !m_eventQueue.IsEmpty();
}

const aqua::EventData& aqua::EventSystem::GetCurrentEventData() const noexcept {
	return m_currentEventData;
}

aqua::EventSystem::EventSystem(Status& status) {
	AQUA_ASSERT(g_EventSystem == nullptr, Literal("Attempt to create another EventSystem instance"));

	if (!status.IsSuccess()) {
		return;
	}
	if (g_EventSystem != nullptr) {
		return;
	}
	g_EventSystem = this;

	AQUA_LOG(Literal("Engine event system is initialized"));
}

aqua::EventSystem::~EventSystem() {
	g_EventSystem = nullptr;
}