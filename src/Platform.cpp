#include "Platform.h"

#include <cstdlib>

#include "flog.h"

//#define WIN32_LEAN_AND_MEAN
//#define WIN32_EXTRA_LEAN
#include <windows.h>

class Win32Process : public Process {
	public:
	PROCESS_INFORMATION proc;

	Win32Process(PROCESS_INFORMATION h){
		this->proc = h;
	}

	bool IsRunning(){
		return WaitForSingleObject(proc.hProcess, 0) == WAIT_TIMEOUT;
	}
	
	bool Wait(int msTimeout)
	{
		return WaitForSingleObject(proc.hProcess, msTimeout) != WAIT_TIMEOUT;
	}
	
	void Kill()
	{
		TerminateProcess(proc.hProcess, 100);
	}
	
	void InjectDll(const std::string& path, int timeout)
	{
		LPVOID pszDllPathRemote = VirtualAllocEx(proc.hProcess, NULL, path.size(), MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

		WriteProcessMemory(proc.hProcess, pszDllPathRemote, (LPVOID)path.c_str(), path.size(), NULL);

		HANDLE hThread = CreateRemoteThread(proc.hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)
				GetProcAddress(GetModuleHandle("Kernel32"), "LoadLibraryA"), pszDllPathRemote, 0, NULL);

		DWORD ret = WaitForSingleObject(hThread, timeout);
		if(ret != 0)
			throw PlatformEx(Str("failed to wait for remote thread when injecting dll: " << path << ", error: " << ret));

		HMODULE hLibModule;
		GetExitCodeThread(hThread, (DWORD*)&hLibModule);
		CloseHandle(hThread);

		VirtualFreeEx(proc.hProcess, pszDllPathRemote, path.size(), MEM_RELEASE);

		if(!hLibModule)
			throw PlatformEx(Str("could not inject dll: " << path));
	}

	void Resume()
	{
		ResumeThread(proc.hThread);
	}
	
	int GetExitCode()
	{
		DWORD ret;

		if(!GetExitCodeProcess(proc.hProcess, &ret))
			throw PlatformEx(Str("failed to get process exit code"));

		return ret;
	}
};

class Win32Platform : public Platform {
	public:
	HANDLE jobHandle;

	Win32Platform(){
		// SetErrorMode to never displaying any popup windows. Should propagate to child processes.
		SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX | SEM_NOOPENFILEERRORBOX);

		// Create a job object with limits that kill the subprocesses on exit
		jobHandle = CreateJobObject(NULL, NULL);
		AssertEx(jobHandle != 0, PlatformEx, "(CreateJobObject) win32 error: " << GetErrorStr(GetLastError()) << " (" << GetLastError() << ")");
			
		JOBOBJECT_EXTENDED_LIMIT_INFORMATION eInfo;
		memset(&eInfo, 0, sizeof(JOBOBJECT_EXTENDED_LIMIT_INFORMATION));
		eInfo.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;

		int err = SetInformationJobObject(jobHandle, JobObjectExtendedLimitInformation, &eInfo, sizeof(JOBOBJECT_EXTENDED_LIMIT_INFORMATION));
		AssertEx(err != 0, PlatformEx, "(SetInformationJobObject) win32 error: " << GetErrorStr(GetLastError()) << " (" << GetLastError() << ")");
	}

	~Win32Platform(){
		CloseHandle(jobHandle);
	}

	std::string GetErrorStr(DWORD nErrorCode)
	{
		char* msg;
		std::string ret = "Unknown";

		int nChar = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, 
			nErrorCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&msg, 0, NULL);

		if (nChar > 0){
			ret.assign(msg);
			free(msg);
		}

		return ret;
	}

	void Sleep(int ms){
		::Sleep(ms);
	}

	std::string CombinePath(const StrVec& paths){
		return Tools::CombinePath(paths, '\\');
	}
	
	std::string GetWorkingDirectory() {
		char path[PATH_MAX];		
		GetModuleFileName(GetModuleHandle(NULL), path, sizeof(path));

		char drive[_MAX_DRIVE];
		char dir[_MAX_DIR];

		_splitpath(path, drive, dir, NULL, NULL);

		return CombinePath({drive, dir});
	}

	ProcessPtr StartProcess(const std::string& executable, const StrVec& args, const std::string& directory, 
		bool startPaused, bool showWindow, int msTimeout)
	{
		STARTUPINFO si;
		PROCESS_INFORMATION pi;

		memset(&si, 0, sizeof(STARTUPINFO));
		memset(&pi, 0, sizeof(PROCESS_INFORMATION));

		si.cb = sizeof(si);

		si.dwFlags = STARTF_USESHOWWINDOW;
		si.wShowWindow = SW_SHOWDEFAULT;

		FlogD("starting process: " << executable);

		std::string env = Str("PATH=" << directory << "\0");

		char* cmdLine = strdup(Tools::Join({Str('"' << executable << '"'), Tools::Join(args)}).c_str());

		FlogExpD(cmdLine);
		int err = CreateProcess(executable.c_str(), cmdLine, NULL, NULL, FALSE,
			(showWindow ? 0 : CREATE_NO_WINDOW) | (startPaused ? CREATE_SUSPENDED : 0),
			(LPVOID)env.c_str(), directory.c_str(), &si, &pi);

		free(cmdLine);

		AssertEx(err != 0, PlatformEx, "(CreateProcess) win32 error: " << GetErrorStr(GetLastError()) << " (" << GetLastError() << ")");

		// wait for process to start before returning
		bool processStarted = false;

		for(int i = 0; i < msTimeout; i++){
			if(WaitForSingleObject(pi.hProcess, 0) == WAIT_TIMEOUT){
				processStarted = true;
				break;
			}

			Sleep(1);
		}

		if(!processStarted)
			throw PlatformEx(Str("failed to start process: " << executable));
				
		if(!AssignProcessToJobObject(jobHandle, pi.hProcess))
			throw PlatformEx(Str("failed to assign process to job: " << executable));
			
		return ProcessPtr(new Win32Process(pi));
	}

	
	virtual void GetRandChars(char* buffer, int len)
	{
		HCRYPTPROV hProvider = 0;

		if (!::CryptAcquireContext(&hProvider, 0, 0, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT | CRYPT_SILENT)){
			throw PlatformEx("CryptAcquireContext failed");
		}

		if (!::CryptGenRandom(hProvider, len, (BYTE*)buffer)){
			::CryptReleaseContext(hProvider, 0);
			throw PlatformEx("CryptGenRandom failed");
		}

		CryptReleaseContext(hProvider, 0);
	}
};

PlatformPtr Platform::Create()
{
	return PlatformPtr(new Win32Platform());
}
