#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

#define DLLEXPORT __declspec(dllexport) extern

#define LIB_PREFIX ""
#define LIB_SUFFIX ".dll"

#if defined(WIN32) || defined(__CYGWIN__)
/**
 * Using CLRHost under cygwin requires LoadLibraryA
 * Using dlopen instead will cause
 *	 Assembly::GetExecutingAssembly()->Location
 * to throw due to invalid characters in the path (likely due to the cygwin path)
 **/
# include <windows.h>
# define LIB_HANDLE HMODULE
# define LIB_OPEN(path) LoadLibraryA(path)
# define LIB_GETSYM(handle, sym) GetProcAddress(handle, sym)
# define LIB_CLOSE(handle) FreeLibrary(handle)
#else
# include <dlfcn.h>
# define LIB_HANDLE void *
# define LIB_OPEN(path) dlopen(path, RTLD_GLOBAL)
# define LIB_GETSYM(handle, sym) dlsym(handle, sym)
# define LIB_CLOSE(handle) dlclose(handle)
#endif

#ifdef DEBUG
bool launchDebugger();
#endif

void initCygwin();

#ifdef __cplusplus
}
#endif