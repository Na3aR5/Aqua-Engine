#ifndef AQUA_EVENT_HEADER
#define AQUA_EVENT_HEADER

#include <cstdint>

#define AQUA_EVENT_COUNT_COMPRESS_TO_64_BITS \
	(((int)Event::EVENT_COUNT >> 6) + (int)((int)Event::EVENT_COUNT % 64 != 0))

namespace aqua {
	enum class Event {
		WINDOW_CLOSE,

		EVENT_COUNT
	}; // enum Event

	union EventData {

	}; // union EventData

	struct EventSet {
	public:
		EventSet() = default;

	public:
		EventSet& Add(Event event) noexcept;
		void Remove(Event event) noexcept;

		bool Has(Event event) const noexcept;

	public:
		uint64_t events[AQUA_EVENT_COUNT_COMPRESS_TO_64_BITS] = {};
	}; // struct EventSet
} // namespace aqua

#endif // !AQUA_EVENT_HEADER