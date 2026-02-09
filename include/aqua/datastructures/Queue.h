#ifndef AQUA_QUEUE_HEADER
#define AQUA_QUEUE_HEADER

#include <aqua/Error.h>

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
#endif // !AQUA_QUEUE_HEADER