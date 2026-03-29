#ifndef AQUA_UTILITY_SERIALIZER_HEADER
#define AQUA_UTILITY_SERIALIZER_HEADER

#include <aqua/Error.h>
#include <aqua/datastructures/Array.h>

#include <fstream>

namespace aqua {
	class Serializer {
	public:
		Serializer() = default;
		~Serializer();

		Status SetTargetFile(const char* filePath, bool clearContents = true) noexcept;
		const char* SetDeserializeSource(const char* source) noexcept;

	public:
		template <typename T, typename SizeT> requires (std::is_trivial_v<T> && std::is_unsigned_v<SizeT>)
		Status SerializeArray(const T* array, SizeT size) noexcept {
			if (!m_serializeStream) {
				return Error::FAILED_TO_WRITE;
			}
			m_serializeStream.write((const char*)&size, sizeof(SizeT));
			if (size > 0) {
				m_serializeStream.write((const char*)array, sizeof(T) * size);
			}
			if (!m_serializeStream) {
				return Error::FAILED_TO_WRITE;
			}
			return Success{};
		}

		template <typename T> requires (std::is_trivial_v<T>)
		Status SerializeTrivial(const T& value) noexcept {
			if (!m_serializeStream) {
				return Error::FAILED_TO_WRITE;
			}
			m_serializeStream.write((const char*)&value, sizeof(T));
			if (!m_serializeStream) {
				return Error::FAILED_TO_WRITE;
			}
			return Success{};
		}

		template <typename T>
		T DeserializeTrivial() noexcept {
			T value = *(const T*)m_deserializeSource;
			m_deserializeSource += sizeof(T);
			return value;
		}

		template <typename Array, typename SizeT> requires (std::is_unsigned_v<SizeT>)
		Expected<Array, Error> DeserializeArray() noexcept {
			Array array{};
			SizeT size = DeserializeTrivial<SizeT>();
			AQUA_TRY(array.Reserve(size));

			typename Array::ValueType* arrayData = (typename Array::ValueType*)m_deserializeSource;
			for (SizeT i = 0; i < size; ++i) {
				array.EmplaceBackUnchecked(*(arrayData + i));
			}
			m_deserializeSource += sizeof(typename Array::ValueType) * size;

			return Expected<Array, Error>(std::move(array));
		}

		template <typename Array, typename SizeT> requires (std::is_unsigned_v<SizeT>)
		Status DeserializeArray(Array& array) noexcept {
			SizeT size = DeserializeTrivial<SizeT>();
			AQUA_TRY(array.Reserve(size));

			const typename Array::ValueType* arrayData = (const typename Array::ValueType*)m_deserializeSource;
			for (SizeT i = 0; i < size; ++i) {
				array.EmplaceBackUnchecked(*(arrayData + i));
			}
			m_deserializeSource += sizeof(typename Array::ValueType) * size;

			return Success{};
		}

		template <typename String, typename SizeT> requires (std::is_unsigned_v<SizeT>)
		Status DeserializeString(String& string) noexcept {
			SizeT size = DeserializeTrivial<SizeT>();
			const typename String::ValueType* stringData = (const typename String::ValueType*)m_deserializeSource;
			AQUA_TRY(string.Set(stringData, size));

			m_deserializeSource += sizeof(typename String::ValueType) * size;

			return Success{};
		}

	private:
		const char*   m_deserializeSource = nullptr;
		std::ofstream m_serializeStream;
	};
} // namespace aqua

#endif // !AQUA_UTILITY_SERIALIZER_HEADER