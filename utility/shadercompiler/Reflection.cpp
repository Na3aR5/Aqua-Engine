#include <shadercompiler/Reflection.h>
#include <glslang/Include/Types.h>
#include <fstream>

aqua::ShaderReflection reflect::MakeReflection(glslang::TProgram& program) {
	aqua::ShaderReflection reflection{};

	program.buildReflection();
	{
		int descriptorBlockCount = program.getNumUniformBlocks();

		aqua::Status status = reflection.descriptorBindings.Reserve(descriptorBlockCount);
		if (!status.IsSuccess()) { return reflection; }

		status = reflection.pushConstants.Reserve(descriptorBlockCount);
		if (!status.IsSuccess()) { return reflection; }

		for (int i = 0; i < descriptorBlockCount; ++i) {
			const auto& block = program.getUniformBlock(i);

			if (block.getType()->getQualifier().layoutPushConstant) {
				aqua::ShaderReflection::PushConstant pushConstant{};
				pushConstant.offset = (uint32_t)block.offset;
				pushConstant.bytes  = (uint32_t)block.size;

				reflection.pushConstants.EmplaceBackUnchecked(pushConstant);
				continue;
			}
			aqua::ShaderReflection::DescriptorBinding binding{};
			binding.binding = (uint32_t)block.getBinding();
			binding.set     = block.getType()->getQualifier().layoutSet;
			binding.bytes   = (uint32_t)block.size;

			if (block.getType()->getQualifier().storage == glslang::EvqBuffer) {
				binding.type = aqua::ShaderReflection::UNIFORM_BUFFER;
			}
			reflection.descriptorBindings.EmplaceBackUnchecked(binding);
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
		size_t vertexLayoutCount = reflection.vertexLayouts.GetSize();
		file.write((char*)&vertexLayoutCount, sizeof(size_t));
		for (const aqua::ShaderReflection::VertexLayout layout : reflection.vertexLayouts) {
			file.write((const char*)&layout, sizeof(layout));
		}
	}
	{
		size_t descriptorBindingCount = reflection.descriptorBindings.GetSize();
		file.write((char*)&descriptorBindingCount, sizeof(size_t));
		for (const aqua::ShaderReflection::DescriptorBinding& binding : reflection.descriptorBindings) {
			file.write((const char*)&binding, sizeof(binding));
		}
	}
	{
		size_t pushConstantCount = reflection.pushConstants.GetSize();
		file.write((char*)&pushConstantCount, sizeof(size_t));
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