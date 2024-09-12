/*
 * Copyright (C) 2021 Stefano Moioli <smxdev4@gmail.com>
 **/
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <map>
#include <filesystem>
#include <string>
#include <variant>
#include <optional>

#define DEBUG

#include <fcntl.h>

#include "config.h"
#include "coreclr_delegates.h"
#include "nethost.h"
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
	get_function_pointer_fn m_loadAssembly;
public:

	PluginInstance(fx_string asmPath, get_function_pointer_fn pfnLoadAssembly)
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
			targetClassName.c_str(),
			targetMethodName.c_str(),
			NULL, //-> public delegate int ComponentEntryPoint(IntPtr args, int sizeBytes);
			NULL, NULL,
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
	std::string hostfxr_path;
	fx_string host_path;
	std::optional<fx_string> dotnet_root;
	fx_string asm_base;
	fx_string asm_path;
};


static std::map<ASMHANDLE, PluginInstance> gPlugins;

static hostfxr_handle runtimeHandle = nullptr;
static get_function_pointer_fn pfnLoadAssembly = nullptr;
static hostfxr_close_fn pfnClose = nullptr;

static int initHostFxr(
	struct dotnet_init_params &initParams,
	hostfxr_initialize_for_dotnet_command_line_fn pfnInitializer,
	hostfxr_get_runtime_property_value_fn pfnGetRuntimeProperty,
	hostfxr_set_runtime_property_value_fn pfnSetRuntimeProperty,
	hostfxr_get_runtime_delegate_fn pfnGetDelegate,
	get_function_pointer_fn *ppfnLoadAssembly
) {
	bool hasRuntimeProperties = pfnGetRuntimeProperty && pfnSetRuntimeProperty;

	struct hostfxr_initialize_parameters hostfxr_params;
	hostfxr_params.size = sizeof(struct hostfxr_initialize_parameters);
	hostfxr_params.host_path = initParams.host_path.c_str();
	hostfxr_params.dotnet_root = initParams.dotnet_root
		? (*initParams.dotnet_root).c_str()
		: nullptr;

	hostfxr_handle runtimeHandle = nullptr;
	get_function_pointer_fn pfnLoadAssembly = nullptr;

	fx_string runtime_config_path = initParams.asm_base + ".runtimeconfig.json"_toNativeString;
	fx_string deps_file_path = initParams.asm_base + ".deps.json"_toNativeString;

	const char_t *argv[] = {
		initParams.asm_path.c_str(),
		nullptr
	};
	pfnInitializer(
		1, argv,
		&hostfxr_params, &runtimeHandle
	);
	if (runtimeHandle == nullptr) {
		DPRINTF("Failed to initialize dotnet core\n");
		return -1;
	}

	DPRINTF("host_path: %s\n", ::str_conv<char>(initParams.host_path).c_str());

#if false
	int result;
	if(hasRuntimeProperties){
		DPRINTF("APP_CONTEXT_BASE_DIRECTORY => %s\n", ::str_conv<char>(initParams.host_path).c_str());
		if((result=pfnSetRuntimeProperty(
			runtimeHandle,
			"APP_CONTEXT_BASE_DIRECTORY"_toNativeString.c_str(),
			initParams.host_path.c_str()
		)) != 0){
			DPRINTF("WARNING: Failed to set APP_CONTEXT_BASE_DIRECTORY\n");
		}
		if((result=pfnSetRuntimeProperty(
			runtimeHandle,
			"APP_PATHS"_toNativeString.c_str(),
			initParams.host_path.c_str()
		)) != 0){
			DPRINTF("WARNING: Failed to set APP_PATHS\n");
		}

		const char_t *APP_CONTEXT_DEPS_FILES = nullptr;
		if((result=pfnGetRuntimeProperty(
			runtimeHandle,
			"APP_CONTEXT_DEPS_FILES"_toNativeString.c_str(),
			&APP_CONTEXT_DEPS_FILES
		)) == 0){
			fx_string appContextDepsFiles = (
				fx_string(APP_CONTEXT_DEPS_FILES)
				+ ";"_toNativeString
				+ deps_file_path
			);
			DPRINTF("APP_CONTEXT_DEPS_FILES => %s\n", ::str_conv<char>(appContextDepsFiles).c_str());
			if((result=pfnSetRuntimeProperty(
				runtimeHandle,
				"APP_CONTEXT_DEPS_FILES"_toNativeString.c_str(),
				appContextDepsFiles.c_str()
			)) != 0){
				DPRINTF("WARNING: Failed to set APP_CONTEXT_DEPS_FILES\n");
			}
		} else {
			DPRINTF("WARNING: failed to get APP_CONTEXT_DEPS_FILES\n");
		}
	}
#endif

	pfnGetDelegate(runtimeHandle, hdt_get_function_pointer, (void **)&pfnLoadAssembly);
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
	get_function_pointer_fn *pfnLoadAssembly
) {
	LIB_HANDLE hostfxr = LIB_OPEN(initParams.hostfxr_path.c_str());
	if (hostfxr == nullptr) {
		DPRINTF("dlopen '%s' failed\n", initParams.hostfxr_path.c_str());
		return -1;
	}

	hostfxr_initialize_for_dotnet_command_line_fn pfnInitializer = nullptr;
	hostfxr_get_runtime_delegate_fn pfnGetDelegate = nullptr;
	hostfxr_get_runtime_property_value_fn pfnGetRuntimeProperty = nullptr;
	hostfxr_set_runtime_property_value_fn pfnSetRuntimeProperty = nullptr;

	/**
	 * @brief
	 * `hostfxr_initialize_for_dotnet_command_line`, despite the name, actually meanas "initialize for app", while
	 * `hostfxr_initialize_for_runtime_config` means "initialize for component" (library)
	 * We need to use the "app" API to support more complex assemblies which wouldn't otherwise work with the "library" API.
	 */
	pfnInitializer = (hostfxr_initialize_for_dotnet_command_line_fn)LIB_GETSYM(hostfxr, "hostfxr_initialize_for_dotnet_command_line");
	pfnGetRuntimeProperty = (hostfxr_get_runtime_property_value_fn)LIB_GETSYM(hostfxr, "hostfxr_get_runtime_property_value");
	pfnSetRuntimeProperty = (hostfxr_set_runtime_property_value_fn)LIB_GETSYM(hostfxr, "hostfxr_set_runtime_property_value");
	pfnGetDelegate = (hostfxr_get_runtime_delegate_fn)LIB_GETSYM(hostfxr, "hostfxr_get_runtime_delegate");
	*ppfnClose = (hostfxr_close_fn)LIB_GETSYM(hostfxr, "hostfxr_close");

	if (pfnInitializer == nullptr
		|| pfnGetDelegate == nullptr
		|| ppfnClose == nullptr
	) {
		DPRINTF("failed to resolve libhostfxr symbols\n");
		return -2;
	}

	initHostFxr(initParams, pfnInitializer, pfnGetRuntimeProperty, pfnSetRuntimeProperty, pfnGetDelegate, pfnLoadAssembly);
	return 0;
}

static std::variant<int, std::string> getHostFxrPath(std::filesystem::path& base_dir){
	size_t hostfxr_pathsz = 0;

	int (NETHOST_CALLTYPE *_get_hostfxr_path)(
		char_t * buffer, size_t * buffer_size,
		const struct get_hostfxr_parameters *parameters
	) = nullptr;

	std::string nethost_path = ::to_native_path((base_dir / FX_LIBRARY_NAME("nethost")).string());

#ifdef NETHOST_STATIC
	_get_hostfxr_path = &get_hostfxr_path;
#else
	LIB_HANDLE nethost = LIB_OPEN(nethost_path.c_str());
	if(nethost == nullptr){
		DPRINTF("failed to load nethost (path: '%s')\n", nethost_path.c_str());
		return -1;
	}
	lib_getsym(nethost, "get_hostfxr_path", _get_hostfxr_path);
	if(_get_hostfxr_path == nullptr){
		DPRINTF("failed to locate get_hostfxr_path");
		return -2;
	}
#endif

	if(_get_hostfxr_path(nullptr, &hostfxr_pathsz, nullptr) == 0 || hostfxr_pathsz == 0){
		DPRINTF("failed to get hostfxr path size\n");
		return -3;
	}
	fx_string buf(hostfxr_pathsz, '\0');
	if(_get_hostfxr_path(buf.data(), &hostfxr_pathsz, nullptr) != 0){
		DPRINTF("failed to locate hostfxr\n");
		return -3;
	}
	return ::str_conv<char>(buf);
}

extern "C" {
	DLLEXPORT const ASMHANDLE APICALL clrInit(
		const char *assemblyPath, const char *pluginFolder, bool enableDebug
	){
		DPRINTF("\n");

		#if defined(WIN32) || defined(__CYGWIN__)
		initCygwin();
		#endif

		ASMHANDLE handle = str_hash(assemblyPath);
		if (gPlugins.find(handle) != gPlugins.end()) {
			return handle;
		}

		std::filesystem::path asmPath(assemblyPath);	
		std::filesystem::path asmDir = asmPath.parent_path();
		std::string asmName = asmPath.filename().replace_extension().string();

		if (::pfnLoadAssembly == nullptr) {
			auto hostfxr_res = getHostFxrPath(asmDir);
			if(std::holds_alternative<int>(hostfxr_res)){
				DPRINTF("getHostFxrPath failed\n");
				return NULL_ASMHANDLE;
			}
			std::string hostFxrPath = std::get<std::string>(hostfxr_res);

			hostfxr_initialize_for_runtime_config_fn pfnInitializer = nullptr;
			hostfxr_get_runtime_delegate_fn pfnGetDelegate = nullptr;

			fx_string asmDirStr = ::to_native_fx_path(asmDir.string<char_t>());

			/**
			 * under cygwin, converting the following path to Windows:
			 * /cygdrive/c/something
			 *
			 * when the following file exists:
			 * /cygdrive/c/something.exe
			 *
			 * will result in C:\something.exe`, which defeats the point of 
			 * "getting the path to the filename without the extension"
			 *
			 * to workaround this, we'll use the dirname and append the stem
			 */
			fx_string nat_asmBase = ::to_native_fx_path(
				asmPath.parent_path().string<char_t>()
			)
			+ ::str_conv<char_t>(std::string(1, std::filesystem::path::preferred_separator))
			+ asmPath.stem().string<char_t>();

			dotnet_init_params initParams;
			initParams.hostfxr_path = ::to_native_path(hostFxrPath);
			initParams.host_path = asmDirStr;
			initParams.asm_base = nat_asmBase;
			initParams.asm_path = asmPath.string<char_t>();
			//$FIXME: provide a switch for self contained apps?
			//initParams.dotnet_root = asmDirStr.c_str();
			initParams.dotnet_root = {};

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