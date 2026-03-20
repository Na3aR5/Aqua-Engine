#include <shadercompiler/Reflection.h>
#include <glslang/Include/Types.h>

#include <map>
#include <fstream>

namespace {
	uint32_t ConvertStages(EShLanguageMask stages) noexcept {
		uint32_t convertedStages = 0;
		if (stages & EShLanguageMask::EShLangVertexMask) {
			convertedStages |= (uint32_t)aqua::ShaderReflection::Stage::VERTEX;
		}
		if (stages & EShLanguageMask::EShLangFragmentMask) {
			convertedStages |= (uint32_t)aqua::ShaderReflection::Stage::FRAGMENT;
		}
		return convertedStages;
	}
}

aqua::ShaderReflection reflect::MakeReflection(glslang::TProgram& program) {
	aqua::ShaderReflection reflection{};

	program.buildReflection();
	{
		using DescriptorBindingArrayType = decltype(reflection.descriptorSets[0].bindings);

		std::map<uint32_t, DescriptorBindingArrayType> uniqueDescriptorSets;
		int descriptorBlockCount = program.getNumUniformBlocks();

		aqua::Status status = reflection.descriptorSets.Reserve(descriptorBlockCount);
		if (!status.IsSuccess()) return reflection;

		status = reflection.pushConstants.Reserve(descriptorBlockCount);
		if (!status.IsSuccess()) return reflection;

		for (int i = 0; i < descriptorBlockCount; ++i) {
			const auto& block = program.getUniformBlock(i);
			if (block.getType()->getQualifier().layoutPushConstant) {
				aqua::ShaderReflection::PushConstant pushConstant{};
				pushConstant.offset = (uint32_t)block.offset;
				pushConstant.bytes  = (uint32_t)block.size;

				reflection.pushConstants.EmplaceBackUnchecked(pushConstant);
				continue;
			}
			uint32_t layoutSet = block.getType()->getQualifier().layoutSet;
			auto setIt = uniqueDescriptorSets.find(layoutSet);
			if (setIt == uniqueDescriptorSets.cend()) {
				size_t setIndex = reflection.descriptorSets.GetSize();
				auto [it, res] = uniqueDescriptorSets.emplace(layoutSet, DescriptorBindingArrayType());
				if (!res) {
					return reflection;
				}
				setIt = it;
			}
			aqua::ShaderReflection::DescriptorBinding binding{};
			binding.binding = (uint32_t)block.getBinding();
			binding.set     = layoutSet;
			binding.bytes   = (uint32_t)block.size;
			binding.stages  = ConvertStages(program.getUniformStages(i));

			if (block.getType()->getBasicType() == glslang::EbtBlock) {
				binding.type = aqua::ShaderReflection::UNIFORM_BUFFER;
			}
			uint32_t count = 1;
			if (block.getType()->isArray()) {
				count = block.getType()->getArraySizes()->getDimSize(0);
			}
			binding.count = count;
			if (!setIt->second.EmplaceBack(binding).IsSuccess()) {
				return reflection;
			}
		}
		for (auto it = uniqueDescriptorSets.begin(); it != uniqueDescriptorSets.end(); ++it) {
			reflection.descriptorSets.EmplaceBackUnchecked();
			reflection.descriptorSets.Last().set = it->first;
			reflection.descriptorSets.Last().bindings = std::move(it->second);
		}
	}
	{
		int vertexLayoutCount = program.getNumPipeInputs();
		aqua::Status status = reflection.vertexLayouts.Reserve(vertexLayoutCount);
		if (!status.IsSuccess()) {
			return reflection;
		}
		for (int i = 0; i < vertexLayoutCount; ++i) {
			const auto& input = program.getPipeInput(i);

			aqua::ShaderReflection::VertexLayout layout{};
			layout.location = input.layoutLocation();
			layout.bytes    = input.getType()->getVectorSize() *
				(input.getType()->getBasicType() == glslang::EbtDouble ? sizeof(double) : sizeof(float));

			reflection.vertexLayouts.EmplaceBackUnchecked(layout);
		}
	}
	reflection.incomplete = false;
	return reflection;
}

std::string reflect::WriteShaderReflection(const std::filesystem::path& path, const aqua::ShaderReflection& reflection) {
	std::ofstream file(path, std::ios::binary | std::ios::trunc);

	if (!file.is_open()) {
		return std::string("Failed to write file ") + path.string();
	}
	{
		uint32_t vertexLayoutCount = (uint32_t)reflection.vertexLayouts.GetSize();
		file.write((const char*)&vertexLayoutCount, sizeof(uint32_t));
		for (const aqua::ShaderReflection::VertexLayout layout : reflection.vertexLayouts) {
			file.write((const char*)&layout, sizeof(layout));
		}
	}
	{
		uint32_t descriptorSetCount = reflection.descriptorSets.GetSize();
		file.write((const char*)&descriptorSetCount, sizeof(uint32_t));
		for (const aqua::ShaderReflection::DescriptorSet& set : reflection.descriptorSets) {
			file.write((const char*)&set.set, sizeof(uint32_t));
			
			uint32_t bindingCount = (uint32_t)set.bindings.GetSize();
			file.write((const char*)&bindingCount, sizeof(uint32_t));
			for (const aqua::ShaderReflection::DescriptorBinding& binding : set.bindings) {
				file.write((const char*)&binding, sizeof(binding));
			}
		}
	}
	{
		uint32_t pushConstantCount = reflection.pushConstants.GetSize();
		file.write((const char*)&pushConstantCount, sizeof(uint32_t));
		for (const aqua::ShaderReflection::PushConstant& constant : reflection.pushConstants) {
			file.write((const char*)&constant, sizeof(constant));
		}
	}
	if (!file) {
		return std::string("Failed to write file ") + path.string();
	}
	file.close();

	return {};
}