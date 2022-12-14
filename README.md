# EzDotNet
Load a C# assembly inside a native executable

## How does it work:
There are 3 backends:
- CLRHost for Windows (.NET Framework v4.x)
- MonoHost for any platform supporting Mono (Windows/macOS/Linux/etc...)
- CoreCLR for platforms supporting dotnet core

The backends expose the same interface so that it's possible to use them with the same APIs.

## Dotnet project setup
First, create a new console application:
> dotnet new console -o ManagedProgram

Then, add the `Microsoft.NETCore.DotNetAppHost` nuget package, for example via the Package Manager console:
> Install-Package Microsoft.NETCore.DotNetAppHost

Create a EntryPoint for the native code:

```csharp
namespace ManagedSample
{
	public class EntryPoint {
		private static int Entry(IntPtr args, int sizeBytes) {
			Main(new string[] { });
			return 0;
		}
	}
}
 ```

Note down the fully qualified class name (aka class name including namespace), and the method name. We will need them later in the native code.
In this example: `ManagedSample.EntryPoint` and `Entry`

**NOTE**: Make sure the .NET project is set to the correct processor architecture that matches your target. It's recommended to set `x86` or `x64` instead of `AnyCPU`

**NOTE**: When building the project, you **MUST** use the `publish` command (either from the IDE of `dotnet publish` from the command line).
This is important to make sure `nethost.dll` and required dependencies are copied to the output directory.

As an alternative, and to have F5 debug working, you can add the following target to the `.csproj` file to copy the nethost at build time:
```xml
    <ItemGroup Condition="'$(OS)'=='Windows_NT'">
        <PackageReference Include="runtime.win-x64.Microsoft.NETCore.DotNetAppHost" Version="7.0.0" />
    </ItemGroup>
    <Target AfterTargets="Build" Name="CopyHostFxr" Condition="'$(OS)'=='Windows_NT'">
        <Message Importance="high" Text="Copying nethost.dll" />
        <Copy SourceFiles="$(OutDir)runtimes\win-x64\native\nethost.dll" DestinationFolder="$(OutDir)" />
    </Target>
```
If you need multi-platform support, you will need to add multiple configurations for each OS/architecture combo


The following paragraph explains how to setup the native part

## Native API

To load a managed assembly, we need to pull the EzDotnet code into our project.

This can be done in one of the following ways:
- Statically linking against one of the backends: `coreclrhost`, `monohost` or `clrhost`
- Dynamicaly linking (`dlopen`/`LoadLibrary`)
- Using the dynamic helper (`ezdotnet_shared`)


The backends share a common interface:

- `ASMHANDLE clrInit(const char *assemblyPath, const char *baseDir, bool enableDebug)`

Loads the assembly specified by `assemblyPath`, sets the base search directory to `baseDir`

`enableDebug` is supported on Windows only (it's ignored on other platforms). If set, the JIT debugger will be invoked

Returns a handle to the loaded assembly

- `bool clrDeInit(ASMHANDLE handle)`

Deinitializes the execution environment (depending on the runtime, it might be impossible to call init again after this)


- `int runMethod(ASMHANDLE handle, const char *typeName, const char *methodName)`

Runs the method `methodName` inside the class `typeName` given a `handle` to the assembly loaded with `clrInit`

The method is expected to have the following signature:
```csharp
private static int Entry(IntPtr args, int sizeBytes) {
	Main(new string[] { });
	return 0;
}
 ```
The method visibility is ignored, so you can use `private` to hide the method from other assemblies.

The purpose of `Entry` is to read arguments passed from the native host and call the standard `Main` (for details, see the [Cygwin Sample](https://github.com/smx-smx/EzDotnet/blob/6a44ed661c4ea41f74c47698d908117628545717/samples/Managed/Cygwin/Program.cs#L29)).

Building the C# project as a console application is **strongly recommended**, as doing so will generate the necessary `runtimeconfig` json files, which are required if you're going to use the `CoreCLR` host.

All versions of CoreCLR are supported.


If you decide to use the dynamic helper, you have to resolve the `int main(int argc, char *argv[])` method from `ezdotnet_shared`, and invoke it as following:

```cpp
typedef int (*pfnEzDotNetMain)(int argc, const char *argv[]);

HMODULE ezDotNet = LoadLibraryA("libezdotnet_shared.dll");
pfnEzDotNetMain main = reinterpret_cast<pfnEzDotNetMain>(GetProcAddress(ezDotNet, "main"));
const char *argv[] = {
	// name of the program (argv0) - unused (can be set to anything)
	"ezdotnet",
	// path of the .NET backend to use
	"libcoreclrhost.dll",
	// path of the .NET assembly to load
	"bin/x86/Debug/net7.0/publishManagedSample.dll", 
	// fully qualified class name to invoke
	"ManagedSample.EntryPoint", 
	// name of the entry method inside the class (can be private)
	"Entry" 
};
pfnMain(5, argv);
```



## Use cases

### Executable or Library
You can use this project inside an executable or a library.
You can either link statically against a single loader or you can load them dynamically, so that the CLR engine to use (CLR/CoreCLR/Mono) can be chosen at runtime

**WARNING**

If you're loading EzDotnet from a dll, avoid loading the CLR from library constructors like `DllMain`. Doing so will cause a deadlock in `clrInit`, as some code in the native hosts will wait for a lock held within `DllMain` (probably by `ntdll.dll` or `kernel32.dll`) to be released.

Make sure you load the CLR from a separate thread, e.g. by using `CreateThread`, so that `DllMain` is free to return and the lock can be freed

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
