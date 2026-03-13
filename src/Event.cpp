#include <aqua/pch.h>
#include <aqua/engine/Event.h>

aqua::EventSet aqua::EventSet::AllEvents() noexcept {
	return EventSet().Add(Event::WINDOW_CLOSE);
}

aqua::EventSet& aqua::EventSet::Add(Event event) noexcept {
	int intEvent = (int)event;
	events[intEvent >> 6] |= (uint64_t)1 << (intEvent % 64);
	return *this;
}

void aqua::EventSet::Remove(Event event) noexcept {
	int intEvent = (int)event;
	events[intEvent >> 6] &= ~((uint64_t)1 << (intEvent % 64));
}

bool aqua::EventSet::Has(Event event) const noexcept {
	int intEvent = (int)event;
	return (bool)(events[intEvent >> 6] & ((uint64_t)1 << (intEvent % 64)));
}