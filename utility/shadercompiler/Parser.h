#ifndef AQUA_SHADER_COMPILER_PARSER_HEADER
#define AQUA_SHADER_COMPILER_PARSER_HEADER

#include <vector>
#include <string>
#include <set>

namespace parser {
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
		COMPILE,

		CMD_COUNT
	};

	// Available flags
	enum class Flag {
		VALUE,
		SHADER_LANG,

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
			EXECUTE_AFTER_CMD_BIT			  = 0x1,
			EXECUTE_ORDER_DOES_NOT_MATTER_BIT = 0x2,
			NOT_REQUIRE_CMD_PARAMS_BIT		  = 0x4
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
			uint32_t                     states = 0;
			const std::set<Flag>		 enabledFlags;
			const std::vector<ParamType> paramTypes;
		};

		enum State {
			PARAM_INFINITE_BIT = 0x1
		};

		Command						  cmd;
		uint32_t                      states = 0;
		const std::set<Flag>*		  enabledFlags;
		const std::vector<ParamType>* paramTypes;
	};

	// Parsed command info
	struct CommandInfo {
		CommandReq				 req;
		std::vector<FlagInfo>    flags;
		std::vector<std::string> params;
	};

	class CommandParser {
	public:
		struct ParseResult {
			bool	    success = false;
			std::string errorInfo;
			CommandInfo cmdInfo;
		};

		struct AnalyzeResult {
			bool		success = false;
			std::string errorInfo;
		};

	public:
		ParseResult Parse(const std::string& cmdLine);
		AnalyzeResult Analyze(const ParseResult& parseResult) const;
	};

	const char* GetCommandName(Command) noexcept;
	const char* GetCommandDesc(Command) noexcept;
	const char* GetFlagDesc(Flag) noexcept;
	const char* GetFlagName(Flag) noexcept;
}

#endif // !AQUA_SHADER_COMPILER_PARSER_HEADER