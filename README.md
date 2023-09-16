# EzDotNet
Load a C# assembly inside a native executable

## Project structure:
There are 3 backends:
- CLRHost for Windows (.NET Framework v4.x)
- MonoHost for any platform supporting Mono (Windows/macOS/Linux/etc...)
- CoreCLR for platforms supporting dotnet core

The backends expose the same interface so that it's possible to swap them while keeping the same code.

To load a managed assembly, we need to pull the EzDotnet APIs into our project.

This can be done in one of the following ways:
- Statically linking against one of the backends: `coreclrhost`, `monohost` or `clrhost`
- Dynamic linking (`dlopen`/`LoadLibrary`)
- Using the sample dynamic helper library (`ezdotnet_shared`)

## C# Project setup
First, create a new console application:
```bash
dotnet new console -o ManagedSample
```

Then, add the `Microsoft.NETCore.DotNetAppHost` nuget package, for example via the `dotnet` cli:
```bash
dotnet add package Microsoft.NETCore.DotNetAppHost
```

Now create a EntryPoint for the native code, using the following code as a starting point:

```csharp
namespace ManagedSample
{
	public class EntryPoint {
		private static string[] ReadArgv(IntPtr args, int sizeBytes) {
			int nargs = sizeBytes / IntPtr.Size;
			string[] argv = new string[nargs];
			
			for(int i=0; i<nargs; i++, args += IntPtr.Size) {
				IntPtr charPtr = Marshal.ReadIntPtr(args);
				argv[i] = Marshal.PtrToStringAnsi(charPtr);
			}
			return argv;
		}
		
		private static int Entry(IntPtr args, int sizeBytes) {
			string[] argv = ReadArgv(args, sizeBytes);
			Main(argv);
			return 0;
		}

		public static void Main(string[] args){
			Console.WriteLine("Hello, World");
		}
	}
}
```

Note down the fully qualified class name (the class name including namespace - `ManagedSample.EntryPoint`), and the method name (`Entry`). We will need them later in the native code.

**NOTE**: It's *strongly* recommended to build the C# project as a console application in order to generate the necessary `runtimeconfig` json files, which are required if you're going to use the `CoreCLR` host.

**NOTE**: The `AnyCPU` processor architecture should work normally. In case of issues, change the target processor architecture to match the one used by the native loader.

**NOTE**: When building the project, you **MUST** use a `publish` command like the following (either from the IDE or `dotnet publish` from the command line):

```bash
dotnet publish -r linux-x64 --no-self-contained
```

This is important to make sure the correct `nethost.dll` and required dependencies are copied to the output directory.
`--no-self-contained` is also required, or you will get the following error at load time: `Initialization for self-contained components is not supported`


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
If you need multi-platform support, you will need to add multiple configurations for each runtime ID (`rid`) and OS combo


The following paragraph explains how to setup the native loader:

## Native setup
A sample dynamic helper is provided to ease the process of loading .NET and calling the entry point of an assembly.

Otherwise, refer to the [API Documentation](#api-documentation) to use static/dynamic linking yourself.

### Dynamic helper

If you decide to use the dynamic helper, you have to load `ezdotnet_shared` and resolve the `int main(int argc, char *argv[])` method.

Refer ot the following sample for details:

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
// call main(argc, argv)
pfnMain(5, argv);
```

### API documentation

The backends share a common interface:

#### clrInit

- `ASMHANDLE clrInit(const char *assemblyPath, const char *baseDir, bool enableDebug)`

Loads the assembly specified by `assemblyPath`, sets the base search directory to `baseDir`

> NOTE: `enableDebug` is supported on Windows only (it's ignored on other platforms). If set, the JIT debugger will be invoked

Returns: a handle to the loaded assembly


#### clrDeInit
- `bool clrDeInit(ASMHANDLE handle)`

Deinitializes the execution environment.

> NOTE: on some runtimes, it might be impossible to call `clrInit` again afterwards


#### runMethod
- `int runMethod(ASMHANDLE handle, const char *typeName, const char *methodName)`

Runs the method `methodName` inside the class `typeName`, given a `handle` to an assembly loaded by a previous `clrInit` call.

The C# method is expected to have the following signature:
```csharp
		private static int Entry(IntPtr args, int sizeBytes) {
			string[] argv = ReadArgv(args, sizeBytes);
			Main(argv);
			return 0;
		}
```
> NOTE: The C# method visibility is ignored, so you can supply `private` methods as well

In this example, the `Entry` function reads arguments passed from the native host, and calls the Program `Main` entry  (for details, see the [Cygwin Sample](https://github.com/smx-smx/EzDotnet/blob/6a44ed661c4ea41f74c47698d908117628545717/samples/Managed/Cygwin/Program.cs#L29) or the [Project Setup](#c-project-setup)).

## Use cases

### Executable or Library
You can use EzDotnet inside an executable or a library.
You can either link statically against a single loader or use dynamic linking (e.g. `dlopen`) so that the engine to use (CLR/CoreCLR/Mono) can be chosen at runtime.

**WARNING**

If you're loading EzDotnet from a DLL, avoid loading the CLR from library constructors like `DllMain`. Doing so will cause a deadlock in `clrInit`.

Instead, create a separate thread and use it to load the CLR, so that `DllMain` is free to return.

### Cygwin Interoperability
This project enables you to call Cygwin code from .NET.
For this use case, the .NET host/loader (for example `samples/cli/ezdotnet`, or `libcoreclrhost`) **MUST** be compiled under Cygwin.

In other words, you can call code Cygwin code from .NET only if you're starting with a Cygwin process, and you load .NET afterwards.
Starting from Win32 and calling into Cygwin will **NOT** work

Therefore, if you want to build a typical CLI or Windows Forms application with Cygwin features, you will need to start the application with the `ezdotnet` CLI for it to work properly.

**NOTE**: The `ezdotnet` CLI **MUST** be compiled as a Cygwin application.

### Process Injection
If you're building a shared library, you can inject it into another process to enable it to run .NET code.
For this use case you will need to use a library injector.
There are several tools and ways to achieve this, for example:

#### Windows
- [Detours](https://github.com/microsoft/Detours) has an API to spawn a process with a DLL
- [SetSail](https://github.com/TheAssemblyArmada/SetSail) can inject a DLL at the EXE Entrypoint

#### Unix-like
- [LD_PRELOAD](https://man7.org/linux/man-pages/man8/ld.so.8.html) (Linux, FreeBSD, and others) can be used to preload a library in a executable (at launch time)

#### Universal
- [ezinject](https://github.com/smx-smx/ezinject) can inject a library in a running executable
- or use your favorite injector