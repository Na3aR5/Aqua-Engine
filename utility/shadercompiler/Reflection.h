#ifndef AQUA_SHADER_COMPILER_REFLECTION_HEADER
#define AQUA_SHADER_COMPILER_REFLECTION_HEADER

#include <glslang/Include/Types.h>
#include <aqua/Reflection.h>
#include <filesystem>

namespace reflect {
	aqua::ShaderReflection MakeReflection(const uint32_t* spirvSource, size_t size, EShLanguage);
	std::string WriteShaderReflection(const std::filesystem::path&, const aqua::ShaderReflection&);
}

#endif // !AQUA_SHADER_COMPILER_REFLECTION_HEADER