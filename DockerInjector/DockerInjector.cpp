#include <iostream>
#include <direct.h>
#include <windows.h>
#include <tlhelp32.h>
#include <algorithm>

BOOL FileExists(LPCTSTR szPath)
{
	DWORD dwAttrib = GetFileAttributes(szPath);

	return (dwAttrib != INVALID_FILE_ATTRIBUTES &&
		!(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}

BOOL SetDebugPrivilege()
{
	TOKEN_PRIVILEGES NewState;
	LUID luid;
	HANDLE hToken;

	OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &hToken);
	LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &luid);

	NewState.PrivilegeCount = 1;
	NewState.Privileges[0].Luid = luid;
	NewState.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

	AdjustTokenPrivileges(hToken, FALSE, &NewState, sizeof(NewState), NULL, NULL);

	CloseHandle(hToken);

	if (GetLastError() == ERROR_NOT_ALL_ASSIGNED)
	{
		return FALSE;
	}
	return TRUE;
}

DWORD GetProcessIdByName(LPCWCH lpczProc);
BOOL InjectDll(DWORD pid, std::wstring dllPath);

int main()
{
	WCHAR pwd[1024];
	GetCurrentDirectory(1024, pwd);

	std::wstring dllPath = std::wstring(pwd);
	dllPath += L"\\DockerPatch.dll";

	if (FileExists(dllPath.c_str()) == FALSE) {
		puts("DLL file missing!");
		puts("Make sure it is present in the current directory");
		return 1;
	}

	puts("Setting debug privilege...");
	if (SetDebugPrivilege() == FALSE)
	{
		puts("Unable to set debug privilege!");
		puts("Make sure this program is running as Administrator");
		return 2;
	}

	DWORD pid = GetProcessIdByName(L"dockerd.exe");
	if (pid == -1)
	{
		puts("Process not found");
		return 3;
	}

	if (InjectDll(pid, dllPath) == TRUE)
	{
		return 0;
	}
	else 
	{
		return 4;
	}
	
}

BOOL InjectDll(DWORD pid, std::wstring dllPath)
{
	puts("Opening process...");
	//Get the process handle for the target process
	HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
	if (!hProcess)
	{
		puts("Could not find process");
		return FALSE;
	}

	puts("Injecting...");

	//Retrieve kernel32.dll module handle to get LoadLibrary base address
	HMODULE hModule = GetModuleHandle(L"kernel32.dll");

	//Get address of LoadLibraryA in kernel32.dll
	LPVOID lpBaseAddress = static_cast<LPVOID>(GetProcAddress(hModule, "LoadLibraryW"));
	if (lpBaseAddress == nullptr)
	{
		puts("Unable to locate LoadLibraryW");
		return FALSE;
	}

	//Allocate space in target process for inject.dll
	LPVOID lpSpace = static_cast<LPVOID>(VirtualAllocEx(hProcess, nullptr, dllPath.length() * sizeof(WCHAR), MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE));
	if (lpSpace == nullptr)
	{
		printf("Could not allocate memory in process %u\n", static_cast<int>(pid));
		return FALSE;
	}

	//Write inject.dll to memory of process
	int n = WriteProcessMemory(hProcess, lpSpace, dllPath.c_str(), dllPath.length() * sizeof(WCHAR), nullptr);
	if (n == 0)
	{
		puts("Could not write to process's address space");
		return FALSE;
	}

	HANDLE hThread = CreateRemoteThread(hProcess, nullptr, 0, static_cast<LPTHREAD_START_ROUTINE>(lpBaseAddress), lpSpace, 0, nullptr);
	if (hThread == nullptr)
	{
		return FALSE;
	}
	else
	{
		DWORD threadId = GetThreadId(hThread);
		DWORD processId = GetProcessIdOfThread(hThread);
		printf("Injected thread ID: %u for PID: %u", threadId, processId);

		CloseHandle(hProcess);
		return TRUE;
	}
}

DWORD GetProcessIdByName(const LPCWCH name)
{
	HANDLE hSnap;
	PROCESSENTRY32 peProc;
	DWORD dwRet = -1;

	if ((hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0)) != INVALID_HANDLE_VALUE)
	{
		peProc.dwSize = sizeof(PROCESSENTRY32);

		if (Process32First(hSnap, &peProc))
			while (Process32Next(hSnap, &peProc))
				if (!wcscmp(name, peProc.szExeFile))
					dwRet = peProc.th32ProcessID;
	}

	CloseHandle(hSnap);

	return dwRet;
}
