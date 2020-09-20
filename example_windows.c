/*
 * Copyright 2019 Stefano Moioli <smxdev4@gmail.com>
 **/
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <windows.h>

#ifndef DEBUG_MODE
#define DEBUG_MODE 0
#endif

#ifndef LDR_DLL
#define LDR_DLL "ldr.dll"
#endif

#ifndef TARGET_ASM
#define TARGET_ASM "Example.dll"
#endif

#ifndef TARGET_ASM_CLASS
#define TARGET_ASM_CLASS "Example"
#endif

#ifndef TARGET_ASM_FUNC
#define TARGET_ASM_FUNC "Main"
#endif

typedef size_t PLGHANDLE;

typedef PLGHANDLE (*clrInitFunc)(const char *asmPath, const char *asmDir, int enableDebug);
typedef int (*runMethodFunc)(PLGHANDLE handle, const char *typeName, const char *methodName);

int go(){
	AllocConsole();
	freopen("CONIN$", "r", stdin); 
	freopen("CONOUT$", "w", stdout); 
	freopen("CONOUT$", "w", stderr);

	HMODULE hmod = LoadLibraryA(LDR_DLL);
	if(hmod == NULL){
		fprintf(stderr, "Failed to load %s\n", LDR_DLL);
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

	printf("calling clrInit, pwd: %s, asm: %s\n", buf, TARGET_ASM);
	PLGHANDLE handle = clrInit(TARGET_ASM, buf, DEBUG_MODE);
	
	printf("calling runMethod, handle: %zu\n", handle);
	runMethod(handle, TARGET_ASM_CLASS, TARGET_ASM_FUNC);
	return 0;
}

DWORD WINAPI threadedGo(LPVOID param){
	return go();
}

void deferredGo(){
	DWORD dwThreadId;
	HANDLE hThread = CreateThread(NULL, 0, threadedGo, NULL, 0, &dwThreadId);

	if(hThread == NULL){
		fputs("thread creation failed", stderr);
	}
}

BOOL WINAPI DllMain(
    HINSTANCE hinstDLL,  // handle to DLL module
    DWORD fdwReason,     // reason for calling function
    LPVOID lpReserved )  // reserved
{
    switch( fdwReason ) 
    { 
        case DLL_PROCESS_ATTACH:;
			#ifdef DEFERRED_LOAD
			deferredGo();
			#else
		 	go();
			#endif
            break;

        case DLL_THREAD_ATTACH:
            break;

        case DLL_THREAD_DETACH:
            break;

        case DLL_PROCESS_DETACH:
            break;
    }
    return TRUE;
}
