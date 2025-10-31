/*
 * Copyright (C) 2021 Stefano Moioli <smxdev4@gmail.com>
 **/
#include <cstdio>
#include <cstdbool>
#include <mono/jit/jit.h>
#include <mono/metadata/mono-config.h>
#include <mono/metadata/assembly.h>
#include <mono/metadata/debug-helpers.h>
#include <mono/metadata/threads.h>
#include <mono/metadata/mono-gc.h>
#include <mono/utils/mono-error.h>

#include <cstring>
#include <iostream>
#include <map>
#include <filesystem>

#define DEBUG

#ifdef _WIN32
#include <sstream>
#include <windows.h>
#endif

#include "common/common.h"

static MonoMethod *imageFindMethod(MonoImage *image, const char *methodName) {
	MonoMethodDesc *desc = mono_method_desc_new(methodName, false);
	if (desc == nullptr) {
		DPRINTF("Invalid method name '%s'\n", methodName);
		return nullptr;
	}
	MonoMethod *method = mono_method_desc_search_in_image(desc, image);
	mono_method_desc_free(desc);
	if (method == nullptr) {
		return nullptr;
	}
	return method;
}

struct PluginInstanceData {
	PluginInstanceData(){}
private:
	const char *m_asmName;

	MonoMethod *findMethod(const char *methodDesc){
		MonoMethod *method = nullptr;

		void *pUserData[] = {
			(void *)methodDesc,
			(void *)m_asmName,
			(void *)&method
		};

		mono_assembly_foreach([](void *refAsmPtr, void *userdata) {
			void **pPointers = (void **)userdata;
			const char *monoMethodName = (const char *)pPointers[0];
			const char *m_asmName = (const char *)pPointers[1];
			MonoMethod **ppMethod = (MonoMethod **)pPointers[2];

			if(*ppMethod != nullptr)
				return;

			MonoAssembly *refAsm = (MonoAssembly *)refAsmPtr;
			const char *asmName = mono_assembly_name_get_name(mono_assembly_get_name(refAsm));
			if(std::strcmp(asmName, m_asmName) != 0)
				return;

			MonoImage *refAsmImage = mono_assembly_get_image(refAsm);
			if (refAsmImage == nullptr) {
				fprintf(stderr, "Cannot get image for assembly '%s'\n", asmName);
				return;
			}

			*ppMethod = imageFindMethod(refAsmImage, monoMethodName);
		}, pUserData);

		return method;
	}

public:
	MonoDomain *appDomain;

	int runMethod(
		const char *typeName, const char *methodName,
		int argc, const char *argv[]
	){
		MonoThread *thread = mono_thread_attach(appDomain);
		bool methodInvoked = false;

		std::string methodDesc = std::string(typeName) + "::" + std::string(methodName);

		MonoMethod *method = findMethod(methodDesc.c_str());

		int rc = -1;
		if(method == nullptr){
			return rc;
		}

		MonoObject *exception = nullptr;

		void *argsPtr = argv;
		int argsSizeInBytes = argc * sizeof(char *);

		void *args[2] = {
			&argsPtr,
			&argsSizeInBytes
		};
		MonoObject *ret = mono_runtime_invoke(method, nullptr, args, &exception);

		if (exception != nullptr) {
			mono_print_unhandled_exception(exception);
		} else {
			rc = 0;
		}

		mono_thread_detach(thread);
		return rc;
	}

	PluginInstanceData(MonoDomain *pAppDomain, MonoAssembly *pMonoAsm) :
		appDomain(pAppDomain),
		m_asmName(mono_assembly_name_get_name(mono_assembly_get_name(pMonoAsm)))
	{
	}
};

static std::map<ASMHANDLE, PluginInstanceData> gPlugins;

static MonoDomain *rootDomain = nullptr;
static bool gMonoInitialized = false;

ASMHANDLE initialize(const std::string& asmPathStr, const std::string& asmDirStr){
	/**
	 * Get Paths
	 */
	std::filesystem::path asmPath = asmPathStr;
	std::string pluginName = asmPath.filename().replace_extension().string();

	ASMHANDLE handle = str_hash(pluginName.c_str());
	if (gPlugins.find(handle) != gPlugins.end()) {
		return handle;
	}

	DPRINTF("loading %s\n", asmPath.c_str());

	if (!gMonoInitialized) {
		DPRINTF("initializing mono\n");

		rootDomain = mono_jit_init_version("SharpInj", "v4.0.30319");
		if (!rootDomain) {
			fprintf(stderr, "Failed to initialize mono\n");
			return NULL_ASMHANDLE;
		}
		// Load the default mono configuration file
		mono_config_parse(nullptr);

		gMonoInitialized = true;
		DPRINTF("mono initialization completed\n");
	}

	/**
	 * Create AppDomain
	 */
	DPRINTF("creating appdomain...\n");
	MonoDomain *newDomain = mono_domain_create_appdomain(const_cast<char *>(pluginName.c_str()), nullptr);
	std::string configPath = asmPath.replace_extension().string() + ".config";
	mono_domain_set_config(newDomain, asmDirStr.c_str(), configPath.c_str());


	DPRINTF("loading assembly...\n");
	MonoAssembly *pluginAsm = mono_domain_assembly_open(newDomain, asmPathStr.c_str());
	if (!pluginAsm) {
		fprintf(stderr, "Failed to open assembly '%s'\n", asmPathStr.c_str());
		return NULL_ASMHANDLE;
	}

	MonoImage *image = mono_assembly_get_image(pluginAsm);
	if (!image) {
		fprintf(stderr, "Failed to get image\n");
		return NULL_ASMHANDLE;
	}

	// NOTE: can't use mono_assembly_load_references (it's deprecated and does nothing)

	int numAssemblies = mono_image_get_table_rows(image, MONO_TABLE_ASSEMBLYREF);
	for (int i = 0; i < numAssemblies; i++) {
		mono_assembly_load_reference(image, i);
	}

	gPlugins.emplace(handle, PluginInstanceData(newDomain, pluginAsm));
	return handle;
}

extern "C" {
	/*
	 * Initializes the Mono runtime
	 */
	DLLEXPORT const ASMHANDLE APICALL clrInit(
		const char *assemblyPath, const char *pluginFolder, bool enableDebug
	) {
		DPRINTF("\n");

		#if defined(WIN32) || defined(__CYGWIN__)
		initCygwin();
		#endif

		std::string asmPath = ::to_native_path(std::string(assemblyPath));
		std::string asmDir = ::to_native_path(std::string(pluginFolder));
		return initialize(asmPath, asmDir);
	}

	/*
	 * Unloads the app domain with all the assemblies it contains
	 */
	DLLEXPORT bool APICALL clrDeInit(ASMHANDLE handle) {
		DPRINTF("\n");

		PluginInstanceData data = gPlugins[handle];
		mono_domain_unload(data.appDomain);

		gPlugins.erase(handle);
		return true;
	}

	///////////// Plugin Interface
	/*
	 * These are C bindings to C# methods.
	 * Calling any of the methods below will call the respective C# method in the loaded assembly
	 */
	DLLEXPORT int APICALL runMethod(
		ASMHANDLE handle,
		const char *typeName, const char *methodName,
		int argc, const char *argv[]
	) {
		DPRINTF("\n");
		return gPlugins.at(handle).runMethod(typeName, methodName, argc, argv);
	}
}
