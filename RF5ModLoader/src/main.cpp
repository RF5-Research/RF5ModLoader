#include "pch.h"
#include <string>
#include <filesystem>
#include <fstream>
#include <polyhook2/Detour/x64Detour.hpp>
#include <polyhook2/CapstoneDisassembler.hpp>
#include <polyhook2/PE/IatHook.hpp>
#include "main.h"

using namespace std;

const char* gameAssemblyName = "GameAssembly";
std::wstring DataDir;
std::wstring DataDirName;

FARPROC GetModuleSymbolAddress(const char* module, const char* symbol)
{
	HMODULE moduleHandle = GetModuleHandleA(module);
	if (moduleHandle == NULL) {
		printf("Failed to load module `%s`\n", module);
		return NULL;
	}
	FARPROC funcPTR = GetProcAddress(moduleHandle, symbol);
	if (funcPTR == NULL)
	{
		printf("Failed to load symbol `%s` in module `%s`\n", symbol, module);
		return NULL;
	}
	return funcPTR;
}

void wstring_replace(
	std::wstring& s,
	std::wstring const& toReplace,
	std::wstring const& replaceWith
) {
	std::size_t pos = s.find(toReplace);
	if (pos == std::wstring::npos) return;
	s.replace(pos, toReplace.length(), replaceWith);
}

uint64_t okernel32_CreateFileW = NULL;
NOINLINE HANDLE __cdecl Hook_kernel32_CreateFileW(
	_In_ LPCWSTR lpFileName,
	_In_ DWORD dwDesiredAccess,
	_In_ DWORD dwShareMode,
	_In_opt_ LPSECURITY_ATTRIBUTES lpSecurityAttributes,
	_In_ DWORD dwCreationDisposition,
	_In_ DWORD dwFlagsAndAttributes,
	_In_opt_ HANDLE hTemplateFile
)
{
	auto funcPTR = PLH::FnCast(okernel32_CreateFileW, Hook_kernel32_CreateFileW);
	auto filepath = std::filesystem::path(lpFileName).lexically_normal().wstring();
	if (filepath.find(DataDir) != std::wstring::npos)
	{
		wstring_replace(filepath, DataDirName, L"mods");
		if (std::filesystem::exists(filepath))
		{
			wprintf(L"Patching file: %s\n", filepath.c_str());
			return funcPTR(filepath.c_str(), dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
		}
	}
	return funcPTR(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
}

//void LoadPlugins()
//{
//	HANDLE process = GetCurrentProcess();
//	auto dir = std::format("{}/{}", filesystem::current_path().generic_string().c_str(), "plugins");
//	if (filesystem::exists(dir))
//	{
//		for (const auto& dirEntry : filesystem::recursive_directory_iterator::recursive_directory_iterator(dir))
//		{
//			if (dirEntry.is_regular_file())
//			{
//				auto plugin = dirEntry.path();
//				if (plugin.extension().compare("dll"))
//				{
//					auto moduleName = plugin.generic_string();
//					LoadLibraryA(moduleName.c_str());
//					printf("Inject Module: %s\n", moduleName.c_str());
//				}
//			}
//		}
//	}
//}

//uint64_t oil2cpp_thread_attach;
//PLH::x64Detour* Detour_il2cpp_thread_attach;
//NOINLINE void* __cdecl Hook_il2cpp_thread_attach(void* domain)
//{
//	//Important to procedurally do it this way, or else it'll break
//	auto thread = PLH::FnCast(oil2cpp_thread_attach, Hook_il2cpp_thread_attach)(domain);
//	Detour_il2cpp_thread_attach->unHook();
//	LoadPlugins();
//	return thread;
//}

//uint64_t oLoadLibraryW;
//PLH::x64Detour* Detour_LoadLibraryW;
//NOINLINE HMODULE __cdecl Hook_LoadLibraryW(LPCWSTR lpLibFileName)
//{
//	printf("Test\n");
//	auto moduleName = std::format("{}.dll", gameAssemblyName);
//	size_t newsize = strlen(moduleName.c_str()) + 1;
//	wchar_t* wcstring = new wchar_t[newsize];
//	size_t convertedChars = 0;
//	mbstowcs_s(&convertedChars, wcstring, newsize, moduleName.c_str(), _TRUNCATE);
//
//	if (std::wstring(lpLibFileName).compare(wcstring))
//	{
//		Detour_LoadLibraryW->unHook();
//		LoadLibraryW(wcstring);
//		PLH::CapstoneDisassembler dis(PLH::Mode::x64);
//		Detour_il2cpp_thread_attach = new PLH::x64Detour(
//			reinterpret_cast<char*>(GetModuleSymbolAddress(moduleName.c_str(), "il2cpp_thread_attach")),
//			reinterpret_cast<char*>(&Hook_il2cpp_thread_attach),
//			&oil2cpp_thread_attach,
//			dis
//		);
//		Detour_il2cpp_thread_attach->hook();
//		return GetModuleHandleA(moduleName.c_str());
//	}
//	else
//	{
//		return PLH::FnCast(oLoadLibraryW, Hook_LoadLibraryW)(lpLibFileName);
//	}
//}

DllExport void Initialize()
{
	freopen_s(reinterpret_cast<FILE**>(stdout), "CONOUT$", "w", stdout);

	wchar_t buffer[MAX_PATH];
	GetModuleFileNameW(NULL, buffer, MAX_PATH);
	auto fileName = std::filesystem::path(buffer).stem();

	auto dataDir = ::filesystem::absolute(std::format("{}_Data", fileName.generic_string().c_str()));
	DataDir = dataDir.lexically_normal();
	DataDirName = dataDir.stem();

	if (!std::filesystem::exists("mods"))
		std::filesystem::create_directory("mods");
	//if (!std::filesystem::exists("plugins"))
	//	std::filesystem::create_directory("plugins");

	PLH::CapstoneDisassembler dis(PLH::Mode::x64);
	//Detour_LoadLibraryW = new PLH::x64Detour(
	//	reinterpret_cast<char*>(LoadLibraryW),
	//	reinterpret_cast<char*>(&Hook_LoadLibraryW),
	//	&oLoadLibraryW,
	//	dis
	//);
	//Detour_LoadLibraryW->hook();

	auto detour_kernel32_CreateFileW = new PLH::x64Detour(
		reinterpret_cast<char*>((GetModuleSymbolAddress("kernel32", "CreateFileW"))),
		reinterpret_cast<char*>(&Hook_kernel32_CreateFileW),
		&okernel32_CreateFileW,
		dis
	);
	detour_kernel32_CreateFileW->hook();
}