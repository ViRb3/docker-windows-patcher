#include <Windows.h>
#include <stdio.h>
#include <Psapi.h>

BOOL WINAPI DllMain(IN HINSTANCE hInstDLL, IN SIZE_T nReason, IN PVOID pvReserved)
{
	UNREFERENCED_PARAMETER(hInstDLL);

	if (DLL_PROCESS_ATTACH == nReason)
	{
		return main_DllMainProcessAttach();
	}
	else if (DLL_PROCESS_DETACH == nReason)
	{
		return main_DllMainProcessDetach();
	}
	else
	{
		return TRUE;
	}
}

BOOL Initialize();

static BOOL main_DllMainProcessAttach(VOID)
{
	Initialize();
	return TRUE;
}

static BOOL main_DllMainProcessDetach(VOID)
{
	return TRUE;
}

void WriteToMemory(uintptr_t addressToWrite, char* valueToWrite, int byteNum)
{
	unsigned long oldProtection;
	VirtualProtect((void*)(addressToWrite), byteNum, PAGE_EXECUTE_READWRITE, &oldProtection);
	memcpy((void*)(addressToWrite), valueToWrite, byteNum);
	VirtualProtect((void*)(addressToWrite), byteNum, oldProtection, NULL);
}

uintptr_t FindPattern(HMODULE hModule, char* pattern, char* mask)
{
	MODULEINFO mInfo = { 0 };
	GetModuleInformation(GetCurrentProcess(), hModule, &mInfo, sizeof(MODULEINFO));
	uintptr_t base = (uintptr_t)mInfo.lpBaseOfDll;
	uintptr_t size = (uintptr_t)mInfo.SizeOfImage;

	uintptr_t patternLength = (uintptr_t)strlen(mask);

	for (uintptr_t i = 0; i < size - patternLength; i++)
	{
		BOOL found = TRUE;
		for (uintptr_t j = 0; j < patternLength; j++)
		{
			found &= mask[j] == '?' || pattern[j] == *(char*)(base + i + j);
		}

		if (found)
		{
			return base + i;
		}
	}

	return NULL;
}

BOOL Initialize()
{
	HMODULE hDll = LoadLibrary(L"vmcompute.dll");
	if (!hDll)
		return FALSE;

	uintptr_t addr = FindPattern(hDll, "\x33\xff\xff\x76\x03\x00\x00", "x??xxxx");
	if (!addr)
		return FALSE;

	WriteToMemory(addr + 3, "\x36", 1);

	return TRUE;
}