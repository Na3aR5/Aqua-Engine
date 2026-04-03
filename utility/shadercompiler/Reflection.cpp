#include <shadercompiler/Reflection.h>
#include <aqua/utility/Serializer.h>

#include <spirv_cross.hpp>

#include <map>
#include <fstream>

template <>
struct aqua::Serializer::NeedDefinedAlgorithm<aqua::ShaderReflection> {
	using Need = std::true_type;
};
template <>
struct aqua::Serializer::NeedDefinedAlgorithm<aqua::ShaderReflection::DescriptorSet> {
	using Need = std::true_type;
};

template <>
struct aqua::Serializer::ObjectSerializer<aqua::ShaderReflection::DescriptorSet> {
public:
	Status operator()(std::ofstream& fs, const aqua::ShaderReflection::DescriptorSet& set) const noexcept {
		AQUA_TRY(ObjectSerializer<decltype(set.set)>()(fs, set.set));
		AQUA_TRY(ObjectSerializer<decltype(set.bindings)>()(fs, set.bindings));

		return Success{};
	}
};

template <>
struct aqua::Serializer::ObjectSerializer<aqua::ShaderReflection> {
public:
	Status operator()(std::ofstream& fs, const aqua::ShaderReflection& reflection) const noexcept {
		AQUA_TRY(ObjectSerializer<decltype(reflection.entryPointName)>()(fs, reflection.entryPointName));
		AQUA_TRY(ObjectSerializer<decltype(reflection.vertexLayouts)>()(fs, reflection.vertexLayouts));
		AQUA_TRY(ObjectSerializer<decltype(reflection.descriptorSets)>()(fs, reflection.descriptorSets));
		AQUA_TRY(ObjectSerializer<decltype(reflection.pushConstants)>()(fs, reflection.pushConstants));

		return Success{};
	}
};

namespace {
	uint32_t ConvertStages(EShLanguageMask stages) noexcept {
		uint32_t convertedStages = 0;
		if (stages & EShLanguageMask::EShLangVertexMask) {
			convertedStages |= (uint32_t)aqua::ShaderStage::VERTEX_BIT;
		}
		if (stages & EShLanguageMask::EShLangFragmentMask) {
			convertedStages |= (uint32_t)aqua::ShaderStage::FRAGMENT_BIT;
		}
		return convertedStages;
	}

	uint32_t ConvertStage(EShLanguage stage) noexcept {
		switch (stage) {
			case EShLanguage::EShLangVertex:
				return (uint32_t)aqua::ShaderStage::VERTEX_BIT;

			case EShLanguage::EShLangFragment:
				return (uint32_t)aqua::ShaderStage::FRAGMENT_BIT;

			default:
				break;
		}
		return 0;
	}
}

aqua::ShaderReflection reflect::MakeReflection(const uint32_t* spirvSource, size_t size, EShLanguage stage) {
	aqua::ShaderReflection reflection{};

	spirv_cross::Compiler compiler(spirvSource, size);
	spirv_cross::ShaderResources resources = compiler.get_shader_resources();

	auto entryPoints = compiler.get_entry_points_and_stages();
	if (!reflection.entryPointName.Set(entryPoints[0].name.c_str()).IsSuccess()) {
		return reflection;
	}

	if (!reflection.descriptorSets.Reserve(resources.uniform_buffers.size()).IsSuccess()) {
		return reflection;
	}
	{
		using DescriptorBindingArrayType = decltype(reflection.descriptorSets[0].bindings);
		std::map<uint32_t, DescriptorBindingArrayType> uniqueDescriptorSets;

		for (const auto& uniformBuffer : resources.uniform_buffers) {
			const auto& type = compiler.get_type(uniformBuffer.base_type_id);

			aqua::ShaderReflection::DescriptorBinding binding{};
			binding.binding = compiler.get_decoration(uniformBuffer.id, spv::DecorationBinding);
			binding.set     = compiler.get_decoration(uniformBuffer.id, spv::DecorationDescriptorSet);
			binding.bytes   = compiler.get_declared_struct_size(type);
			binding.type    = aqua::ShaderReflection::UNIFORM_BUFFER;
			binding.stages  = ConvertStage(stage);
			binding.count   = 1;

			if (!type.array.empty()) {
				binding.count = type.array.back();
			}
			auto setIt = uniqueDescriptorSets.find(binding.set);
			if (setIt == uniqueDescriptorSets.cend()) {
				setIt = uniqueDescriptorSets.emplace(binding.set, DescriptorBindingArrayType()).first;
			}
			if (!setIt->second.EmplaceBack(binding).IsSuccess()) {
				return reflection;
			}
		}
		if (!reflection.descriptorSets.Reserve(uniqueDescriptorSets.size()).IsSuccess()) {
			return reflection;
		}
		for (auto it = uniqueDescriptorSets.begin(); it != uniqueDescriptorSets.end(); ++it) {
			reflection.descriptorSets.EmplaceBackUnchecked();
			reflection.descriptorSets.Last().set = it->first;
			reflection.descriptorSets.Last().bindings = std::move(it->second);
		}
	}

	if (!reflection.pushConstants.Reserve(resources.push_constant_buffers.size()).IsSuccess()) {
		return reflection;
	}
	for (const auto& pushConstant : resources.push_constant_buffers) {
		const auto& type = compiler.get_type(pushConstant.base_type_id);

		aqua::ShaderReflection::PushConstant reflectionPushConstant{};
		reflectionPushConstant.offset = 0; // temporarily (whole range)
		reflectionPushConstant.bytes  = compiler.get_declared_struct_size(type);

		reflection.pushConstants.EmplaceBackUnchecked(reflectionPushConstant);
	}

	if (!reflection.vertexLayouts.Reserve(resources.stage_inputs.size()).IsSuccess()) {
		return reflection;
	}
	for (const auto& stageInput : resources.stage_inputs) {
		auto& type = compiler.get_type(stageInput.type_id);

		aqua::ShaderReflection::VertexLayout layout{};
		layout.location = compiler.get_decoration(stageInput.id, spv::DecorationLocation);
		layout.bytes    = (type.width / 8) * type.vecsize;

		reflection.vertexLayouts.EmplaceBackUnchecked(layout);
	}

	reflection.incomplete = false;
	return reflection;
}

std::string reflect::WriteShaderReflection(const std::filesystem::path& path, const aqua::ShaderReflection& reflection) {
	aqua::Serializer serializer;
	serializer.SetTargetFile(path.string().c_str());

	if (!serializer.Serialize(reflection).IsSuccess()) {
		return "Failed to write shader reflection";
	}
	return {};
}