#include <aqua/pch.h>
#include <aqua/Reflection.h>
#include <aqua/System.h>
#include <aqua/utility/Serializer.h>
#include <aqua/datastructures/String.h>

template <>
struct aqua::Serializer::NeedDefinedAlgorithm<aqua::ShaderReflection> {
	using Need = std::true_type;
};

template <>
struct aqua::Serializer::NeedDefinedAlgorithm<aqua::ShaderReflection::DescriptorSet> {
	using Need = std::true_type;
};

template <>
struct aqua::Serializer::ObjectDeserializer<aqua::ShaderReflection::DescriptorSet> {
public:
	using CanFail = std::true_type;

	aqua::Expected<aqua::ShaderReflection::DescriptorSet, aqua::Error> operator()(const char*& source) const noexcept {
		ShaderReflection::DescriptorSet set;
		set.set = ObjectDeserializer<decltype(set.set)>()(source);

		AQUA_TRY(ObjectDeserializer<decltype(set.bindings)>()(source), bindings);
		set.bindings = std::move(bindings.GetValue());
		return Expected<ShaderReflection::DescriptorSet, Error>(std::move(set));
	}
};

template <>
struct aqua::Serializer::ObjectDeserializer<aqua::ShaderReflection> {
public:
	using CanFail = std::true_type;

	aqua::Expected<aqua::ShaderReflection, aqua::Error> operator()(const char* source) const noexcept {
		aqua::ShaderReflection reflection{};
		{
			AQUA_TRY(ObjectDeserializer<decltype(reflection.entryPointName)>()(source), entryPointName);
			reflection.entryPointName = std::move(entryPointName.GetValue());
		}
		{
			AQUA_TRY(ObjectDeserializer<decltype(reflection.vertexLayouts)>()(source), vertexLayouts);
			reflection.vertexLayouts = std::move(vertexLayouts.GetValue());
		}
		{
			AQUA_TRY(ObjectDeserializer<decltype(reflection.descriptorSets)>()(source), descriptorSets);
			reflection.descriptorSets = std::move(descriptorSets.GetValue());
		}
		{
			AQUA_TRY(ObjectDeserializer<decltype(reflection.pushConstants)>()(source), pushConstants);
			reflection.pushConstants = std::move(pushConstants.GetValue());
		}
		reflection.incomplete = false;
		return Expected<ShaderReflection, Error>(std::move(reflection));
	}
};

aqua::Expected<aqua::ShaderReflection, aqua::Error> aqua::DeserializeShaderReflection(const char* path) noexcept {
	AQUA_TRY(aqua::System::Get().ReadFile(path), source);

	Serializer serializer;
	return std::move(serializer.Deserialize<ShaderReflection>(source.GetValue().GetPtr()));
}