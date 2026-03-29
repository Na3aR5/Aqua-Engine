#include <aqua/pch.h>
#include <aqua/Reflection.h>
#include <aqua/System.h>
#include <aqua/utility/Serializer.h>
#include <aqua/datastructures/String.h>

#include <fstream>

#include <map>

aqua::Expected<aqua::ShaderReflection, aqua::Error> aqua::DeserializeShaderReflection(const char* path) noexcept {
	AQUA_TRY(aqua::System::Get().ReadFile(path), source);

	Serializer serializer;
	serializer.SetDeserializeSource(source.GetValue().GetPtr());

	ShaderReflection reflection{};
	AQUA_TRY((serializer.DeserializeString<decltype(reflection.entryPointName), uint32_t>(reflection.entryPointName)));
	AQUA_TRY((serializer.DeserializeArray<decltype(reflection.vertexLayouts), uint32_t>(reflection.vertexLayouts)));

	uint32_t descriptorSetCount = serializer.DeserializeTrivial<uint32_t>();
	AQUA_TRY(reflection.descriptorSets.Reserve(descriptorSetCount));

	for (uint32_t i = 0; i < descriptorSetCount; ++i) {
		uint32_t set = serializer.DeserializeTrivial<uint32_t>();
		reflection.descriptorSets.EmplaceBackUnchecked();
		reflection.descriptorSets.Last().set = set;

		AQUA_TRY((serializer.DeserializeArray<decltype(reflection.descriptorSets[0].bindings), uint32_t>(
			reflection.descriptorSets.Last().bindings
		)));
	}
	AQUA_TRY((serializer.DeserializeArray<decltype(reflection.pushConstants), uint32_t>(reflection.pushConstants)));

	reflection.incomplete = false;
	return Expected<ShaderReflection, Error>(std::move(reflection));
}