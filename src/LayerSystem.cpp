#include <aqua/pch.h>
#include <aqua/engine/LayerSystem.h>

#include <aqua/Logger.h>
#include <aqua/Assert.h>

void aqua::LayerSystem::ILayer::SetEvents(const EventSet& eventSet) noexcept {
	m_eventSet = eventSet;
}

bool aqua::LayerSystem::ILayer::IsEventNeedToHandle(Event e) const noexcept {
	return m_eventSet.Has(e);
}

bool aqua::LayerSystem::ILayer::IsCurrentEventBlocked() const noexcept {
	bool blocked = m_blockCurrentEvent;
	m_blockCurrentEvent = false;
	return blocked;
}

void aqua::LayerSystem::ILayer::BlockCurrentEvent() noexcept {
	m_blockCurrentEvent = true;
}

namespace {
	aqua::LayerSystem* g_LayerSystem = nullptr;
}

aqua::LayerSystem::LayerSystem(Status& status) {
	AQUA_ASSERT(g_LayerSystem == nullptr, Literal("Attempt to create another LayerSystem instance"));

	if (!status.IsSuccess() || g_LayerSystem != nullptr) {
		return;
	}
	g_LayerSystem = this;

	AQUA_LOG(Literal("Engine layer system is initialized"));
}

aqua::LayerSystem::~LayerSystem() {
	g_LayerSystem = nullptr;
}

aqua::LayerSystem& aqua::LayerSystem::Get() noexcept { return *g_LayerSystem; }
const aqua::LayerSystem& aqua::LayerSystem::GetConst() noexcept { return *g_LayerSystem; }

aqua::Status aqua::LayerSystem::_HandleEvents() noexcept {
	EventSystem& eventSystem = EventSystem::Get();

	while (eventSystem.HasEvents()) {
		Event event = eventSystem._Dispatch();

		for (UniqueData<ILayer>& layer : m_layers) {
			if (layer->IsEventNeedToHandle(event)) {
				if (!layer->OnEvent(event)) {
					return Error::LAYER_FAILED_TO_HANDLE_EVENT;
				}
				if (layer->IsCurrentEventBlocked()) {
					break;
				}
			}
		}
	}
	return Success{};
}

aqua::Status aqua::LayerSystem::_Update() noexcept {
	for (UniqueData<ILayer>& layer : m_layers) {
		if (!layer->OnUpdate()) {
			return Error::LAYER_FAILED_TO_UPDATE;
		}
	}
	return Success{};
}

aqua::Status aqua::LayerSystem::_Render() noexcept {
	for (UniqueData<ILayer>& layer : m_layers) {
		if (!layer->OnRender()) {
			return Error::LAYER_FAILED_TO_RENDER;
		}
	}
	return Success{};
}