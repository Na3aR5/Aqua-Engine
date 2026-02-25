#include <aqua/engine/EventSystem.h>
#include <aqua/engine/WindowSystem.h>
#include <aqua/Logger.h>
#include <aqua/Assert.h>

namespace {
	aqua::EventSystem* g_EventSystem = nullptr;
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

	if (!status.IsSuccess() || g_EventSystem != nullptr) {
		return;
	}
	g_EventSystem = this;

	AQUA_LOG(Literal("Engine event system is initialized"));
}

aqua::EventSystem::~EventSystem() {
	g_EventSystem = nullptr;
}

aqua::Event aqua::EventSystem::_Dispatch() noexcept {
	const _EventInfo& info = m_eventQueue.Get();
	m_currentEventData = info.data;
	Event event = info.event;
	m_eventQueue.Pop();
	return event;
}

void aqua::EventSystem::_RegisterEvent(Event event, const EventData& data) noexcept {
	if (m_eventQueue.IsFull()) {
		return; // drop (almost never happened)
	}
	m_eventQueue.Push(_EventInfo{ event, data });
}

void aqua::EventSystem::_PollEvents() noexcept {
	WindowSystem::Get().m_private.PollEvents();
}