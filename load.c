/*
 * Copyright 2019 Stefano Moioli <smxdev4@gmail.com>
 **/
#include <stdio.h>
#include <stdint.h>
#include <windows.h>

#define DEBUG_MODE 0

#define LDR_DLL "ldr.dll"
#define TARGET_ASM "Example.dll"
#define TARGET_ASM_CLASS "Example"
#define TARGET_ASM_FUNC "Main"

typedef int (*clrInitFunc)(const char *asmPath, const char *asmDir, int enableDebug);
typedef int (*runMethodFunc)(size_t handle, const char *typeName, const char *methodName);

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
	
	printf("calling clrInit, pwd: %s\n", buf);
	size_t handle = clrInit(TARGET_ASM, buf, DEBUG_MODE);
	
	printf("calling runMethod, handle: %lu\n", handle);
	runMethod(handle, TARGET_ASM_CLASS, TARGET_ASM_FUNC);
	return 0;
}

DWORD WINAPI sleepAndGo(LPVOID param){
	Sleep(1000);
	return go();
}

void deferredGo(){
	DWORD dwThreadId;
	HANDLE hThread = CreateThread(NULL, 0, sleepAndGo, NULL, 0, &dwThreadId);

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
        case DLL_PROCESS_ATTACH:
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
