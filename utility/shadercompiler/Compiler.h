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

	public:
		Compiler();
		~Compiler();

	public:
		const std::filesystem::path& GetInputDirectory() const noexcept;
		const std::filesystem::path& GetOutputDirectory() const noexcept;

		bool SetInputDirectory(const std::string& path);
		bool SetOutputDirectory(const std::string& path);

		std::string Compile(Lang lang, const std::vector<std::string>& shaders);

	private:
		std::pair<std::string, EShLanguage> _DeduceShaderStage(const std::string& shader) const;
		std::pair<bool, int> _DeduceShaderVersion(const std::string& source) const;
		bool _CreateSprivShaderFile(const std::string& path, const std::vector<uint32_t>& spirv) const;

	private:
		std::filesystem::path m_inputDirectory  = std::filesystem::current_path();
		std::filesystem::path m_outputDirectory = std::filesystem::current_path();
	};

	const std::map<std::string, Compiler::Lang>& GetShaderLangMap();
}

#endif // !AQUA_SHADER_COMPILER_COMPILER_HEADER