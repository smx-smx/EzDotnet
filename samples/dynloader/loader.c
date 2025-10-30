#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include "common/common_win32.h"

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

extern int ezdotnet_main(int argc, const char *argv[]);

int start_dotnet() {
    const char *loader_path = getenv("EZ_LOADER_PATH");
    const char *asm_path = getenv("EZ_ASM_PATH");
    const char *asm_class_name = getenv("EZ_CLASS_NAME");
    const char *asm_method_name = getenv("EZ_CLASS_METHOD");

    bool fail=false;
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

    const char *argv[] = {
        "ezdotnet",
        loader_path,
        asm_path,
        asm_class_name,
        asm_method_name
    };
    return ezdotnet_main(ARRAY_SIZE(argv), argv);
}

void *start_dotnet_thread(void* arg){
    return (void *)(uintptr_t)start_dotnet();
}

void __attribute__((constructor(101))) dll_main() {
    pthread_t tid;
    if (pthread_create(&tid, NULL, &start_dotnet_thread, NULL) != 0) {
        fputs("pthread_create() failed\n", stderr);
        return;
    }
    if (pthread_detach(tid) != 0) {
        fputs("pthread_detach() failed\n", stderr);
    }
}


