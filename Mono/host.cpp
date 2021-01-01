/**
 * Copyright (C) 2019 Stefano Moioli <smxdev4@gmail.com>
 */
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
#include <Windows.h>
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
public:
	MonoDomain *appDomain;

	int runMethod(const char *typeName, const char *methodName){
		MonoThread *thread = mono_thread_attach(appDomain);

		bool methodInvoked = false;

		std::string methodDesc = std::string(typeName) + "::" + std::string(methodName);

		void *pUserData[] = {
			(void *)methodDesc.c_str(),
			(void *)m_asmName,
			(void *)&methodInvoked
		};

		mono_assembly_foreach([](void *refAsmPtr, void *userdata) {
			void **pPointers = (void **)userdata;
			const char *monoMethodName = (const char *)pPointers[0];
			const char *m_asmName = (const char *)pPointers[1];
			bool *pMethodInvoked = (bool *)pPointers[2];

			if(*pMethodInvoked)
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

			MonoMethod *method = imageFindMethod(refAsmImage, monoMethodName);
			if(method == nullptr)
				return;

			MonoObject *exception = nullptr;
			void **args = nullptr;
			MonoObject *ret = mono_runtime_invoke(method, nullptr, args, &exception);

			*pMethodInvoked = true;

			if (exception) {
				mono_print_unhandled_exception(exception);
			}
		}, pUserData);

		mono_thread_detach(thread);
		return 0;
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

extern "C" {
	/*
	 * Initializes the Mono runtime
	 */
	DLLEXPORT const ASMHANDLE APICALL clrInit(
		const char *assemblyPath, const char *pluginFolder, bool enableDebug
	) {
		DPRINTF("\n");

		/**
		 * Get Paths
		 */
		std::filesystem::path asmPath(assemblyPath);
		std::string pluginName = asmPath.filename().replace_extension().string();

		ASMHANDLE handle = str_hash(pluginName.c_str());
		if (gPlugins.find(handle) != gPlugins.end()) {
			return handle;
		}

		DPRINTF("loading %s\n", assemblyPath);

		if (!gMonoInitialized) {
			DPRINTF("initializing mono\n");

			rootDomain = mono_jit_init_version("SharpInj", "v4.0");
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
		mono_domain_set_config(newDomain, pluginFolder, configPath.c_str());


		DPRINTF("loading assembly...\n");
		MonoAssembly *pluginAsm = mono_domain_assembly_open(newDomain, assemblyPath);
		if (!pluginAsm) {
			fprintf(stderr, "Failed to open assembly '%s'\n", assemblyPath);
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
	DLLEXPORT int APICALL runMethod(ASMHANDLE handle, const char *typeName, const char *methodName	) {
		DPRINTF("\n");
		return gPlugins.at(handle).runMethod(typeName, methodName);
	}
}
