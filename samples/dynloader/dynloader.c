#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <pthread.h>
#define INVALID_HANDLE_VALUE NULL
#endif


#include "common/common_win32.h"

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

typedef int (*pfnEzDotNetMain)(int argc, const char *argv[]);

int start_dotnet() {
	const char *helper_path = getenv("EZ_HELPER_PATH");
	const char *loader_path = getenv("EZ_LOADER_PATH");
	const char *asm_path = getenv("EZ_ASM_PATH");
	const char *asm_class_name = getenv("EZ_CLASS_NAME");
	const char *asm_method_name = getenv("EZ_CLASS_METHOD");

	bool fail=false;
	if(!helper_path){
		fputs("FATAL: EZ_HELPER_PATH not specified\n", stderr);
		fail = true;
	}
	if(!loader_path){
		fputs("FATAL: EZ_LOADER_PATH not specified\n", stderr);
		fail=true;
	}
	if(!asm_path){
		fputs("FATAL: EZ_ASM_PATH not specified\n", stderr);
		fail=true;
	}
	if(!asm_class_name){
		fputs("FATAL: EZ_CLASS_NAME not specified\n", stderr);
		fail=true;
	}
	if(!asm_method_name){
		fputs("FATAL: EZ_CLASS_METHOD not specified\n", stderr);
		fail=true;
	}

	if(fail){
		return EXIT_FAILURE;
	}

	LIB_HANDLE ezDotNet = LIB_OPEN(helper_path);
	if(ezDotNet == NULL || ezDotNet == INVALID_HANDLE_VALUE){
		fprintf(stdout, "LoadLibraryA failed\n");
		return EXIT_FAILURE;
	}
	const pfnEzDotNetMain pfnMain = (pfnEzDotNetMain)(void *)LIB_GETSYM(ezDotNet, "main");
	if (!pfnMain) {
		fputs("GetProcAddress failed\n", stderr);
		LIB_CLOSE(ezDotNet);
		return EXIT_FAILURE;
	}

	const char *argv[] = {
		"ezdotnet",
		loader_path,
		asm_path,
		asm_class_name,
		asm_method_name
	};
	const int result = pfnMain(ARRAY_SIZE(argv), argv);
	LIB_CLOSE(ezDotNet);
	return result;
}

void *start_dotnet_thread(void* arg){
	return (void *)(uintptr_t)start_dotnet();
}

#ifdef _WIN32
#define DLL_CTOR
#else
#define DLL_CTOR __attribute__((constructor(101)))
#endif

void DLL_CTOR dll_main() {
#ifdef _WIN32
	HANDLE hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)start_dotnet_thread, NULL, 0, NULL);
	if (hThread == NULL) {
		fputs("CreateThread failed\n", stderr);
		return;
	}
	CloseHandle(hThread);
#else
	pthread_t tid;
	if (pthread_create(&tid, NULL, &start_dotnet_thread, NULL) != 0) {
		fputs("pthread_create() failed\n", stderr);
		return;
	}
	if (pthread_detach(tid) != 0) {
		fputs("pthread_detach() failed\n", stderr);
	}
#endif
}


#ifdef _WIN32
BOOL WINAPI DllMain(
    HINSTANCE hinstDLL,
    DWORD fdwReason,
    LPVOID lpvReserved
){
    if(fdwReason == DLL_PROCESS_ATTACH){
        dll_main();
    }
    return TRUE;
}
#endif
