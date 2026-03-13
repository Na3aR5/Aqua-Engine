#ifndef AQUA_SHADER_COMPILER_REFLECTION_HEADER
#define AQUA_SHADER_COMPILER_REFLECTION_HEADER

#include <aqua/Reflection.h>
#include <glslang/Public/ShaderLang.h>
#include <filesystem>

namespace reflect {
	aqua::ShaderReflection MakeReflection(glslang::TProgram&);
	std::string WriteShaderReflection(const std::filesystem::path&, const aqua::ShaderReflection&);
}

#endif // !AQUA_SHADER_COMPILER_REFLECTION_HEADER