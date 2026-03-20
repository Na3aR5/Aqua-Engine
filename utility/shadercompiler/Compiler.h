#ifndef AQUA_SHADER_COMPILER_COMPILER_HEADER
#define AQUA_SHADER_COMPILER_COMPILER_HEADER

#include <map>
#include <filesystem>
#include <glslang/Public/ShaderLang.h>

namespace compiler {
	class Compiler {
	public:
		enum Lang {
			GLSL
		};

		enum API {
			VULKAN
		};

	public:
		Compiler();
		~Compiler();

	public:
		const std::filesystem::path& GetInputDirectory() const noexcept;
		const std::filesystem::path& GetOutputDirectory() const noexcept;

		bool SetInputDirectory(const std::string& path);
		bool SetOutputDirectory(const std::string& path);

		std::string Compile(Lang lang, API api, const std::vector<std::string>& shaders);

	private:
		std::pair<std::string, EShLanguage> _DeduceShaderStage(const std::filesystem::path& shader) const;
		std::pair<bool, int> _DeduceShaderVersion(const std::string& source) const;
		bool _CreateSpirvShaderFile(const std::filesystem::path& path, const std::vector<uint32_t>& spirv) const;

	private:
		std::filesystem::path m_inputDirectory  = std::filesystem::current_path();
		std::filesystem::path m_outputDirectory = std::filesystem::current_path();
	};

	const std::map<std::string, Compiler::Lang>& GetShaderLangMap();
	const std::map<std::string, Compiler::API>& GetShaderAPIMap();
}

#endif // !AQUA_SHADER_COMPILER_COMPILER_HEADER