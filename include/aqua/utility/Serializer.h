#ifndef AQUA_UTILITY_SERIALIZER_HEADER
#define AQUA_UTILITY_SERIALIZER_HEADER

#include <aqua/Error.h>
#include <aqua/datastructures/String.h>
#include <aqua/datastructures/Array.h>

#include <tuple>
#include <fstream>

namespace aqua {
	class Serializer {
	public:
		template <typename ObjectType>
		struct NeedDefinedAlgorithm {
			using Need = std::false_type;
		};

		template <typename ObjectType>
		struct ObjectDeserializer {
		public:
			using CanFail = std::false_type;

			ObjectType operator()(const char*& source) const noexcept {
				ObjectType object = *(const ObjectType*)source;
				source += sizeof(ObjectType);
				return object;
			}
		};

		template <typename ObjectType>
		struct ObjectSerializer {
		public:
			Status operator()(std::ofstream& fs, const ObjectType& object) const noexcept {
				fs.write((const char*)&object, sizeof(ObjectType));
				if (!fs) {
					return Error::FAILED_TO_WRITE;
				}
				return Success{};
			}
		};

	public:
		Serializer() = default;
		~Serializer();

		Status SetTargetFile(const char* filePath, bool clearContents = true) noexcept;

	public:
		template <typename ObjectType>
		Status Serialize(const ObjectType& object) noexcept {
			return ObjectSerializer<ObjectType>()(m_serializeStream, object);
		}

		template <typename ObjectType>
		Expected<ObjectType, Error> Deserialize(const char* source) noexcept {
			return std::move(ObjectDeserializer<ObjectType>()(source));
		}

	private:
		std::ofstream m_serializeStream;
	}; // class Serializer

	template <typename ... Types>
	struct Serializer::NeedDefinedAlgorithm<SafeArray<Types...>> {
		using Need = std::true_type;
	};
	template <typename ... Types>
	struct Serializer::NeedDefinedAlgorithm<BasicSafeString<Types...>> {
		using Need = std::true_type;
	};

	template <typename ... Types>
	struct Serializer::ObjectSerializer<SafeArray<Types...>> {
	public:
		Status operator()(std::ofstream& fs, const SafeArray<Types...>& array) const noexcept {
			size_t size = array.GetSize();
			fs.write((const char*)&size, sizeof(size_t));

			using ValueType = typename SafeArray<Types...>::ValueType;

			if constexpr (std::is_same_v<std::true_type, typename NeedDefinedAlgorithm<ValueType>::Need>) {
				ObjectSerializer<ValueType> serializer;
				for (size_t i = 0; i < size; ++i) {
					AQUA_TRY(serializer(fs, array[i]));
				}
			}
			else {
				const ValueType* arrayData = array.GetPtr();
				fs.write((const char*)arrayData, sizeof(ValueType) * size);
				if (!fs) {
					return Error::FAILED_TO_WRITE;
				}
			}
			return Success{};
		}
	}; // SafeArray serializer

	template <typename ... Types>
	struct Serializer::ObjectSerializer<BasicSafeString<Types...>> {
	public:
		Status operator()(std::ofstream& fs, const BasicSafeString<Types...>& string) const noexcept {
			size_t size = string.GetSize();
			fs.write((const char*)&size, sizeof(size_t));

			using ValueType = typename BasicSafeString<Types...>::ValueType;
			const ValueType* stringData = string.GetPtr();
			fs.write((const char*)stringData, sizeof(ValueType) * size);

			if (!fs) {
				return Error::FAILED_TO_WRITE;
			}
			return Success{};
		}
	}; // String serializer

	template <typename ... Types>
	struct Serializer::ObjectDeserializer<SafeArray<Types...>> {
	public:
		using CanFail = std::true_type;

		Expected<SafeArray<Types...>, Error> operator()(const char*& source) const noexcept {
			size_t size = *(const size_t*)source;
			source += sizeof(size_t);

			SafeArray<Types...> result;
			AQUA_TRY(result.Reserve(size));

			for (size_t i = 0; i < size; ++i) {
				using ValueType = typename SafeArray<Types...>::ValueType;

				if constexpr (std::is_same_v<std::true_type, typename NeedDefinedAlgorithm<ValueType>::Need>) {
					ObjectDeserializer<ValueType> deserializer{};
					if constexpr (std::is_same_v<std::true_type, typename ObjectDeserializer<ValueType>::CanFail>) {
						AQUA_TRY(deserializer(source), expectedValue);
						result.EmplaceBackUnchecked(std::move(expectedValue.GetValue()));
					}
					else {
						result.EmplaceBackUnchecked(deserializer(source));
					}
				}
				else {
					result.EmplaceBackUnchecked(*(const ValueType*)source);
					source += sizeof(ValueType);
				}
			}
			return Expected<SafeArray<Types...>, Error>(std::move(result));
		}
	}; // SafeArray deserializer

	template <typename ... Types>
	struct Serializer::ObjectDeserializer<BasicSafeString<Types...>> {
	public:
		using CanFail = std::true_type;

		Expected<BasicSafeString<Types...>, Error> operator()(const char*& source) const noexcept {
			size_t size = *(const size_t*)source;
			source += sizeof(size_t);

			BasicSafeString<Types...> result;
			AQUA_TRY(result.Set((const typename BasicSafeString<Types...>::ValueType*)source, size));
			source += size * sizeof(typename BasicSafeString<Types...>::ValueType);

			return Expected<BasicSafeString<Types...>, Error>(std::move(result));
		}
	}; // SafeString deserializer
} // namespace aqua

#endif // !AQUA_UTILITY_SERIALIZER_HEADER