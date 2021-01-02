/*
 * Copyright (C) 2021 Stefano Moioli <smxdev4@gmail.com>
 **/
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <windows.h>

#include "cygwin.h"

#ifndef DEBUG_MODE
#define DEBUG_MODE 0
#endif

typedef size_t ASMHANDLE;
typedef ASMHANDLE (*clrInitFunc)(const char *asmPath, const char *asmDir, int enableDebug);
typedef int (*runMethodFunc)(ASMHANDLE handle, const char *typeName, const char *methodName);

int go(
    const char *loaderPath,
    const char *asmPath,
    const char *targetClassName, const char *targetMethodName
){
	HMODULE hmod = LoadLibraryA(loaderPath);
	if(hmod == NULL){
		fprintf(stderr, "Failed to load %s\n", loaderPath);
		return -1;
	}

	fprintf(stderr, "Handle: %p\n", hmod);
	clrInitFunc clrInit = (clrInitFunc)GetProcAddress(hmod, "clrInit");
	if(clrInit == NULL){
		fputs("clrInit not found", stderr);
		return -1;
	}

	runMethodFunc runMethod = (runMethodFunc)GetProcAddress(hmod, "runMethod");
	if(runMethod == NULL){
		fputs("runMethod not found", stderr);
		return -1;
	}

	TCHAR buf[255];
	GetCurrentDirectory(sizeof(buf), buf);

	printf("calling clrInit, pwd: %s, asm: %s\n", buf, asmPath);
	ASMHANDLE handle = clrInit(asmPath, buf, DEBUG_MODE);
	
	printf("calling runMethod, handle: %zu\n", handle);
	runMethod(handle, targetClassName, targetMethodName);
	return 0;
}

#if defined(_WIN32) || defined(__CYGWIN__)
#define ENV_PUT(key, val) SetEnvironmentVariable(key, val)
#else
#define ENV_PUT(key, val) setenv(key, val, 1)
#endif

int main(int argc, char *argv[]){
    char tmp[50];
    snprintf(tmp, sizeof(tmp), "%p", &argv[5]);
    ENV_PUT("EZDOTNET_ARGV", tmp);
    snprintf(tmp, sizeof(tmp), "%d", argc - 5);
    ENV_PUT("EZDOTNET_ARGC", tmp);

    if(argc < 5){
        fprintf(stderr, "Usage: %s [loaderPath] [asmPath] [className] [methodName]\n", argv[0]);
        return 1;
    }
    const char *loaderPath = argv[1];
    const char *asmPath = argv[2];
    const char *className = argv[3];
    const char *methodName = argv[4];
	go(loaderPath, asmPath, className, methodName);
	return 0;
}
