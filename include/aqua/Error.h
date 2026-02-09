#ifndef AQUA_ERROR_HEADER
#define AQUA_ERROR_HEADER

#include <new>
#include <cstdint>
#include <type_traits>

namespace aqua {
	enum class Error {
		// Logic
		ITERATOR_OR_INDEX_OUT_OF_RANGE,

		// Memory
		FAILED_TO_ALLOCATE_MEMORY,

		// Library
		FAILED_TO_INITIALIZE_GLFW_LIBRARY,
		FAILED_TO_LOAD_OPENGL_LIBRARY,

		// Engine
		ATTEMPT_TO_CREATE_ANOTHER_ENGINE_INSTANCE,
		ATTEMPT_TO_START_CRASHED_ENGINE,
		ATTEMPT_TO_START_ALREADY_STARTED_ENGINE,

		// Engine.Logger
		FAILED_TO_CREATE_LOGGER_OWN_CONSOLE,
		FAILED_TO_WRITE_LOG_MESSAGE,

		// Engine.WindowSystem
		INCORRECT_WINDOW_RESOLUTION_OR_TITLE_IS_NULLPTR,
		FAILED_TO_CREATE_WINDOW_INSTANCE,

		// Engine.LayerSystem
		LAYER_FAILED_TO_HANDLE_EVENT,
		LAYER_FAILED_TO_UPDATE,
		LAYER_FAILED_TO_RENDER,

		// Data Structure
		DATA_STRUCTURE_FAILED_TO_COPY_ALLOCATOR,
	}; // enum Error

	template <typename ErrorT>
	class Unexpected {
	public:
		using ErrorType = ErrorT;

	public:
		Unexpected(const ErrorType& error) noexcept requires(std::is_nothrow_copy_constructible_v<ErrorType>) :
			m_error(error) {}

		Unexpected(ErrorType&& error) noexcept : m_error(std::move(error)) {}

	public:
		ErrorType&		 GetError()		  noexcept { return m_error; }
		const ErrorType& GetError() const noexcept { return m_error; }

	private:
		ErrorType m_error;
	}; // class Unexpected

	template <typename T, typename ErrorT>
	class Expected {
		using ValueType = T;
		using ErrorType = ErrorT;

	public:
		static_assert(!std::is_same_v<ValueType, ErrorType>,
			"aqua::Expected - ValueType and ErrorType must be different");

	public:
		Expected() noexcept requires(std::is_nothrow_default_constructible_v<ValueType>) :
			m_value(), m_hasValue(true) {}

		Expected(const Expected&) noexcept requires(
			std::is_trivially_copy_constructible_v<ValueType> &&
			std::is_trivially_copy_constructible_v<ErrorType>) = default;

		Expected(const Expected& other) requires(
			!(std::is_trivially_copy_constructible_v<ValueType> &&
			  std::is_trivially_copy_constructible_v<ErrorType>)) : m_hasValue(other.m_hasValue) {
			static_assert(std::is_nothrow_copy_constructible_v<ValueType> && std::is_nothrow_copy_constructible_v<ErrorType>,
				"aqua::Expected - ValueType and ErrorType must be nothrow copy constructible");

			if (other.m_hasValue) {
				new (&m_value) ValueType(other.m_value);
			}
			else {
				new (&m_error) ErrorType(other.m_error);
			}
		}

		Expected(Expected&& other) noexcept requires(
			!(std::is_trivially_move_constructible_v<ValueType> &&
			  std::is_trivially_move_constructible_v<ErrorType>)) : m_hasValue(other.m_hasValue) {
			if (other.m_hasValue) {
				new (&m_value) ValueType(std::move(other.m_value));
			}
			else {
				new (&m_error) ErrorType(std::move(other.m_error));
			}
		}

		Expected(const Unexpected<ErrorType>& unexpected) noexcept :
			m_hasValue(false), m_error(unexpected.GetError()) {}
		Expected(Unexpected<ErrorType>&& unexpected) noexcept :
			m_hasValue(false), m_error(std::move(unexpected.GetError())) {}

		Expected(const ValueType& value) noexcept requires(std::is_nothrow_copy_constructible_v<ValueType>) :
			m_value(value), m_hasValue(true) {}
		Expected(ValueType&& value) noexcept : m_value(std::move(value)), m_hasValue(true) {}

		Expected(const ErrorType& error) noexcept requires(std::is_nothrow_copy_constructible_v<ErrorType>) :
			m_error(error), m_hasValue(false) {}
		Expected(ErrorType&& error) noexcept : m_error(std::move(error)), m_hasValue(false) {}

		template <typename U>
		Expected(const U&) requires(!std::is_same_v<std::decay_t<U>, T>) = delete;

		template <typename U>
		Expected(U&&) requires(!std::is_same_v<std::decay_t<U>, ValueType> &&
							   !std::is_same_v<std::decay_t<U>, Expected>) = delete;

		~Expected() { _Destroy(); }

	public:
		Expected& operator=(const Expected&) noexcept requires(
			std::is_trivially_copy_constructible_v<ValueType> &&
			std::is_trivially_copy_constructible_v<ErrorType>) = default;

		Expected& operator=(const Expected& other) requires(
			!(std::is_trivially_copy_constructible_v<ValueType> &&
			  std::is_trivially_copy_constructible_v<ErrorType>)) {
			static_assert(std::is_nothrow_copy_constructible_v<ValueType> && std::is_nothrow_copy_constructible_v<ErrorType>,
				"aqua::Expected - ValueType and ErrorType must be nothrow copy constructible");

			_Destroy();
			if (other.m_hasValue) {
				new (&m_value) ValueType(other.m_value);
			}
			else {
				new (&m_error) ErrorType(other.m_error);
			}
			return *this;
		}

		Expected& operator=(Expected&& other) noexcept requires(
			!(std::is_trivially_copy_constructible_v<ValueType> &&
			  std::is_trivially_copy_constructible_v<ErrorType>)) {
			_Destroy();
			if (other.m_hasValue) {
				new (&m_value) ValueType(std::move(other.m_value));
			}
			else {
				new (&m_error) ErrorType(std::move(other.m_error));
			}
			return *this;
		}

	public:
		bool HasValue() const noexcept { return m_hasValue; }

		ValueType&		 GetValue()		  { return m_value; }
		const ValueType& GetValue() const { return m_value; }

		ErrorType&		 GetError()		  { return m_error; }
		const ErrorType& GetError() const { return m_error; }

		aqua::Unexpected<ErrorType> Unexpected() const {
			return aqua::Unexpected<ErrorType>(m_error);
		}

	public:
		template <typename ... Types>
		void EmplaceValue(Types&& ... args) {
			static_assert(std::is_nothrow_constructible_v<ValueType, Types...>,
				"aqua::Expected - ValueType must be nothrow constructible with specified arguments");

			_Destroy();
			new (&m_value) ValueType(std::forward<Types>(args)...);
			m_hasValue = true;
		}

		template <typename ... Types>
		void EmplaceError(Types&& ... args) {
			static_assert(std::is_nothrow_constructible_v<ErrorType, Types...>,
				"aqua::Expected - ErrorType must be nothrow constructible with specified arguments");

			_Destroy();
			new (&m_error) ErrorType(std::forward<Types>(args)...);
			m_hasValue = false;
		}

	private:
		void _Destroy() noexcept {
			if (m_hasValue) {
				if constexpr (!std::is_trivially_destructible_v<ValueType>) {
					m_value.~ValueType();
				}
			}
			else {
				if constexpr (!std::is_trivially_destructible_v<ErrorType>) {
					m_error.~ErrorType();
				}
			}
		}

	private:
		union {
			T      m_value;
			ErrorT m_error;
		};
		bool m_hasValue = false;
	}; // class Expected

	struct Success {};

	// Expected specialization, that stores success / error of some operation
	template <typename ErrorT>
	class Expected<Success, ErrorT> {
	public:
		using ValueType = Success;
		using ErrorType = ErrorT;

	public:
		Expected() noexcept : _(), m_hasValue(true) {}
		Expected(const Expected&) noexcept requires(std::is_trivially_copy_constructible_v<ErrorType>) = default;

		Expected(const Expected& other) requires(!std::is_trivially_copy_constructible_v<ErrorType>) :
		m_hasValue(other.m_hasValue) {
			static_assert(std::is_trivially_copy_constructible_v<ErrorType>,
				"aqua::Expected - ErrorType must be nothrow copy constructible");

			if (!m_hasValue) {
				new (&m_error) ErrorType(other.m_error);
			}
		}

		Expected(Expected&& other) noexcept requires(!std::is_trivially_move_constructible_v<ErrorType>) :
		m_hasValue(other.m_hasValue) {
			if (!m_hasValue) {
				new (&m_error) ErrorType(std::move(other.m_error));
			}
		}

		Expected(Success) noexcept : _(), m_hasValue(true) {}

		Expected(const Unexpected<ErrorType>& unexpected) noexcept : m_error(unexpected.GetError()), m_hasValue(false) {}
		Expected(Unexpected<ErrorType>&& unexpected) noexcept :
			m_error(std::move(unexpected.GetError())), m_hasValue(false) {
		}

		Expected(const ErrorType& error) noexcept requires(std::is_nothrow_copy_constructible_v<ErrorType>) :
			m_error(error), m_hasValue(false) {}
		Expected(ErrorType&& error) noexcept : m_error(std::move(error)), m_hasValue(false) {}

		template <typename U>
		Expected(const U&) requires(!std::is_same_v<std::decay_t<U>, Success>) = delete;

		template <typename U>
		Expected(U&&) requires(!std::is_same_v<std::decay_t<U>, Success> &&
							   !std::is_same_v<std::decay_t<U>, Expected>) = delete;

		~Expected() { _Destroy(); }

	public:
		Expected& operator=(const Expected&) noexcept requires(std::is_trivially_copy_assignable_v<ErrorType>) = default;
		Expected& operator=(const Expected& other) noexcept requires(!std::is_trivially_copy_assignable_v<ErrorType>) {
			static_assert(std::is_trivially_copy_constructible_v<ErrorType>,
				"aqua::Expected - ErrorType must be nothrow copy constructible");

			_Destroy();
			if (!other.m_hasValue) {
				new (&m_error) ErrorType(other.m_error);
			}
			return *this;
		}

		Expected& operator=(Expected&& other) noexcept requires(!std::is_trivially_move_constructible_v<ErrorType>) {
			_Destroy();
			if (!other.m_hasValue) {
				new (&m_error) ErrorType(std::move(other.m_error));
			}
			return *this;
		}

	public:
		bool HasValue()  const noexcept { return m_hasValue; }
		bool IsSuccess() const noexcept { return m_hasValue; }

		ErrorType& GetError()			  { return m_error; }
		const ErrorType& GetError() const { return m_error; }

		aqua::Unexpected<ErrorType> Unexpected() const {
			return aqua::Unexpected<ErrorType>(m_error);
		}

	public:
		void EmplaceValue() noexcept {
			_Destroy();
			m_hasValue = true;
		}

		template <typename ... Types>
		void EmplaceError(Types&& ... args) noexcept requires(std::is_nothrow_constructible_v<ErrorType, Types...>) {
			_Destroy();
			new (&m_error) ErrorType(std::forward<Types>(args)...);
			m_hasValue = false;
		}

	private:
		void _Destroy() noexcept {
			if (!m_hasValue) {
				if constexpr (std::is_trivially_destructible_v<ErrorType>) {
					m_error.~ErrorType();
				}
			}
		}

	private:
		union {
			char _;
			ErrorType m_error;
		};
		bool m_hasValue = false;
	}; // class Expected<Success, ErrorT>

	using Status = Expected<Success, Error>;
} // namespace aqua

#define AQUA_TRY(expression, resultName) \
	auto resultName = (expression);      \
	if (!resultName.HasValue()) {        \
		return resultName.Unexpected();  \
	}

#endif // !AQUA_ERROR_HEADER