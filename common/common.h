#ifdef __cplusplus
extern "C" {
#endif


#ifdef _WIN32
#define DLLEXPORT __declspec(dllexport) extern
#else
#define DLLEXPORT extern
#endif

#ifdef __i386__
  #define APICALL __attribute__((__cdecl__))
#else
  #define APICALL
#endif

#ifdef DEBUG
#define DPRINTF(fmt, ...) \
	fprintf(stderr, "[%s]: " fmt, __func__, ##__VA_ARGS__)
#else
#define DPRINTF(fmt, ...)
#endif

#define NULL_ASMHANDLE 0
typedef size_t ASMHANDLE;
size_t str_hash(const char *str);

#if defined(_WIN32) && defined(DEBUG)
bool launchDebugger();
#endif


#if defined(WIN32) && !defined(__CYGWIN__)
# define LIB_HANDLE HMODULE
# define LIB_OPEN(path) LoadLibraryA(path)
# define LIB_GETSYM(handle, sym) GetProcAddress(handle, sym)
# define LIB_PREFIX ""
# define LIB_SUFFIX ".dll"
#else
# define LIB_HANDLE void *
# define LIB_OPEN(path) dlopen(path, RTLD_GLOBAL)
# define LIB_GETSYM(handle, sym) dlsym(handle, sym)
# ifdef __CYGWIN__
#  define LIB_PREFIX ""
#  define LIB_SUFFIX ".dll"
# else
#  define LIB_PREFIX "lib"
#  define LIB_SUFFIX ".so"
# endif
#endif

#ifdef __cplusplus
}
#endif