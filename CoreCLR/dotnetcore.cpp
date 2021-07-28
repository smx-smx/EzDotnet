/*
 * Copyright (C) 2021 Stefano Moioli <smxdev4@gmail.com>
 **/
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <map>
#include <filesystem>

#define DEBUG

#include <fcntl.h>

#include "coreclr_delegates.h"
#include "hostfxr.h"

#include "common/common.h"

using fx_string = std::basic_string<char_t>;

fx_string operator "" _toNativeString(const char *ptr, size_t size){
	std::string str(ptr, ptr + size);
	return str_conv<char_t>(str);
}

static fx_string to_native_fx_path(fx_string fx_path){
	std::string path = ::str_conv<char>(fx_path);
	std::string native_path = ::to_native_path(path);
	return ::str_conv<char_t>(native_path);
}
struct PluginInstance {
private:
	fx_string m_asmPath;
	load_assembly_and_get_function_pointer_fn m_loadAssembly;
public:

	PluginInstance(fx_string asmPath, load_assembly_and_get_function_pointer_fn pfnLoadAssembly)
		: m_asmPath(asmPath), m_loadAssembly(pfnLoadAssembly){}

	int runMethod(
		const char *typeName, const char *methodName,
		int argc, const char *argv[]
	) {
		fx_string targetMethodName = ::str_conv<char_t>(methodName);

		fx_string assemblyName = (
			std::filesystem::path(m_asmPath)
				.stem()
				.string<char_t>()
		);

		// HelloWorld.EntryPoint,HelloWorld <-- namespace.type, assembly
		fx_string targetClassName = (
			::str_conv<char_t>(typeName)
				+ ","_toNativeString
				+ assemblyName
		);

		component_entry_point_fn pfnEntry = nullptr;

		DPRINTF("Loading '%s', then running %s in %s\n",
			::str_conv<char>(m_asmPath).c_str(),
			::str_conv<char>(targetMethodName).c_str(),
			::str_conv<char>(targetClassName).c_str()
		);
		m_loadAssembly(
			::to_native_fx_path(m_asmPath).c_str(),
			targetClassName.c_str(),
			targetMethodName.c_str(),
			NULL, //-> public delegate int ComponentEntryPoint(IntPtr args, int sizeBytes);
			NULL,
			(void **)&pfnEntry);

		if (pfnEntry == nullptr) {
			DPRINTF("Failed to locate '%s:%s'\n",
				::str_conv<char>(targetClassName).c_str(),
				::str_conv<char>(targetMethodName).c_str());
			return -1;
		}
		
		pfnEntry(argv, argc * sizeof(char *));
		return 0;
	}
};

struct dotnet_init_params {
	const char *hostfxr_path;
	const char_t *runtimeconfig_path;
	const char_t *host_path;
	const char_t *dotnet_root;
};


static std::map<ASMHANDLE, PluginInstance> gPlugins;

static hostfxr_handle runtimeHandle = nullptr;
static load_assembly_and_get_function_pointer_fn pfnLoadAssembly = nullptr;
static hostfxr_close_fn pfnClose = nullptr;

static int initHostFxr(
	struct dotnet_init_params &initParams,
	hostfxr_initialize_for_runtime_config_fn pfnInitializer,
	hostfxr_get_runtime_delegate_fn pfnGetDelegate,
	load_assembly_and_get_function_pointer_fn *ppfnLoadAssembly
) {
	struct hostfxr_initialize_parameters hostfxr_params;
	hostfxr_params.size = sizeof(struct hostfxr_initialize_parameters);
	hostfxr_params.host_path = initParams.host_path;
	hostfxr_params.dotnet_root = initParams.dotnet_root;

	hostfxr_handle runtimeHandle = nullptr;
	load_assembly_and_get_function_pointer_fn pfnLoadAssembly = nullptr;

	pfnInitializer(
		::to_native_fx_path(initParams.runtimeconfig_path).c_str(),
		&hostfxr_params, &runtimeHandle
	);
	if (runtimeHandle == nullptr) {
		DPRINTF("Failed to initialize dotnet core\n");
		return -1;
	}

	pfnGetDelegate(runtimeHandle, hdt_load_assembly_and_get_function_pointer, (void **)&pfnLoadAssembly);
	if (pfnLoadAssembly == nullptr) {
		DPRINTF("Failed to acquire load_assembly_and_get_function_pointer_fn\n");
		return -2;
	}

	*ppfnLoadAssembly = pfnLoadAssembly;
	return 0;
}

static int loadAndInitHostFxr(
	dotnet_init_params& initParams,
	hostfxr_close_fn *ppfnClose,
	hostfxr_handle *pHandle,
	load_assembly_and_get_function_pointer_fn *pfnLoadAssembly
) {
	LIB_HANDLE hostfxr = LIB_OPEN(initParams.hostfxr_path);
	if (hostfxr == nullptr) {
		DPRINTF("dlopen '%s' failed\n", initParams.hostfxr_path);
		return -1;
	}

	hostfxr_initialize_for_runtime_config_fn pfnInitializer = nullptr;
	hostfxr_get_runtime_delegate_fn pfnGetDelegate = nullptr;

	pfnInitializer = (hostfxr_initialize_for_runtime_config_fn)LIB_GETSYM(hostfxr, "hostfxr_initialize_for_runtime_config");
	pfnGetDelegate = (hostfxr_get_runtime_delegate_fn)LIB_GETSYM(hostfxr, "hostfxr_get_runtime_delegate");
	*ppfnClose = (hostfxr_close_fn)LIB_GETSYM(hostfxr, "hostfxr_close");

	if (*pfnInitializer == nullptr || *pfnGetDelegate == nullptr || *ppfnClose == nullptr) {
		DPRINTF("failed to resolve libhostfxr symbols\n");
		return -2;
	}

	initHostFxr(initParams, pfnInitializer, pfnGetDelegate, pfnLoadAssembly);
	return 0;
}

extern "C" {
	DLLEXPORT const ASMHANDLE APICALL clrInit(
		const char *assemblyPath, const char *pluginFolder, bool enableDebug
	){
		DPRINTF("\n");

		#if defined(WIN32) || defined(__CYGWIN__)
		initCygwin();
		#endif

		std::filesystem::path asmPath(assemblyPath);	
		std::filesystem::path asmDir = asmPath.parent_path();
		
		std::string pluginName = asmPath.filename().replace_extension().string();
		ASMHANDLE handle = str_hash(pluginName.c_str());

		if (gPlugins.find(handle) != gPlugins.end()) {
			return handle;
		}

		if (::pfnLoadAssembly == nullptr) {
			std::filesystem::path hostFxrPath = asmDir / (
				std::string(LIB_PREFIX) + "hostfxr" + LIB_SUFFIX
			);

			hostfxr_initialize_for_runtime_config_fn pfnInitializer = nullptr;
			hostfxr_get_runtime_delegate_fn pfnGetDelegate = nullptr;

			std::string hostFxrPathStr = ::to_native_path(hostFxrPath.string());
			fx_string asmDirStr = asmDir.string<char_t>();

			// copy path before removing the extension
			std::filesystem::path asmBase(asmPath);
			asmBase.replace_extension();

			fx_string runtimeConfigPathStr = (
				asmBase.string<char_t>() + ".runtimeconfig.json"_toNativeString
			);

			dotnet_init_params initParams;
			initParams.hostfxr_path = hostFxrPathStr.c_str();
			initParams.host_path = asmDirStr.c_str();
			initParams.dotnet_root = asmDirStr.c_str();	
			initParams.runtimeconfig_path = runtimeConfigPathStr.c_str();

			if (loadAndInitHostFxr(initParams, &::pfnClose, &::runtimeHandle, &::pfnLoadAssembly) != 0) {
				return NULL_ASMHANDLE;
			}
		}

		gPlugins.emplace(handle, PluginInstance(asmPath.string<char_t>(), ::pfnLoadAssembly));
		return handle;
	}

	DLLEXPORT bool APICALL clrDeInit(ASMHANDLE handle) {
		if (::pfnClose == nullptr || ::runtimeHandle == nullptr) {
			return false;
		}
		::pfnClose(::runtimeHandle);
		return true;
	}

	DLLEXPORT int APICALL runMethod(
		ASMHANDLE handle,
		const char *typeName, const char *methodName,
		int argc, const char *argv[]
	) {
		DPRINTF("\n");
		return gPlugins.at(handle).runMethod(typeName, methodName, argc, argv);
	}
}