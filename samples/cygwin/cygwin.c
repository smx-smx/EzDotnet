/*
 * Copyright 2019 Stefano Moioli <smxdev4@gmail.com>
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

extern int mainCRTStartup(void);

static void(__cdecl *cygwin_dll_init)() = NULL;
static uintptr_t cygTlsSize;

uintptr_t cygwin_load(){
    char cygwinPath[MAX_PATH];

    char *systemDrive = getenv("SystemDrive");
    sprintf_s(cygwinPath, sizeof(cygwinPath), "%s\\cygwin64\\bin\\cygwin1.dll", systemDrive);

	HMODULE hCygwin = LoadLibraryA(cygwinPath);
	assert(hCygwin != NULL);

	uintptr_t (__cdecl *cygwin_internal)(cygwin_getinfo_types, ...) = NULL;

	cygwin_internal = (void *)GetProcAddress(hCygwin, "cygwin_internal");
	cygwin_dll_init = (void *)GetProcAddress(hCygwin, "cygwin_dll_init");
	assert(cygwin_internal != NULL);
	assert(cygwin_dll_init != NULL);

	cygTlsSize = cygwin_internal(CW_CYGTLS_PADSIZE);
	printf("CygTLS Size is %zu\n", cygTlsSize);
}

#if 0
void save_stack(void *pMem){
	uintptr_t localBase = (uintptr_t)pMem + sizeof(void *);

	PNT_TIB tib = (PNT_TIB)NtCurrentTeb();
	uintptr_t stackBase = (uintptr_t)tib->StackBase;

	size_t dumpSize = stackBase - localBase;
	printf("Dumping %zu bytes\n", dumpSize);

	FILE *hdmp = fopen("stack.bin", "wb");
	fwrite((void *)localBase, dumpSize, 1, hdmp);
	fclose(hdmp);
}
#endif

int __cdecl mainCRTStartupHook(){
	cygwin_load();
	_alloca(cygTlsSize);
	cygwin_dll_init();
	return mainCRTStartup();
}

int main(int argc, char *argv[]){
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
