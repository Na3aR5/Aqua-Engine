#ifndef AQUA_EVENT_SYSTEM_HEADER
#define AQUA_EVENT_SYSTEM_HEADER

#include <aqua/Error.h>
#include <aqua/engine/Event.h>
#include <aqua/engine/ForwardSystems.h>
#include <aqua/datastructures/Queue.h>

namespace aqua {
	class EventSystem {
	public:
		~EventSystem();

		EventSystem(const EventSystem&) = delete;
		EventSystem(EventSystem&&) noexcept = delete;

		EventSystem& operator=(const EventSystem&) = delete;
		EventSystem& operator=(EventSystem&&) noexcept = delete;

	public:
		static EventSystem& Get() noexcept;
		static const EventSystem& GetConst() noexcept;

		bool HasEvents() const noexcept;
		const EventData& GetCurrentEventData() const noexcept;

	private:
		void _PollEvents() noexcept;
		Event _Dispatch() noexcept;
		void _RegisterEvent(Event event, const EventData& data) noexcept;

	private:
		friend class Engine;
		friend class LayerSystem;
		friend class GLFW_API;

		EventSystem(Status&);

	private:
		struct _EventInfo {
			Event     event;
			EventData data;
		}; // struct _EventInfo

		EventData				    m_currentEventData;
		RingQueue<_EventInfo, 1024> m_eventQueue;
	}; // class EventSystem
} // namespace aqua

#endif // !AQUA_EVENT_SYSTEM_HEADER