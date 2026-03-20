#include <shadercompiler/Parser.h>
#include <shadercompiler/Compiler.h>

#include <functional>
#include <iostream>
#include <thread>

namespace {
	static constexpr int VERSION_MAJOR = 1;
	static constexpr int VERSION_MINOR = 0;
	static constexpr int VERSION_PATCH = 0;
}

namespace {
	void ShowInputRow();
	void ShowPathNotExist(const std::string& path);
}

int main() {
	parser::CommandParser parser;
	compiler::Compiler    compiler;
	std::string			  cmdLine;

	std::function<void(const parser::CommandParser::ParseResult&)> executeFunctions[(int)parser::Command::CMD_COUNT] = {
		[](const parser::CommandParser::ParseResult& parseResult) { // exit
			std::exit(0);
		},
		[](const parser::CommandParser::ParseResult& parseResult) { // clear
			std::cout << "\033[2J\033[H";
		},
		[](const parser::CommandParser::ParseResult& parseResult) { // help
			size_t i = 0;

			std::cout << "Command list:\n";
			for (size_t i = 0; i < (size_t)parser::Command::CMD_COUNT; ++i) {
				std::cout << "> " << parser::GetCommandName((parser::Command)i) <<
					" - " << parser::GetCommandDesc((parser::Command)i) << '\n';
			}
			std::cout << '\n';

			std::cout << "Flag list:\n";
			for (i = 0; i < (size_t)parser::Flag::FLAG_COUNT; ++i) {
				std::cout << "> " << parser::GetFlagName((parser::Flag)i) <<
					" - " << parser::GetFlagDesc((parser::Flag)i) << '\n';
			}
			std::cout << '\n';
		},
		[&compiler](const parser::CommandParser::ParseResult& parseResult) { // indir
			for (const parser::FlagInfo& flag : parseResult.cmdInfo.flags) {
				if (!(flag.req.states & parser::FlagReq::State::EXECUTE_AFTER_CMD_BIT)) {
					switch (flag.req.flag) {
						case parser::Flag::VALUE:
							std::cout << "Current input directory is " << compiler.GetInputDirectory() << '\n';
							break;
					}
				}
			}
			if (!parseResult.cmdInfo.params.empty()) {
				const std::string& path = parseResult.cmdInfo.params[0];
				bool success = compiler.SetInputDirectory(path);
				if (!success) {
					ShowPathNotExist(path);
					return;
				}
				std::cout << "Input directory has been set\n";
			}
			for (const parser::FlagInfo& flag : parseResult.cmdInfo.flags) {
				if (flag.req.states & parser::FlagReq::State::EXECUTE_AFTER_CMD_BIT) {
					switch (flag.req.flag) {
					case parser::Flag::VALUE:
						std::cout << "Current input directory is " << compiler.GetInputDirectory() << '\n';
						break;
					}
				}
			}
		},
		[&compiler](const parser::CommandParser::ParseResult& parseResult) { // outdir
			for (const parser::FlagInfo& flag : parseResult.cmdInfo.flags) {
				if (!(flag.req.states & parser::FlagReq::State::EXECUTE_AFTER_CMD_BIT)) {
					switch (flag.req.flag) {
						case parser::Flag::VALUE:
							std::cout << "Current output directory is " << compiler.GetOutputDirectory() << '\n';
							break;
					}
				}
			}
			if (!parseResult.cmdInfo.params.empty()) {
				const std::string& path = parseResult.cmdInfo.params[0];
				bool success = compiler.SetOutputDirectory(path);
				if (!success) {
					ShowPathNotExist(path);
					return;
				}
				std::cout << "Output directory has been set\n";
			}
			for (const parser::FlagInfo& flag : parseResult.cmdInfo.flags) {
				if (flag.req.states & parser::FlagReq::State::EXECUTE_AFTER_CMD_BIT) {
					switch (flag.req.flag) {
						case parser::Flag::VALUE:
							std::cout << "Current output directory is " << compiler.GetOutputDirectory() << '\n';
							break;
					}
				}
			}
		},
		[&compiler](const parser::CommandParser::ParseResult& parseResult) { // compile
			compiler::Compiler::Lang lang = compiler::Compiler::GLSL;
			compiler::Compiler::API  api  = compiler::Compiler::VULKAN;

			auto executeFlag = [&](const parser::FlagInfo& flag) {
				switch (flag.req.flag) {
					case parser::Flag::SHADER_LANG: {
						auto langIt = compiler::GetShaderLangMap().find(flag.params[0]);
						if (langIt == compiler::GetShaderLangMap().cend()) {
							std::cout << "Unknown shader language '" << flag.params[0] << "'!\n";
							return;
						}
						lang = langIt->second;
						break;
					}
					case parser::Flag::API: {
						auto apiIt = compiler::GetShaderAPIMap().find(flag.params[0]);
						if (apiIt == compiler::GetShaderAPIMap().cend()) {
							std::cout << "Unknown graphics API '" << flag.params[0] << "'!\n";
							return;
						}
						api = apiIt->second;
						break;
					}
				}
			};
			for (const parser::FlagInfo& flag : parseResult.cmdInfo.flags) {
				if (!(flag.req.states & parser::FlagReq::State::EXECUTE_AFTER_CMD_BIT)) {
					executeFlag(flag);
				}
			}
			std::string errorInfo = compiler.Compile(lang, api, parseResult.cmdInfo.params);
			if (!errorInfo.empty()) {
				std::cout << errorInfo << '\n';
				return;
			}
			for (const parser::FlagInfo& flag : parseResult.cmdInfo.flags) {
				if (flag.req.states & parser::FlagReq::State::EXECUTE_AFTER_CMD_BIT) {
					executeFlag(flag);
				}
			}
		}
	};

	ShowInputRow();
	while (std::getline(std::cin, cmdLine)) {
		std::this_thread::sleep_for(std::chrono::milliseconds(1));

		if (cmdLine.empty()) {
			ShowInputRow();
			continue;
		}
		parser::CommandParser::ParseResult parseResult = parser.Parse(cmdLine);

		if (!parseResult.success) {
			std::cout << parseResult.errorInfo << '\n';
			ShowInputRow();
			continue;
		}
		parser::CommandParser::AnalyzeResult analyzeResult = parser.Analyze(parseResult);

		if (!analyzeResult.success) {
			std::cout << analyzeResult.errorInfo << '\n';
			ShowInputRow();
			continue;
		}
		executeFunctions[(int)parseResult.cmdInfo.req.cmd](parseResult);

		ShowInputRow();
	}
}

namespace {
	void ShowInputRow() {
		std::cout << "<AquaShaderCompiler v" << VERSION_MAJOR << '.' << VERSION_MINOR << '.' << VERSION_PATCH << "> ";
	}

	void ShowPathNotExist(const std::string& path) {
		std::cout << "Path '" << path << "' does not exist in filesystem\n";
	}
}