#include <algorithm>
#include <vector>
#include <Windows.h>

#ifdef __CYGWIN__
#include <sys/cygwin.h>
#else
#include "cygwin.h"
#endif

#include "common.h"

static bool running_in_cygwin = false;
static ssize_t (*pfnCygwinConvPath) (cygwin_conv_path_t what, const void *from, void *to, size_t size) = nullptr;

template<typename F>
void MyGetProcAddress(HMODULE hmod, const char *name, F &dst) {
	dst = reinterpret_cast<F>(::GetProcAddress(hmod, name));
}

void initCygwin() {
	HMODULE hCygwin = GetModuleHandle("cygwin1");
	::running_in_cygwin = hCygwin != nullptr;
	if(!::running_in_cygwin){
		return;
	}

	MyGetProcAddress(hCygwin, "cygwin_conv_path", ::pfnCygwinConvPath);
	::running_in_cygwin = ::pfnCygwinConvPath != nullptr;
}

std::string cygwin_conv_path(const std::string& path, int flags){
	ssize_t size = pfnCygwinConvPath(flags, path.c_str(), nullptr, 0);
	if(size < 0){
		throw "cygwin_conv_path";
	}

	std::vector<char> buf = std::vector<char>(size);
	if(pfnCygwinConvPath(flags, path.c_str(), buf.data(), size) != 0){
		throw "cygwin_conv_path";
	}

	return std::string(buf.data());
}

std::string to_windows_path(const std::string& path){
	return ::cygwin_conv_path(path, CCP_POSIX_TO_WIN_A | CCP_ABSOLUTE);
}

std::string to_native_path(const std::string& path){
	if(!::running_in_cygwin){
		std::string path_cpy = path;
		std::replace(path_cpy.begin(), path_cpy.end(), '/', '\\');
		return path_cpy;
	}
	return ::to_windows_path(path);
}

char *to_windows_path(const char *path){
	std::string str(path);
	std::string converted = ::to_windows_path(str);
	return strdup(converted.c_str());
}

#ifdef DEBUG
//https://stackoverflow.com/a/20387632
bool launchDebugger() {
	std::wstring systemDir(MAX_PATH + 1, '\0');
	UINT nChars = GetSystemDirectoryW(&systemDir[0], systemDir.length());
	if (nChars == 0)
		return false;
	systemDir.resize(nChars);

	DWORD pid = GetCurrentProcessId();
	std::wostringstream s;
	s << systemDir << L"\\vsjitdebugger.exe -p " << pid;
	std::wstring cmdLine = s.str();

	STARTUPINFOW si;
	memset(&si, 0x00, sizeof(si));
	si.cb = sizeof(si);

	PROCESS_INFORMATION pi;
	memset(&pi, 0x00, sizeof(pi));

	if (!CreateProcessW(
		nullptr, &cmdLine[0],
		nullptr, nullptr,
		false, 0, nullptr, nullptr,
		&si, &pi
	)) {
		return false;
	}

	CloseHandle(pi.hThread);
	CloseHandle(pi.hProcess);

	while (!IsDebuggerPresent())
		Sleep(100);

	DebugBreak();
	return true;
}
#endif