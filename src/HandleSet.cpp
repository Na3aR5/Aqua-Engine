#include <aqua/pch.h>
#include <aqua/datastructures/HandleSet.h>

bool aqua::_datastructures::HandleSetBase::Handle::operator==(const Handle& other) const noexcept {
	return m_id == other.m_id && m_gen == other.m_gen;
}

size_t aqua::_datastructures::HandleSetBase::Slot::GetGeneration() const noexcept {
	return m_gen;
}

size_t aqua::_datastructures::HandleSetBase::Slot::GetIndex() const noexcept {
	return m_index >> 1;
}

void aqua::_datastructures::HandleSetBase::Slot::Empty() noexcept {
	m_index &= ~(size_t)1;
}

void aqua::_datastructures::HandleSetBase::Slot::Reuse() noexcept {
	m_index |= 1;
}

bool aqua::_datastructures::HandleSetBase::Slot::IsEmpty() const noexcept {
	return (m_index & 1) == 0;
}

void aqua::_datastructures::HandleSetBase::Slot::SetIndex(size_t index) noexcept {
	m_index = (index << 1) | 1;
}

void aqua::_datastructures::HandleSetBase::Slot::IncrementGeneration() noexcept {
	++m_gen;
}