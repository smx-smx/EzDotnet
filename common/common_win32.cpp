#include "common.h"

#ifdef __CYGWIN__
#include <sys/cygwin.h>
#else
#include "cygwin.h"
#endif

#include <Windows.h>

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
}

std::string to_native_path(const std::string& path){
	if(pfnCygwinConvPath == nullptr){
		return path;
	}

	int flags = CCP_POSIX_TO_WIN_A | CCP_ABSOLUTE;
	
	ssize_t size = pfnCygwinConvPath(flags, path.c_str(), nullptr, 0);
	if(size < 0){
		throw "cygwin_conv_path";
	}

	std::string buf(size, '\0');
	if(pfnCygwinConvPath(flags, path.c_str(), buf.data(), size) != 0){
		throw "cygwin_conv_path";
	}

	return buf;
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