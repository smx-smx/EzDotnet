#pragma region License
/*
 * Copyright (C) 2021 Stefano Moioli <smxdev4@gmail.com>
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#pragma endregion

#include <msclr/marshal_cppstd.h>
#include <filesystem>
#include <string>
#include <map>
#include <unordered_map>
#include <vcclr.h>

using namespace System;
using namespace System::Collections;
using namespace System::Collections::Generic;
using namespace System::IO;
using namespace System::Reflection;
using namespace System::Diagnostics;
using namespace System::Threading;
using namespace System::Runtime::InteropServices;
using namespace System::Runtime::Remoting;

typedef size_t PLGHANDLE;

/**
 * Loads an assembly by looking up the same folder as the executing assembly
 */
ref struct SameFolderAssemblyLoader {
public:
	static Assembly^ LoadFromSameFolder(Object^ sender, ResolveEventArgs^ args) {
		String^ folderPath = Path::GetDirectoryName(Assembly::GetExecutingAssembly()->Location);
		String^ assemblyName = (gcnew AssemblyName(args->Name))->Name + ".dll";
		String^ assemblyPath = Path::Combine(folderPath, assemblyName);
		if (!File::Exists(assemblyPath))
			return nullptr;
		Assembly^ assembly = Assembly::LoadFrom(assemblyPath);
		return assembly;
	}
};

/**
 * Loads an assembly by looking up a list of folders
 */
ref class PathListAssemblyLoader {
	List<String^>^ asmPathList;
	Assembly^ LoadFromPathList(Object^ sender, ResolveEventArgs^ args) {
		for each (String ^ dirPath in asmPathList) {
			String^ assemblyName = (gcnew AssemblyName(args->Name))->Name + ".dll";
			String^ candidatePath = Path::Combine(dirPath, assemblyName);
			if (File::Exists(candidatePath)) {
				Assembly^ assembly = Assembly::LoadFrom(candidatePath);
				return assembly;
			}
		}
		return nullptr;
	}

public:
	PathListAssemblyLoader(List<String^>^ asmPathList) {
		AppDomain^ appDom = AppDomain::CurrentDomain;
		this->asmPathList = asmPathList;
		appDom->AssemblyResolve += gcnew ResolveEventHandler(this, &PathListAssemblyLoader::LoadFromPathList);
	}

	Assembly^ LoadFrom(String^ assemblyPath) {
		AppDomain^ appDom = AppDomain::CurrentDomain;

		AssemblyName^ asmName = gcnew AssemblyName();
		asmName->CodeBase = assemblyPath;
		return appDom->Load(asmName);
	}

};

/**
 * Wraps the assembly instance
 */
[Serializable]
ref class AssemblyInstance : MarshalByRefObject {
private:
	Assembly^ targetAsm;

	MethodInfo^ findMethod(String^ typeName, String^ methodName) {
		Type^ type;
		for each (Type^ _type in targetAsm->GetTypes()) {
			if (_type->Name->Equals(typeName)) {
				type = _type;
				break;
			}
		}

		if (type == nullptr)
			return nullptr;

		return type->GetMethod(methodName,
			BindingFlags::Instance | BindingFlags::Static |
			BindingFlags::NonPublic | BindingFlags::Public
		);
	}

public:
	Object^ InitializeLifetimeService() override {
		return nullptr;
	}

	AssemblyInstance(String ^ assemblyPath, List<String^> ^ asmSearchPaths) {
		targetAsm = (gcnew PathListAssemblyLoader(asmSearchPaths))->LoadFrom(assemblyPath);
	}

	int runMethod(String^ className, String^ methodName) {
		MethodInfo^ method = findMethod(className, methodName);
		if (method == nullptr)
			return -1;

		method->Invoke(nullptr, nullptr);
		return 0;
	}
};

struct AssemblyInstanceData {
public:
	gcroot<AppDomain^> appDomain;
	gcroot<AssemblyInstance^> instance;

	AssemblyInstanceData() {}
	AssemblyInstanceData(AppDomain^ appDomain, AssemblyInstance^ instance) : appDomain(appDomain), instance(instance) {
	}
};

static std::map<PLGHANDLE, AssemblyInstanceData> gHandles;


size_t str_hash(const char* str)
{
	unsigned long hash = 5381;
	int c;

	while (c = *str++)
		hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

	return hash;
}

extern "C" {
	typedef char* (*message_callback_t)(const char*);
	typedef void(*exit_callback_t)();

	__declspec(dllexport)
		extern int __cdecl runMethod(PLGHANDLE handle, const char* typeName, const char* methodName) {

		if (gHandles.find(handle) == gHandles.end()) {
			return -1;
		}

		gHandles.at(handle).instance->runMethod(
			gcnew String(typeName),
			gcnew String(methodName)
		);
		return 0;
	}

	__declspec(dllexport)
		extern const PLGHANDLE __cdecl clrInit(
			const char* assemblyPath, const char* pluginFolder, int enableDebug
		) {
		if (enableDebug) {
			if (!Debugger::IsAttached) {
				Debugger::Launch();
			}
			while (!Debugger::IsAttached) {
				Thread::Sleep(100);
			}
			Debugger::Break();
		}

		/**
		 * Get Paths
		 */
		std::filesystem::path asmPath(assemblyPath);
		std::string pluginName = asmPath.filename().replace_extension().string();

		PLGHANDLE handle = str_hash(pluginName.c_str());

		if (gHandles.find(handle) != gHandles.end()) {
			//gPlugins.at(handle).instance->Initialize(IntPtr(cb1), IntPtr(cb2), enableDebug);
			return handle;
		}

		/**
		 * Create AppDomain
		 */
		String^ applicationName = gcnew String(pluginName.c_str());
		AppDomainSetup^ domainSetup = gcnew AppDomainSetup();
		domainSetup->ApplicationName = applicationName;
		domainSetup->ApplicationBase = gcnew String(pluginFolder);
		AppDomain^ newDomain = AppDomain::CreateDomain(applicationName, nullptr, domainSetup);

		/**
		 * Use the Directory we're running from to resolve assemblies (e.g this assembly)
		 * We have to use this because ApplicationBase and PrivateBinPath aren't sane (ApplicationBase will point to the Kodi's exe dir)
		 * Since PrivateBinPath entries are resolved relative to the ApplicationBase, it's a no-go.
		 * We would need to control the AppDomain creation in pure C++ (e.g before this code is ran)
		 *
		 * CreateInstanceFromAndUnwrap will try to resolve where the Type/Class is (it's in this assembly)
		 * and it will try to load that assembly in the new AppDomain.
		 *
		 * The current AppDomain is used for the resolution, so we need to install a LoadFromSameFolder resolver handler
		 * on the current AppDomain so that the CLRHost assembly can be found
		 */
		AppDomain::CurrentDomain->AssemblyResolve += gcnew ResolveEventHandler(&SameFolderAssemblyLoader::LoadFromSameFolder);

		/**
		 * Setup Assembly search paths
		 */

		String^ thisAsmPath = Assembly::GetExecutingAssembly()->Location;
		List<String^>^ asmPaths = gcnew List<String^>();
		asmPaths->Add(Path::GetDirectoryName(thisAsmPath));

		std::string asmParent = asmPath.parent_path().string();
		if (asmParent.length() > 0) {
			asmPaths->Add(gcnew String(asmParent.c_str()));
		}

		/**
		 * Run the PluginLoader on the new appdomain
		 * We do this by using a Serializable,MarshalByRefObject class, which gets serialized to the new AppDomain and ran there
		 */
		array<Object^>^ args = gcnew array<Object^>(2);
		args[0] = gcnew String(asmPath.c_str());
		args[1] = asmPaths;

		AssemblyInstance^ pl = (AssemblyInstance^)newDomain->CreateInstanceFromAndUnwrap(
			thisAsmPath,
			(AssemblyInstance::typeid)->FullName, false,
			BindingFlags::Default,
			nullptr, args,
			nullptr, nullptr
		);

		gHandles.emplace(handle, AssemblyInstanceData(newDomain, pl));
		return handle;
	}

	__declspec(dllexport)
		extern bool __cdecl clrDeInit(PLGHANDLE handle) {
		AssemblyInstanceData data = gHandles[handle];

		// shouldn't be necessary since we're about to dispose the AppDomain
		RemotingServices::Disconnect(data.instance);
		AppDomain::Unload(data.appDomain);
		gHandles.erase(handle);
		return true;
	}
}