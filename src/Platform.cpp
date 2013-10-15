#include "Platform.h"

#include <cstdlib>

#define WIN32_LEAN_AND_MEAN
#define WIN32_EXTRA_LEAN
#include <windows.h>

class Win32Platform : public Platform {
	public:
	HANDLE jobHandle;

	Win32Platform(){
		// Create a job object with limits that kill the subprocesses on exit
		jobHandle = CreateJobObject(NULL, NULL);

		JOBOBJECT_BASIC_LIMIT_INFORMATION info;
		memset(&info, 0, sizeof(JOBOBJECT_BASIC_LIMIT_INFORMATION));
		info.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;

		SetInformationJobObject(jobHandle, JobObjectBasicLimitInformation, &info, sizeof(JobObjectBasicLimitInformation));
	}

	~Win32Platform(){
		CloseHandle(jobHandle);
	}

	void Sleep(int ms){
		Sleep(ms);
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

	void StartProcess(const std::string& executable, const StrVec& args, const std::string& directory)
	{
		STARTUPINFO si;
		PROCESS_INFORMATION pi;

		memset(&si, 0, sizeof(STARTUPINFO));
		memset(&pi, 0, sizeof(PROCESS_INFORMATION));

		si.cb = sizeof(si);

		si.dwFlags = STARTF_USESHOWWINDOW;
		si.wShowWindow = SW_SHOWDEFAULT;

		char* cmdLine = strdup(Tools::Join(args).c_str());
		CreateProcess(executable.c_str(), cmdLine, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, directory.c_str(), &si, &pi); 
		free(cmdLine);
	}
};

PlatformPtr Platform::Create()
{
	return PlatformPtr(new Win32Platform());
}
