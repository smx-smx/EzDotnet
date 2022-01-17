#ifndef __COMMON_UNIX_H
#define __COMMON_UNIX_H

#define DLLEXPORT extern

#define LIB_PREFIX "lib"
#define LIB_SUFFIX ".so"

#include <dlfcn.h>
#define LIB_HANDLE void *
#define LIB_OPEN(path) dlopen(path, RTLD_NOW | RTLD_GLOBAL)
#define LIB_GETSYM(handle, sym) dlsym(handle, sym)

#endif