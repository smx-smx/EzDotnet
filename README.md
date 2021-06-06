# EzDotNet
Load a C# assembly inside a native executable

## How does it work:
There are 3 backends:
- CLRHost for Windows (.NET Framework v4.x)
- MonoHost for any platform supporting Mono (Windows/macOS/Linux/etc...)
- CoreCLR for platforms supporting dotnet core

The backends expose the same interface so that it's possible to use them with the same APIs.

## API
The backends share a common interface:

- `ASMHANDLE clrInit(const char *assemblyPath, const char *baseDir, bool enableDebug)`

Loads the assembly specified by `assemblyPath`, sets the base search directory to `baseDir`

`enableDebug` is supported on Windows only (it's ignored on other platforms). If set, the JIT debugger will be invoked

Returns a handle to the loaded assembly

- `bool clrDeInit(ASMHANDLE handle)`

Deinitializes the execution environment (depending on the runtime, it might be impossible to call init again after this)


- `int runMethod(ASMHANDLE handle, const char *typeName, const char *methodName)`

Runs the method `methodName` inside the class `typeName` given a `handle` to the assembly loaded with `clrInit`


## Use cases

### Executable or Library
You can use this project inside an executable or a library.
You can either link statically against a single loader or you can load them dynamically, so that the CLR engine to use (CLR/CoreCLR/Mono) can be chosen at runtime

### Cygwin Interop
This project enables you to call Cygwin code from .NET.
For this use case, the entry point (`samples/cli/ezdotnet`) **MUST** be compiled under Cygwin.

In other words, you can call code Cygwin code from .NET only if you're starting with a Cygwin process, and you load .NET afterwards.
Starting from Win32 and calling into Cygwin will **NOT** work

This means that, if you want to build a typical CLI or Windows Forms application with Cygwin features, you will need to start the application with `ezdotnet` under Cygwin for it to work properly.

### Process Injection
If you're building a shared library, you can inject it into another process to enable it to run .NET code.
For this use case you will need to use a library injector
There are several tools and ways to achieve this, for example

- Google (Windows: there are dozen injectors available as prebuilt tools)
- Detours (Windows library) has an API to spawn a process with a DLL
- SetSail (Windows: https://github.com/TheAssemblyArmada/SetSail) can inject a DLL at the EXE Entrypoint
- LD_PRELOAD (Linux, FreeBSD, and others) can be used to preload a library in a executable (at launch time)
- ezinject (https://github.com/smx-smx/ezinject) can inject a library in a running executable
