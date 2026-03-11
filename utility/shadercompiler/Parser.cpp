#include <shadercompiler/Parser.h>

#include <map>

using namespace parser;

namespace {
	const char* CMD_TO_STRING[(int)Command::CMD_COUNT] = {
		"exit",
		"clear",
		"help",
		"indir",
		"outdir",
		"compile"
	};
	const char* CMD_TO_DESC[(int)Command::CMD_COUNT] = {
		"exit program",
		"clear terminal",
		"info about program",
		"source shader files directory",
		"compiled shader files output directory",
		"compile specified shaders"
	};
	const char* FLAG_TO_STRING[(int)Flag::FLAG_COUNT] = {
		"-v",
		"-lang"
	};
	const char* FLAG_TO_DESC[(int)Flag::FLAG_COUNT] = {
		"get value",
		"shaders source language"
	};

	const std::map<std::string, parser::CommandReq::MapType> COMMANDS = {
		{ "exit", parser::CommandReq::MapType {
			.cmd		  = Command::EXIT,
			.enabledFlags = std::set<Flag>(),
			.paramTypes   = std::vector<ParamType>()
		}},
		{ "clear", parser::CommandReq::MapType {
			.cmd		  = Command::CLEAR,
			.enabledFlags = std::set<Flag>(),
			.paramTypes   = std::vector<ParamType>()
		}},
		{ "help", parser::CommandReq::MapType {
			.cmd		  = Command::HELP,
			.enabledFlags = std::set<Flag>(),
			.paramTypes   = std::vector<ParamType>()
		}},
		{ "indir", parser::CommandReq::MapType {
			.cmd		  = Command::INDIR,
			.enabledFlags = std::set<Flag>({ Flag::VALUE }),
			.paramTypes   = std::vector<ParamType>({ ParamType::STRING })
		}},
		{ "outdir", parser::CommandReq::MapType {
			.cmd		  = Command::OUTDIR,
			.enabledFlags = std::set<Flag>({ Flag::VALUE }),
			.paramTypes   = std::vector<ParamType>({ ParamType::STRING })
		}},
		{ "compile", parser::CommandReq::MapType {
			.cmd		  = Command::COMPILE,
			.states		  = CommandReq::State::PARAM_INFINITE_BIT,
			.enabledFlags = std::set<Flag>({ Flag::SHADER_LANG }),
			.paramTypes   = std::vector<ParamType>({ ParamType::STRING })
		}},
	};

	const std::map<std::string, FlagReq::MapType> FLAGS = {
		{ "-v", FlagReq::MapType {
			.flag		= Flag::VALUE,
			.states		= FlagReq::State::NOT_REQUIRE_CMD_PARAMS_BIT,
			.paramTypes = std::vector<ParamType>()
		}},
		{ "-lang", FlagReq::MapType {
			.flag		= Flag::SHADER_LANG,
			.states		= FlagReq::EXECUTE_ORDER_DOES_NOT_MATTER_BIT,
			.paramTypes = std::vector<ParamType>({ ParamType::STRING })
		}}
	};
}

parser::CommandParser::ParseResult parser::CommandParser::Parse(const std::string& cmdLine) {
	ParseResult res{};
	std::vector<std::string> tokens;

	size_t cmdLen = cmdLine.length();
	size_t i = 0;

	while (i < cmdLen) {
		if (isspace(cmdLine[i])) {
			++i;
			continue;
		}
		std::string token;

		size_t j = i;
		while (j < cmdLen && !isspace(cmdLine[j])) ++j;
		token = cmdLine.substr(i, j - i);
		tokens.emplace_back(token);

		i = j;
	}
	std::string& expectedCmd = tokens.front();
	auto cmdIt = COMMANDS.find(expectedCmd);

	if (cmdIt != COMMANDS.cend()) {
		res.cmdInfo.req.cmd			 = cmdIt->second.cmd;
		res.cmdInfo.req.states		 = cmdIt->second.states;
		res.cmdInfo.req.enabledFlags = &cmdIt->second.enabledFlags;
		res.cmdInfo.req.paramTypes	 = &cmdIt->second.paramTypes;
	}
	else {
		res.errorInfo = std::string("Unknown command '") + expectedCmd + "'! Type 'help' to see command list";
		return res;
	}

	i = 1;
	while (i < tokens.size()) {
		std::string& token = tokens[i];
		if (token[0] == '-' && 1 < token.size() && token[1] != '-') {
			auto flagIt = FLAGS.find(token);
			if (flagIt == FLAGS.cend()) {
				res.errorInfo = std::string("Unknown flag '") + token + "'! Type 'help' to see flag list";
				return res;
			}
			FlagInfo flagInfo{};
			flagInfo.req.flag		= flagIt->second.flag;
			flagInfo.req.states		= flagIt->second.states;
			flagInfo.req.paramTypes = &flagIt->second.paramTypes;

			if (!res.cmdInfo.params.empty() && !(flagInfo.req.states & FlagReq::EXECUTE_ORDER_DOES_NOT_MATTER_BIT)) {
				flagInfo.req.states |= FlagReq::State::EXECUTE_AFTER_CMD_BIT;
			}

			++i;
			size_t flagParamCount = flagInfo.req.paramTypes->size();
			for (size_t j = 0; i < tokens.size() && j < flagParamCount; ++i, ++j) {
				flagInfo.params.emplace_back(std::move(tokens[i]));
			}
			if (flagParamCount != flagInfo.params.size()) {
				res.errorInfo = "Incorrect parameter count for flag '" + flagIt->first + '\'';
				return res;
			}
			res.cmdInfo.flags.emplace_back(std::move(flagInfo));
		}
		else {
			if (!res.cmdInfo.params.empty()) {
				res.errorInfo = std::string("Second parameter list for command '") +
					cmdIt->first + "'! Type 'help' to see command template";
				return res;
			}
			while (i < tokens.size() && tokens[i][0] != '-') {
				res.cmdInfo.params.emplace_back(std::move(tokens[i++]));
			}
		}
	}
	res.success = true;
	return res;
}

CommandParser::AnalyzeResult CommandParser::Analyze(const ParseResult& parserResult) const {
	size_t cmdReqParamCount = parserResult.cmdInfo.req.paramTypes->size();
	size_t cmdParamCount = parserResult.cmdInfo.params.size();

	auto flagIt = std::find_if(
		parserResult.cmdInfo.flags.cbegin(),
		parserResult.cmdInfo.flags.cend(),
		[](const FlagInfo& v) -> bool {
			return v.req.states & v.req.NOT_REQUIRE_CMD_PARAMS_BIT;
		}
	);
	if (flagIt == parserResult.cmdInfo.flags.cend() &&
		!(parserResult.cmdInfo.req.states & CommandReq::PARAM_INFINITE_BIT) &&
		cmdParamCount != cmdReqParamCount) {
		return CommandParser::AnalyzeResult {
			.success = false,
			.errorInfo = std::string("Invalid parameter count for command '") +
				CMD_TO_STRING[(int)parserResult.cmdInfo.req.cmd] + '\''
		};
	}
	if (flagIt == parserResult.cmdInfo.flags.cend() &&
		(parserResult.cmdInfo.req.states & CommandReq::PARAM_INFINITE_BIT) &&
		cmdParamCount < cmdReqParamCount) {
		return CommandParser::AnalyzeResult {
			.success = false,
			.errorInfo = std::string("Invalid parameter count for command '") +
				CMD_TO_STRING[(int)parserResult.cmdInfo.req.cmd] + '\''
		};
	}
	for (const FlagInfo& flagInfo : parserResult.cmdInfo.flags) {
		auto flagIt = parserResult.cmdInfo.req.enabledFlags->find(flagInfo.req.flag);
		if (flagIt == parserResult.cmdInfo.req.enabledFlags->cend()) {
			return CommandParser::AnalyzeResult {
				.success = false,
				.errorInfo = std::string("Flag '") + FLAG_TO_STRING[(int)flagInfo.req.flag] +
					"' is incompatible with command '" + CMD_TO_STRING[(int)parserResult.cmdInfo.req.cmd] + '\''
			};
		}
		size_t flagReqParamCount = flagInfo.req.paramTypes->size();
		size_t flagParamCount = flagInfo.params.size();

		if (flagReqParamCount != flagParamCount) {
			return CommandParser::AnalyzeResult{
				.success = false,
				.errorInfo = std::string("Invalid parameter count for flag '") +
					FLAG_TO_STRING[(int)flagInfo.req.flag] + '\''
			};
		}
	}
	return CommandParser::AnalyzeResult{ .success = true, .errorInfo = {} };
}

const char* parser::GetCommandName(Command cmd) noexcept { return CMD_TO_STRING[(int)cmd]; }
const char* parser::GetCommandDesc(Command cmd) noexcept { return CMD_TO_DESC[(int)cmd]; }
const char* parser::GetFlagName(Flag flag)		noexcept { return FLAG_TO_STRING[(int)flag]; }
const char* parser::GetFlagDesc(Flag flag)		noexcept { return FLAG_TO_DESC[(int)flag]; }