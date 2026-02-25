#ifndef AQUA_QUEUE_HEADER
#define AQUA_QUEUE_HEADER

#include <aqua/Error.h>

#include <cstdlib>

#ifndef AQUA_SUPPORT_QUEUE_IMPLEMENTATION
#include <deque>

namespace aqua {
	template <typename T>
	class Queue {
	public:
		Queue() = default;

	public:
		void Push(const T& value) { m_container.push_back(value); }
		void Push(T&& value) { m_container.push_back(std::move(value)); }

		void Pop() { m_container.pop_front(); }

		T&	     Get()		 { return m_container.front(); }
		const T& Get() const { return m_container.front(); }

		bool IsEmpty() const noexcept { return m_container.empty(); }

	private:
		std::deque<T> m_container;
	}; // class Queue
} // namespace aqua
#endif // !AQUA_SUPPORT_QUEUE_IMPLEMENTATION

namespace aqua {
	template <typename T, size_t BufferSize>
	requires (std::is_trivial_v<T>) // for simplicity
	class RingQueue {
	public:
		RingQueue() noexcept = default;
		RingQueue(const RingQueue&) noexcept = default;

		RingQueue(RingQueue&& other) noexcept : m_writeIndex(other.m_writeIndex), m_readIndex(m_readIndex) {
			std::memcpy(m_buffer, other.m_buffer, sizeof(T) * BufferSize);
			other.m_writeIndex = other.m_readIndex = 0;
		}

	public:
		void Push(const T& value) noexcept {
			m_buffer[m_writeIndex++] = value;
			m_writeIndex %= BufferSize;
			m_isFull = m_isFull || m_writeIndex == m_readIndex;
		}

		void Pop() noexcept {
			m_readIndex = (m_readIndex + 1) % BufferSize;
			m_isFull = false;
		}

		T&		 Get()		 noexcept { return m_buffer[m_readIndex]; }
		const T& Get() const noexcept { return m_buffer[m_readIndex]; }

		bool IsEmpty() const noexcept { return !m_isFull && m_writeIndex == m_readIndex; }
		bool IsFull() const noexcept  { return m_isFull; }

	private:
		bool   m_isFull				= false;
		size_t m_writeIndex         = 0;
		size_t m_readIndex		    = 0;
		T	   m_buffer[BufferSize] = {};
	}; // class RingQueue
}

#endif // !AQUA_QUEUE_HEADER