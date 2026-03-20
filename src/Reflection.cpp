#include <aqua/pch.h>
#include <aqua/Reflection.h>
#include <aqua/System.h>
#include <aqua/datastructures/String.h>

#include <fstream>

namespace {
	template <typename ArrayType>
	aqua::Status DeserializeArray(const char*& source, ArrayType& array) noexcept {
		using ValueType = typename ArrayType::ValueType;

		uint32_t size = *(const uint32_t*)source;
		AQUA_TRY(array.Reserve(size));

		source += sizeof(uint32_t);
		for (uint32_t i = 0; i < size; ++i) {
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
	
	AQUA_TRY(DeserializeArray(sourcePtr, reflection.vertexLayouts));

	uint32_t descriptorSetCount = *(const uint32_t*)sourcePtr;
	AQUA_TRY(reflection.descriptorSets.Reserve(descriptorSetCount));

	sourcePtr += sizeof(uint32_t);
	for (uint32_t i = 0; i < descriptorSetCount; ++i) {
		uint32_t set = *(const uint32_t*)sourcePtr;
		sourcePtr += sizeof(uint32_t);

		reflection.descriptorSets.EmplaceBackUnchecked();
		reflection.descriptorSets.Last().set = set;

		DeserializeArray(sourcePtr, reflection.descriptorSets.Last().bindings);
	}
	AQUA_TRY(DeserializeArray(sourcePtr, reflection.pushConstants));

	reflection.incomplete = false;
	return Expected<ShaderReflection, Error>(std::move(reflection));
}