#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

#define DLLEXPORT __declspec(dllexport) extern

#define LIB_PREFIX ""
#define LIB_SUFFIX ".dll"

#ifdef __CYGWIN__
# include <dlfcn.h>
# define LIB_HANDLE void *
# define LIB_OPEN(path) dlopen(path, RTLD_GLOBAL)
# define LIB_GETSYM(handle, sym) dlsym(handle, sym)
#else
# include <Windows.h>
# define LIB_HANDLE HMODULE
# define LIB_OPEN(path) LoadLibraryA(path)
# define LIB_GETSYM(handle, sym) GetProcAddress(handle, sym)
#endif

#ifdef DEBUG
bool launchDebugger();
#endif

void initCygwin();

#ifdef __cplusplus
}
#endif