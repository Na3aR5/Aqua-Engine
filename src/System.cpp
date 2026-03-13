#include <aqua/pch.h>
#include <aqua/System.h>
#include <aqua/Platform.h>
#include <aqua/Assert.h>
#include <aqua/Logger.h>

#include <fstream>

#if AQUA_PLATFORM_WINDOWS
#include <Windows.h>

namespace {
	class ChildProcessImpl : public aqua::System::IChildProcess {
	public:
		ChildProcessImpl() noexcept = default;
		~ChildProcessImpl() { Destroy(); }

	public:
		virtual aqua::Status Create(const aqua::System::ChildProcessCreateInfo& info) noexcept override {
			SECURITY_ATTRIBUTES securityAttributes{};
			securityAttributes.nLength = sizeof(securityAttributes);
			securityAttributes.bInheritHandle = TRUE;

			if (!CreatePipe(&m_readHandle, &m_writeHandle, &securityAttributes, 0)) {
				return aqua::Error::FAILED_TO_CREATE_CHILD_PROCESS;
			}

			STARTUPINFO startupInfo{};
			startupInfo.cb = sizeof(startupInfo);
			startupInfo.dwFlags = STARTF_USESTDHANDLES;
			startupInfo.hStdInput = m_readHandle;

			PROCESS_INFORMATION processInfo{};
			BOOL result = CreateProcessA(info.executablePath, NULL, NULL, NULL, TRUE,
				CREATE_NEW_CONSOLE, NULL, NULL, &startupInfo, &processInfo);

			if (!result) {
				CloseHandle(m_readHandle);
				CloseHandle(m_writeHandle);
				m_readHandle = m_writeHandle = NULL;
				return aqua::Error::FAILED_TO_CREATE_CHILD_PROCESS;
			}
			if (!(info.flags & info.ENABLE_READ_BIT)) {
				CloseHandle(m_readHandle);
				m_readHandle = NULL;
			}
			if (!(info.flags & info.ENABLE_WRITE_BIT)) {
				CloseHandle(m_writeHandle);
				m_writeHandle = NULL;
			}
			m_closeJob = CreateJobObject(NULL, NULL);

			auto terminateProcessFunction = [this, &processInfo]() {
				TerminateProcess(processInfo.hProcess, 1);
				CloseHandle(processInfo.hThread);
				CloseHandle(processInfo.hProcess);
				if (m_readHandle) {
					CloseHandle(m_readHandle);
					m_readHandle = NULL;
				}
				if (m_writeHandle) {
					CloseHandle(m_writeHandle);
					m_writeHandle = NULL;
				}
				if (m_closeJob) {
					CloseHandle(m_closeJob);
					m_closeJob = NULL;
				}
			};

			if (m_closeJob == NULL) {
				terminateProcessFunction();
				return aqua::Error::FAILED_TO_CREATE_CHILD_PROCESS;
			}
			JOBOBJECT_EXTENDED_LIMIT_INFORMATION jobInfo{};
			jobInfo.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;

			if (!SetInformationJobObject(m_closeJob, JobObjectExtendedLimitInformation, &jobInfo, sizeof(jobInfo))) {
				terminateProcessFunction();
				return aqua::Error::FAILED_TO_CREATE_CHILD_PROCESS;
			}
			if (!AssignProcessToJobObject(m_closeJob, processInfo.hProcess)) {
				terminateProcessFunction();
				return aqua::Error::FAILED_TO_CREATE_CHILD_PROCESS;
			}
			m_processHandle = processInfo.hProcess;
			m_threadHandle  = processInfo.hThread;

			return aqua::Success{};
		}

		virtual void Destroy() noexcept override {
			if (m_processHandle) {
				WaitForSingleObject(m_processHandle, INFINITE);
				CloseHandle(m_processHandle);
				m_processHandle = NULL;
			}
			if (m_threadHandle) {
				CloseHandle(m_threadHandle);
				m_threadHandle = NULL;
			}
			if (m_readHandle) {
				CloseHandle(m_readHandle);
				m_readHandle = NULL;
			}
			if (m_writeHandle) {
				CloseHandle(m_writeHandle);
				m_writeHandle = NULL;
			}
			if (m_closeJob) {
				CloseHandle(m_closeJob);
				m_closeJob = NULL;
			}
		}

		aqua::Status Write(const char* buffer, size_t size) noexcept override {
			if (m_writeHandle == NULL) {
				return aqua::Error::ACCESS_DENIED;
			}
			DWORD written;
			WriteFile(m_writeHandle, buffer, (DWORD)size, &written, NULL);
			if ((size_t)written != size) {
				return aqua::Error::FAILED_TO_WRITE;
			}
			return aqua::Success{};
		}

	private:
		HANDLE m_writeHandle   = NULL;
		HANDLE m_readHandle    = NULL;
		HANDLE m_processHandle = NULL;
		HANDLE m_threadHandle  = NULL;
		HANDLE m_closeJob      = NULL;
	}; // class ChildProcessImpl
}
#endif // AQUA_PLATFORM_WINDOWS

namespace {
	aqua::System* g_System = nullptr;
}

aqua::System::System(Status& status) {
	AQUA_ASSERT(g_System == nullptr, Literal("Attempt to create another System instance"));

	if (!status.IsSuccess() || g_System != nullptr) {
		return;
	}
	g_System = this;

	AQUA_LOG(Literal("OS System API is initialized"));
}

aqua::System::~System() {
	g_System = nullptr;
}

aqua::System& aqua::System::Get() noexcept { return *g_System; }
const aqua::System& aqua::System::GetConst() noexcept { return *g_System; }

aqua::Expected<aqua::SafeString, aqua::Error> aqua::System::ReadFile(const char* path) noexcept {
	std::ifstream file(path, std::ios::binary);

	if (!file.is_open()) {
		return Error::FAILED_TO_OPEN_FILE;
	}
	file.seekg(0, std::ios::end);
	size_t size = file.tellg();
	file.seekg(0);

	SafeString buffer;
	AQUA_TRY(buffer.Resize(size), _);
	file.read(buffer.GetPtr(), size);

	if (!file) {
		file.close();
		return Error::FAILED_TO_READ;
	}
	file.close();

	return Expected<SafeString, Error>(std::move(buffer));
}

aqua::Expected<aqua::System::Handle<aqua::System::IChildProcess>, aqua::Error> aqua::System::_Create(
const ChildProcessCreateInfo& info) noexcept {
	AQUA_TRY((aqua::CreateUniqueData<ChildProcessImpl, AllocatorType<ChildProcessImpl>>()), process);
	AQUA_TRY(process.GetValue()->Create(info), _1);
	aqua::MemorySystem::Pointer<IChildProcess> processPtr = process.GetValue().GetPtr();
	AQUA_TRY(m_childProcessRegister.Emplace(std::move(process.GetValue())), registeredProcess);

	return Handle<IChildProcess>(registeredProcess.GetValue(), processPtr);
}