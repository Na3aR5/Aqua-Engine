#include <shadercompiler/Compiler.h>

#include <glslang/Public/ResourceLimits.h>
#include <SPIRV/GlslangToSpv.h>

#include <fstream>
#include <iostream>

namespace {
	const TBuiltInResource* g_ShaderResourceLimits = GetDefaultResources();

	const std::map<std::string, compiler::Compiler::Lang> SHADER_LANGS = {
		{ "glsl", compiler::Compiler::GLSL }
	};
}

namespace {
	std::pair<bool, std::string> ReadFile(const std::filesystem::path& path) {
		std::ifstream file(path, std::ios::binary);
		if (!file.is_open()) {
			return std::pair<bool, std::string>(false, {});
		}
		file.seekg(0, std::ios::end);
		size_t size = file.tellg();
		file.seekg(0);

		std::string buffer(size, '\0');
		file.read(buffer.data(), size);

		if (!file) {
			return std::pair<bool, std::string>(false, {});
		}
		file.close();

		return std::pair<bool, std::string>(true, std::move(buffer));
	}
}

compiler::Compiler::Compiler() {
	glslang::InitializeProcess();
}

compiler::Compiler::~Compiler() {
	glslang::FinalizeProcess();
}

const std::filesystem::path& compiler::Compiler::GetInputDirectory() const noexcept {
	return m_inputDirectory;
}

const std::filesystem::path& compiler::Compiler::GetOutputDirectory() const noexcept {
	return m_outputDirectory;
}

bool compiler::Compiler::SetInputDirectory(const std::string& path) {
	if (!std::filesystem::exists(path)) {
		return false;
	}
	m_inputDirectory = std::filesystem::absolute(path);
	return true;
}

bool compiler::Compiler::SetOutputDirectory(const std::string& path) {
	if (!std::filesystem::exists(path)) {
		return false;
	}
	m_outputDirectory = std::filesystem::absolute(path);
	return true;
}

std::string compiler::Compiler::Compile(Lang lang, const std::vector<std::string>& shaders) {
	for (const std::string& shader : shaders) {
		bool exist1 = std::filesystem::exists(shader);
		std::string inInputDir = (m_inputDirectory / shader).string();
		if (!exist1 && !std::filesystem::exists(inInputDir)) {
			return std::string("File ") + shader + " does not exist in filesystem";
		}
		const std::filesystem::path shaderSourceFile = exist1 ?
			std::filesystem::absolute(shader) : inInputDir;

		auto [readSuccess, source] = ReadFile(shaderSourceFile);
		if (!readSuccess) {
			return std::string("Failed to read ") + shaderSourceFile.string();
		}
		auto [stageErrorInfo, stage] = _DeduceShaderStage(shaderSourceFile.string());
		if (!stageErrorInfo.empty()) {
			return stageErrorInfo;
		}
		glslang::TShader compiledShader(stage);
		const char* sourceCStr = source.c_str();
		compiledShader.setStrings(&sourceCStr, 1);

		auto [versionSuccess, version] = _DeduceShaderVersion(source);
		if (!versionSuccess) {
			return std::string("Failed to deduce shader version");
		}
		if (!compiledShader.parse(g_ShaderResourceLimits, version, false, EShMsgDefault)) {
			return compiledShader.getInfoLog();
		}
		glslang::TProgram program;
		program.addShader(&compiledShader);

		if (!program.link(EShMsgDefault)) {
			return program.getInfoLog();
		}
		std::vector<uint32_t> spirv;
		glslang::GlslangToSpv(
			*program.getIntermediate(stage),
			spirv
		);
		_CreateSprivShaderFile(shaderSourceFile.string(), spirv);

		std::cout << " - Compiled " << shaderSourceFile << '\n';
	}
	return {};
}

std::pair<std::string, EShLanguage> compiler::Compiler::_DeduceShaderStage(const std::string& shader) const {
	auto shaderExtension = std::filesystem::path(shader).extension();
	if (shaderExtension.compare(".vert") == 0) {
		return std::pair<std::string, EShLanguage>({}, EShLanguage::EShLangVertex);
	}
	if (shaderExtension.compare(".frag") == 0) {
		return std::pair<std::string, EShLanguage>({}, EShLanguage::EShLangFragment);
	}
	return std::pair<std::string, EShLanguage>(
		std::string("Unknown shader file extension '" + shaderExtension.string() + '\''),
		EShLanguage::EShLangVertex
	);
}

std::pair<bool, int> compiler::Compiler::_DeduceShaderVersion(const std::string& source) const {
	size_t pos = source.find("#version");
	if (pos == std::string::npos) { // default
		return std::pair<bool, int>(true, 450);
	}
	pos += 8;
	while (pos < source.size() && std::isspace(source[pos])) ++pos;

	if (pos >= source.size()) {
		return std::pair<bool, int>(false, -1);
	}
	int version = -1;
	try {
		version = std::stoi(source.substr(pos, 20));
	}
	catch (...) {
		return std::pair<bool, int>(false, -1);
	}
	return std::pair<bool, int>(true, version);
}

bool compiler::Compiler::_CreateSprivShaderFile(const std::string& pathStr, const std::vector<uint32_t>& spirv) const {
	std::filesystem::path writePath = m_outputDirectory / ((std::filesystem::path)pathStr += ".spv");

	std::ofstream file(writePath, std::ios::binary);
	if (!file.is_open()) {
		return false;
	}
	file.write(
		reinterpret_cast<const char*>(spirv.data()),
		spirv.size() * sizeof(uint32_t)
	);
	if (!file) {
		return false;
	}
	return true;
}

const std::map<std::string, compiler::Compiler::Lang>& compiler::GetShaderLangMap() {
	return SHADER_LANGS;
}