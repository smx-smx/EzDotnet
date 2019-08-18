# SharpInj
Load a C# assembly inside a native executable

## How does it work:
There are 2 backends:
- CLRHost for Windows
- MonoHost for any platform supporting Mono (Windows/macOS/Linux/etc...)

## API
The backends share a common interface:

- `PLGHANDLE clrInit(const char *assemblyPath, const char *baseDir, bool enableDebug)`

Loads the assembly specified by `assemblyPath`, sets the base search directory to `baseDir`

`enableDebug` is supported on Windows only (it's ignored on other platforms). It will spawn a console via `AllocConsole` if true

if compiled with -DLAUNCH_DEBUGGER, the JIT debugger will also be invoked (still a Windows only feature)

Returns a handle to the loaded assembly


- `bool clrDeInit(PLGHANDLE handle)`

Deinitializes the assembly


- `int runMethod(PLGHANDLE handle, const char *typeName, const char *methodName)`

Runs the method `methodName` inside the class `typeName` given a `handle` to the assembly loaded with `clrInit`


## How to use
You will need to create a shared library that links against CLRHost or MonoHost and uses the APIs described in the previous section.

You will then need a way to inject this DLL in the process you're targeting.

There are several tools and ways to achieve this, for example

- Google (Windows: there are dozen injectors available as prebuilt tools)
- Detours (Windows library) has an API to spawn a process with a DLL
- SetSail (Windows: https://github.com/TheAssemblyArmada/SetSail) can inject a DLL at the EXE Entrypoint
- LD_PRELOAD (Linux, FreeBSD, and others) can be used to preload a library in a executable (at launch time)
- libhooker (Linux: https://github.com/smx-smx/libhooker) can inject a library in a running executable
