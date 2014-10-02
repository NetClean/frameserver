#include "Platform.h"

#include <cstdlib>

#include "flog.h"

#define WIN32_LEAN_AND_MEAN
#define WIN32_EXTRA_LEAN
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
};

class Win32Platform : public Platform {
	public:
	HANDLE jobHandle;

	Win32Platform(){
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

	ProcessPtr StartProcess(const std::string& executable, const StrVec& args, const std::string& directory, bool startPaused)
	{
		STARTUPINFO si;
		PROCESS_INFORMATION pi;

		memset(&si, 0, sizeof(STARTUPINFO));
		memset(&pi, 0, sizeof(PROCESS_INFORMATION));

		si.cb = sizeof(si);

		si.dwFlags = STARTF_USESHOWWINDOW;
		si.wShowWindow = SW_SHOWDEFAULT;

		FlogD("starting process: " << executable);

		char* cmdLine = strdup(Tools::Join({Str('"' << executable << '"'), Tools::Join(args)}).c_str());
		FlogExpD(cmdLine);
		int err = CreateProcess(executable.c_str(), cmdLine, NULL, NULL, FALSE, CREATE_NO_WINDOW | (startPaused ? CREATE_SUSPENDED : 0), NULL, directory.c_str(), &si, &pi); 
		free(cmdLine);

		AssertEx(err != 0, PlatformEx, "(CreateProcess) win32 error: " << GetErrorStr(GetLastError()) << " (" << GetLastError() << ")");
		
		return ProcessPtr(new Win32Process(pi));
	}
};

PlatformPtr Platform::Create()
{
	return PlatformPtr(new Win32Platform());
}
