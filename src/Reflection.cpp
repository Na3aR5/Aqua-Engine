#include <aqua/pch.h>
#include <aqua/Reflection.h>
#include <aqua/System.h>
#include <aqua/datastructures/String.h>

#include <fstream>

namespace {
	template <typename ArrayType>
	aqua::Status DeserializeArray(const char*& source, ArrayType& array) noexcept {
		using ValueType = typename ArrayType::ValueType;

		size_t size = *(const size_t*)source;
		AQUA_TRY(array.Reserve(size), _);

		source += sizeof(size_t);
		for (size_t i = 0; i < size; ++i) {
			array.EmplaceBackUnchecked(*(const ValueType*)source);
			source += sizeof(ValueType);
		}
		return aqua::Success{};
	}
}

aqua::Expected<aqua::ShaderReflection, aqua::Error> aqua::DeserializeShaderReflection(const char* path) noexcept {
	AQUA_TRY(aqua::System::Get().ReadFile(path), source);
	const char* sourcePtr = source.GetValue().GetPtr();

	ShaderReflection reflection{};
	
	AQUA_TRY(DeserializeArray(sourcePtr, reflection.vertexLayouts), _1);
	AQUA_TRY(DeserializeArray(sourcePtr, reflection.descriptorBindings), _2);
	AQUA_TRY(DeserializeArray(sourcePtr, reflection.pushConstants), _3);

	return Expected<ShaderReflection, Error>(std::move(reflection));
}