#ifndef AQUA_EVENT_SYSTEM_HEADER
#define AQUA_EVENT_SYSTEM_HEADER

#include <aqua/Error.h>
#include <aqua/engine/ForwardSystems.h>
#include <aqua/engine/Event.h>
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
		void _PollEvents() const noexcept;
		Event _Dispatch() noexcept;
		void _SetWindowCallbacks(void* wnd, const EventSet& events) const noexcept;

	private:
		friend class Engine;
		friend class LayerSystem;
		friend class WindowSystem;

		EventSystem(Status&);

	private:
		struct _EventInfo {
			Event     event;
			EventData data;
		}; // struct _EventInfo

		static void(*s_SetWindowCallbackFuncs[(int)Event::EVENT_COUNT])(void*);

		EventData         m_currentEventData;
		Queue<_EventInfo> m_eventQueue;
	}; // class EventSystem
} // namespace aqua

#endif // !AQUA_EVENT_SYSTEM_HEADER