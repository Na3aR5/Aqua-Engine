#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <filesystem>
#include <thread>

namespace {
	static constexpr int VERSION_MAJOR = 1;
	static constexpr int VERSION_MINOR = 0;
	static constexpr int VERSION_PATCH = 0;
}

namespace {
	void ShowInputRow();
}

// Command and flag parameter type
enum class ParamType {
	STRING
};

// Available commands
enum class Command {
	EXIT,
	CLEAR,
	HELP,
	INDIR,
	OUTDIR,

	CMD_COUNT
};

// Available flags
enum class Flag {
	VALUE,

	FLAG_COUNT
};

// Specific flag requirements
struct FlagReq {
	struct MapType {
		Flag						 flag;
		uint32_t                     states = 0;
		const std::vector<ParamType> paramTypes;
	};

	enum State {
		EXECUTE_AFTER_CMD_BIT	   = 0x1,
		NOT_REQUIRE_CMD_PARAMS_BIT = 0x2,
	};

	Flag						  flag;
	uint32_t                      states = 0;
	const std::vector<ParamType>* paramTypes;
};

// Parsed flag info
struct FlagInfo {
	FlagReq					 req;
	std::vector<std::string> params;
};

// Specific command requirements
struct CommandReq {
	struct MapType {
		Command						 cmd;
		const std::set<Flag>		 enabledFlags;
		const std::vector<ParamType> paramTypes;
	};

	Command						  cmd;
	const std::set<Flag>*		  enabledFlags;
	const std::vector<ParamType>* paramTypes;
};

// Parsed command info
struct CommandInfo {
	CommandReq			     req;
	std::vector<FlagInfo>    flags;
	std::vector<std::string> params;
};

namespace {
	const char* CMD_TO_STRING[(int)Command::CMD_COUNT] = {
		"exit",
		"clear",
		"help",
		"indir",
		"outdir"
	};
	const char* CMD_TO_DESC[(int)Command::CMD_COUNT] = {
		"exit program",
		"clear terminal",
		"info about program",
		"source shader files directory",
		"compiled shader files output directory"
	};
	const char* FLAG_TO_STRING[(int)Flag::FLAG_COUNT] = {
		"-v"
	};
	const char* FLAG_TO_DESC[(int)Flag::FLAG_COUNT] = {
		"get value"
	};

	const std::map<std::string, CommandReq::MapType> COMMANDS = {
		{ "exit", CommandReq::MapType {
			.cmd		  = Command::EXIT,
			.enabledFlags = std::set<Flag>(),
			.paramTypes   = std::vector<ParamType>()
		}},
		{ "clear", CommandReq::MapType {
			.cmd = Command::CLEAR,
			.enabledFlags = std::set<Flag>(),
			.paramTypes = std::vector<ParamType>()
		}},
		{ "help", CommandReq::MapType {
			.cmd		  = Command::HELP,
			.enabledFlags = std::set<Flag>(),
			.paramTypes   = std::vector<ParamType>()
		}},
		{ "indir", CommandReq::MapType {
			.cmd		  = Command::INDIR,
			.enabledFlags = std::set<Flag>({ Flag::VALUE }),
			.paramTypes   = std::vector<ParamType>({ ParamType::STRING })
		}},
		{ "outdir", CommandReq::MapType {
			.cmd		  = Command::OUTDIR,
			.enabledFlags = std::set<Flag>({ Flag::VALUE }),
			.paramTypes   = std::vector<ParamType>({ ParamType::STRING })
		}}
	};

	const std::map<std::string, FlagReq::MapType> FLAGS = {
		{ "-v", FlagReq::MapType {
			.flag       = Flag::VALUE,
			.states     = FlagReq::State::NOT_REQUIRE_CMD_PARAMS_BIT,  
			.paramTypes = std::vector<ParamType>()
		}}
	};

	std::filesystem::path g_OutputDirectory = std::filesystem::current_path();
	std::filesystem::path g_InputDirectory  = std::filesystem::current_path();
}

class CommandParser {
public:
	enum Token {
		CMD,
		FLAG,
		PARAM
	};

	struct ParseResult {
		bool	    success = false;
		std::string errorInfo;
		CommandInfo cmdInfo;
	};

public:
	ParseResult Parse(const std::string& cmdLine) const {
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
			res.cmdInfo.req.enabledFlags = &cmdIt->second.enabledFlags;
			res.cmdInfo.req.paramTypes   = &cmdIt->second.paramTypes;
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

				if (!res.cmdInfo.params.empty()) {
					flagInfo.req.states |= FlagReq::State::EXECUTE_AFTER_CMD_BIT;
				}

				++i;
				size_t flagParamCount = flagInfo.req.paramTypes->size();
				for (size_t j = 0; i < tokens.size() && j < flagParamCount; ++i) {
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

	std::pair<bool, std::string> Analyze(const ParseResult& parserResult) const {
		size_t cmdReqParamCount = parserResult.cmdInfo.req.paramTypes->size();
		size_t cmdParamCount = parserResult.cmdInfo.params.size();

		auto flagIt = std::find_if(
			parserResult.cmdInfo.flags.cbegin(),
			parserResult.cmdInfo.flags.cend(),
			[](const FlagInfo& v) -> bool {
				return v.req.states & v.req.NOT_REQUIRE_CMD_PARAMS_BIT;
			}
		);
		if (flagIt == parserResult.cmdInfo.flags.cend() && cmdReqParamCount != cmdParamCount) {
			return std::pair<bool, std::string>(false,
				std::string("Invalid parameter count for command '") + CMD_TO_STRING[(int)parserResult.cmdInfo.req.cmd] + '\''
			);
		}
		for (const FlagInfo& flagInfo : parserResult.cmdInfo.flags) {
			auto flagIt = parserResult.cmdInfo.req.enabledFlags->find(flagInfo.req.flag);
			if (flagIt == parserResult.cmdInfo.req.enabledFlags->cend()) {
				return std::pair<bool, std::string>(false,
					std::string("Flag '") + FLAG_TO_STRING[(int)flagInfo.req.flag] + "' is incompatible with command '" +
						CMD_TO_STRING[(int)parserResult.cmdInfo.req.cmd] + '\''
				);
			}
			size_t flagReqParamCount = flagInfo.req.paramTypes->size();
			size_t flagParamCount = flagInfo.params.size();

			if (flagReqParamCount != flagParamCount) {
				return std::pair<bool, std::string>(false,
					std::string("Invalid parameter count for flag '") + FLAG_TO_STRING[(int)flagInfo.req.flag] + '\''
				);
			}
		}
		return std::pair<bool, std::string>(true, {});
	}
};

class Compiler {
public:

};

int main() {
	void (*executeFunctions[(int)Command::CMD_COUNT])(const CommandParser::ParseResult&) = {
		[](const CommandParser::ParseResult& parseResult) { // exit
			std::exit(0);
		},
		[](const CommandParser::ParseResult& parseResult) {
			std::cout << "\033[2J\033[H";
		},
		[](const CommandParser::ParseResult& parseResult) { // help
			size_t i = 0;

			std::cout << "Command list:\n";
			for (size_t i = 0; i < (size_t)Command::CMD_COUNT; ++i) {
				std::cout << "> " << CMD_TO_STRING[i] << " - " << CMD_TO_DESC[i] << '\n';
			}
			std::cout << '\n';

			std::cout << "Flag list:\n";
			for (i = 0; i < (size_t)Flag::FLAG_COUNT; ++i) {
				std::cout << "> " << FLAG_TO_STRING[i] << " - " << FLAG_TO_DESC[i] << '\n';
			}
			std::cout << '\n';
		},
		[](const CommandParser::ParseResult& parseResult) { // indir

		},
		[](const CommandParser::ParseResult& parseResult) { // outdir
			for (const FlagInfo& flag : parseResult.cmdInfo.flags) {
				if (!(flag.req.states & FlagReq::State::EXECUTE_AFTER_CMD_BIT)) {
					switch (flag.req.flag) {
						case Flag::VALUE:
							std::cout << "Current output directory is " << g_OutputDirectory << '\n';
							break;
					}
				}
			}
			if (!parseResult.cmdInfo.params.empty()) {
				const std::string& path = parseResult.cmdInfo.params[0];
				if (!std::filesystem::exists(path)) {
					std::cout << "Path " << path << " does not exist in file system\n";
					return;
				}
				g_OutputDirectory = std::filesystem::absolute(path);
				std::cout << "Output directory has been set\n";
			}
			for (const FlagInfo& flag : parseResult.cmdInfo.flags) {
				if (flag.req.states & FlagReq::State::EXECUTE_AFTER_CMD_BIT) {
					switch (flag.req.flag) {
						case Flag::VALUE:
							std::cout << "Current output directory is " << g_OutputDirectory << '\n';
							break;
					}
				}
			}
		}
	};

	CommandParser parser;
	std::string cmdLine;

	ShowInputRow();
	while (std::getline(std::cin, cmdLine)) {
		if (cmdLine.empty()) {
			ShowInputRow();
			continue;
		}
		CommandParser::ParseResult parseResult = parser.Parse(cmdLine);

		if (!parseResult.success) {
			std::cout << parseResult.errorInfo << '\n';
			ShowInputRow();
			continue;
		}
		std::pair<bool, std::string> analyzeResult = parser.Analyze(parseResult);

		if (!analyzeResult.first) {
			std::cout << analyzeResult.second << '\n';
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
}