#define _GNU_SOURCE
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <map>
#include <filesystem>

#define DEBUG

#ifdef WIN32
#include <io.h>
#include <Windows.h>
#else
#include <dlfcn.h>
#endif

#include <fcntl.h>

#include "coreclr_delegates.h"
#include "hostfxr.h"

#include "common/common.h"

template<typename TTo, typename TFrom>
static std::basic_string<TTo> str_conv(std::basic_string<TFrom> str){
	size_t length = str.length();
	const TFrom* data = str.data();
	return std::basic_string<TTo>(data, data + length);
}

template <typename TTo>
static std::basic_string<TTo> str_conv(const char *str){
	return str_conv<TTo, char>(std::string(str));
}

struct PluginInstance {
private:
	std::basic_string<char_t> m_asmPath;
	load_assembly_and_get_function_pointer_fn m_loadAssembly;
public:

	PluginInstance(std::basic_string<char_t> asmPath, load_assembly_and_get_function_pointer_fn pfnLoadAssembly)
		: m_asmPath(asmPath), m_loadAssembly(pfnLoadAssembly){}

	int runMethod(const char *typeName, const char *methodName) {
		std::basic_string<char_t> targetMethodName = ::str_conv<char_t>(methodName);

		std::basic_string<char_t> assemblyName = std::filesystem::path(m_asmPath)
													.filename()
													.replace_extension()
													.string<char_t>();

		// HelloWorld.EntryPoint,HelloWorld <-- namespace.type, assembly
		std::basic_string<char_t> targetClassName = ::str_conv<char_t>(typeName)
												+ ::str_conv<char_t>(",")
												+ assemblyName;

		component_entry_point_fn pfnEntry = nullptr;

		DPRINTF("Loading '%s', then running %s in %s\n",
			::str_conv<char>(m_asmPath).c_str(),
			::str_conv<char>(targetMethodName).c_str(),
			::str_conv<char>(targetClassName).c_str()
		);
		m_loadAssembly(
			m_asmPath.c_str(),
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
		
		pfnEntry(NULL, 0);
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

	pfnInitializer(initParams.runtimeconfig_path, &hostfxr_params, &runtimeHandle);
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

			std::string hostFxrPathStr = hostFxrPath.string();
			std::basic_string<char_t> asmDirStr = asmDir.string<char_t>();

			// copy path before removing the extension
			std::filesystem::path asmBase(asmPath);
			asmBase.replace_extension();

			std::basic_string<char_t> runtimeConfigPathStr = (
				asmBase.string<char_t>() + ::str_conv<char_t>(".runtimeconfig.json")
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

	DLLEXPORT int APICALL runMethod(ASMHANDLE handle, const char *typeName, const char *methodName) {
		DPRINTF("\n");
		return gPlugins.at(handle).runMethod(typeName, methodName);
	}
}