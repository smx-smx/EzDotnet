/*
 * Copyright (C) 2026 Stefano Moioli <smxdev4@gmail.com>
 **/
extern "C" {
#include <mono/jit/mono-private-unstable.h>
#include <mono/jit/jit.h>
#include <mono/metadata/assembly.h>
#include <mono/metadata/class.h>
#include <mono/metadata/threads.h>
}

#include <cstdio>
#include <cstdlib>
#include <cassert>

#include <nlohmann/json.hpp>
#include <filesystem>
#include <fstream>
#include <vector>
#include <algorithm>

#ifdef _WIN32
#define PATH_SEP_CHAR ';'
#else
#define PATH_SEP_CHAR ':'
#endif

#define DEBUG
#include "common/common.h"

namespace util
{
	template< typename... Args >
	std::string ssprintf(const char *format, Args... args) {
		int length = std::snprintf(nullptr, 0, format, args...);
		assert(length >= 0);

		std::vector<char> buf(length + 1);
		std::snprintf(buf.data(), length + 1, format, args...);

		std::string str(buf.data());
		return str;
	}

    template <typename T>
    std::basic_string<T> lowercase(const std::basic_string<T>& s)
    {
        std::basic_string<T> s2 = s;
        std::transform(s2.begin(), s2.end(), s2.begin(),
                       [](const T v){ return static_cast<T>(std::tolower(v)); });
        return s2;
    }
}

/* This is a magic number that must be passed to mono_jit_init_version */
#define FRAMEWORK_VERSION "v4.0.30319"


template<typename T>
std::optional<T> json_get_opt(const nlohmann::json& j, const std::string& key) {
	if(j.contains(key)){
		return j.at(key);
	}
	return std::nullopt;
}

static nlohmann::json load_deps_json(const char *depsFile){
	std::ostringstream tpa;

	std::ifstream file(depsFile);
	if(!file.is_open()){
		throw std::runtime_error(util::ssprintf("Cannot open %s for reading", depsFile));
	}


	nlohmann::json deps{};
	try {
		file >> deps;
	} catch(const nlohmann::json::parse_error& e){
		printf("failed to parse %s\n", depsFile);
	}
	file.close();

    return deps;
}

struct ctx {
    std::string main_asm_name;
    std::string main_asm_fqdn;
    std::string tpa_list;
};

#if 0
// GCC-way, incompatible with MSVC
#define GET_BREAK(t, p, n) ({ \
        auto v = json_get_opt<t>(p, n); if(!v.has_value()) break; \
    	v.value(); })

#define GET_CONT(t, p, n) ({ \
        auto v = json_get_opt<t>(p, n); if(!v.has_value()) continue; \
    	v.value(); })
#endif

#define GET_BREAK(v, t, p, n) \
		auto __opt_##v = json_get_opt<t>(p, n); \
		if(!__opt_##v.has_value()) break; \
		t v = __opt_##v.value()

#define GET_CONT(v, t, p, n) \
		auto __opt_##v = json_get_opt<t>(p, n); \
		if(!__opt_##v.has_value()) continue; \
		t v = __opt_##v.value()

#define GETJ_B(v, p, n) GET_BREAK(v, nlohmann::json, p, n)
#define GETS_B(v, p, n) GET_BREAK(v, std::string, p, n)
#define GETJ_C(v, p, n) GET_CONT(v, nlohmann::json, p, n)
#define GETS_C(v, p, n) GET_CONT(v, std::string, p, n)

#define GETJ_B(v, p, n) GET_BREAK(v, nlohmann::json, p, n)
#define GETS_B(v, p, n) GET_BREAK(v, std::string, p, n)
#define GETJ_C(v, p, n) GET_CONT(v, nlohmann::json, p, n)
#define GETS_C(v, p, n) GET_CONT(v, std::string, p, n)

static struct ctx build_tpa_list(const char *depsFile, const nlohmann::json &deps){
    std::filesystem::path depsFs(depsFile);
    auto deps_dir = depsFs.parent_path();

	std::ostringstream tpa;
	bool tpa_first = true;
	auto tpa_append = [&](const std::string& itm){
		if(tpa_first){
			tpa_first = false;
		} else {
#ifdef _WIN32
			tpa << ';';
#else
			tpa << ':';
#endif
		}
		tpa << itm;
	};

    struct ctx result;
	do {
		// get the framework name and RID
		GETJ_B(runtimeTarget, deps, "runtimeTarget");
		GETS_B(runtimeTarget_name, runtimeTarget, "name");

		// get the targets exported by the framework
		GETJ_B(all_targets, deps, "targets");
		GETJ_B(framework_target, all_targets, runtimeTarget_name);

		std::optional<std::pair<std::string, nlohmann::json>> project_node = std::nullopt;

		// get all libraries contained in this bundle
		GETJ_B(libraries, deps, "libraries");

		// look for a "project" item
		for(const auto& itm : libraries.items()){
			auto lib_name = itm.key();
			auto lib_item = itm.value();
			GETS_C(lib_type, lib_item, "type");
			if(!project_node.has_value() && lib_type == "project"){
				project_node = {lib_name, lib_item};
				break;
			}
		}
        result.main_asm_name = project_node->first;

		if(!project_node.has_value()) break;

		// get the project node
		GETJ_B(project_target, framework_target, project_node->first);

        auto slash = project_node->first.find('/');
        if(slash == std::string::npos){
            // no version specifier
            break;
        }
        result.main_asm_fqdn = util::ssprintf("%s, Version=%s",
                                       project_node->first.substr(0, slash).c_str(),
                                       project_node->first.substr(slash + 1).c_str());


        GETJ_B(project_rt, project_target, "runtime");
        for(const auto& itm : project_rt.items()) {
            const auto& lib_name = itm.key();
            if(util::lowercase(lib_name).ends_with(".dll")) {
                tpa_append((deps_dir / lib_name).string());
            }
        }

		// process the project dependencies
		GETJ_B(project_deps, project_target, "dependencies");
		for(const auto& itm : project_deps.items()){
			const auto& item_name = itm.key();
			std::string item_version = itm.value();

			auto item_key = util::ssprintf("%s/%s",
										   item_name.c_str(),
										   item_version.c_str());

			GETJ_C(target_item, framework_target, item_key);
			auto target_runtime = json_get_opt<nlohmann::json>(target_item, "runtime");
			auto target_native = json_get_opt<nlohmann::json>(target_item, "native");

			if(target_runtime.has_value()){
				for(const auto& lib : target_runtime.value().items()){
					const auto& lib_name = lib.key();
					if(util::lowercase(lib_name).ends_with(".dll")) {
						tpa_append((deps_dir / lib_name).string());
					}
				}
			}
			if(target_native.has_value()){
                for(const auto& lib : target_native.value().items()) {
                    const auto& lib_name = lib.key();
                    if (util::lowercase(lib_name).ends_with(".dll")) {
                        tpa_append((deps_dir / lib_name).string());
                    }
                }
            }
		}
	} while(false);

    result.tpa_list = tpa.str();
    return result;
}

static MonoAssembly *load_asm_by_name(const char *asmName){
    MonoAssemblyName *aname = mono_assembly_name_new (asmName);
    if (!aname) {
        printf ("Couldn't parse assembly name '%s'\n", asmName);
        return NULL;
    }
    MonoImageOpenStatus status;
    MonoAssembly *assembly = mono_assembly_load_full (aname, /*basedir*/ NULL, &status, 0);
    if (!assembly || status != MONO_IMAGE_OK) {
        printf ("Couldn't open \"%s\", (status=0x%08x)\n", asmName, status);
        mono_assembly_name_free (aname);
        return NULL;
    }
    mono_assembly_name_free (aname);
    return assembly;
}

static MonoAssembly *load_asm_by_path(const char *asmPath){
    MonoImageOpenStatus status;
    MonoAssembly *assembly = mono_assembly_open_full(asmPath, &status, false);
    if (!assembly || status != MONO_IMAGE_OK) {
        printf ("Couldn't open \"%s\", (status=0x%08x)\n", asmPath, status);
        return NULL;
    }
    return assembly;
}

struct AsmInstanceData {
private:
	MonoImage *m_image;
public:
	AsmInstanceData(MonoImage *image)
		: m_image(image){}

	int runMethod(
		const char *typeName, const char *methodName,
		int argc, const char *argv[]
	) {
		std::string typeName_owned(typeName);
		int lastdot = typeName_owned.find_last_of('.');

		std::string type_ns = typeName_owned.substr(0, lastdot);
		std::string type_cls = typeName_owned.substr(lastdot + 1);

		MonoThread *thread = mono_thread_attach (mono_get_root_domain ());
		MonoClass *kls = mono_class_from_name (m_image, type_ns.c_str(), type_cls.c_str());
		if (!kls) {
			fprintf (stderr, "Couldn't find type \"%s\"", typeName);
			return -1;
		}

		MonoMethod *method = mono_class_get_method_from_name (kls, methodName, 2);
		if (!method) {
			fprintf (stderr, "Couldn't find method \"%s\"\n", methodName);
			return -1;
		}

		MonoObject *exception = nullptr;

		void *argsPtr = argv;
		int argsSizeInBytes = argc * sizeof(char *);

		void *args[2] = {
			&argsPtr,
			&argsSizeInBytes
		};
		MonoObject *ret = mono_runtime_invoke(method, nullptr, args, &exception);

		int rc = exception != nullptr ? -1 : 0;

		if (exception != nullptr) {
			mono_print_unhandled_exception(exception);
		}

		// $FIXME: mono_thread_detach Cannot transition thread 000067B4 from STATE_BLOCKING with DO_BLOCKING
#if 0
		// special case: main thread is exiting
		if (mono_thread_detach_if_exiting()) {
			return rc;
		}

		// otherwise: detach the current thread
		thread = mono_thread_current();
		if (thread) {
			mono_thread_detach (thread);
		}
#endif
		return rc;
	}
};

static std::map<ASMHANDLE, AsmInstanceData> gAssemblies;

static std::string getDepsJsonPath(const std::string& inputPath) {
	std::filesystem::path path(inputPath);

	std::string ext = path.extension().string();
	std::transform(ext.begin(), ext.end(), ext.begin(),
		[](unsigned char c){ return std::tolower(c); });

	if (ext == ".json") {
		return path.string();
	}
	return (path.parent_path() / path.stem()).string() + ".deps.json";
}

ASMHANDLE initialize(const std::string& inputPath, const std::string& asmDirStr) {
	std::string depsPathStr = getDepsJsonPath(inputPath);

	ASMHANDLE handle = str_hash(depsPathStr.c_str());
	if (gAssemblies.find(handle) != gAssemblies.end()) {
		return handle;
	}

	DPRINTF("loading deps JSON: %s\n", depsPathStr.c_str());

	nlohmann::json deps = load_deps_json(depsPathStr.c_str());
	auto tpa_list = build_tpa_list(depsPathStr.c_str(), deps);

	const char *prop_keys[] = {"TRUSTED_PLATFORM_ASSEMBLIES"};
	const char *prop_values[] = {tpa_list.tpa_list.c_str()};
	int nprops = sizeof(prop_keys)/sizeof(prop_keys[0]);

	monovm_initialize (nprops, (const char**) &prop_keys, (const char**) &prop_values);
	MonoDomain *root_domain = mono_jit_init_version ("SharpInj", FRAMEWORK_VERSION);

	if (!root_domain) {
		fprintf(stderr, "root domain was null, expected non-NULL on success\n");
		return NULL_ASMHANDLE;
	}

	DPRINTF("loading asm: %s\n", tpa_list.main_asm_fqdn.c_str());
	MonoAssembly *assembly = load_asm_by_name(tpa_list.main_asm_fqdn.c_str());
	if (!assembly) {
		fprintf(stderr, "failed to obtain the main assembly\n");
		return NULL_ASMHANDLE;
	}

	MonoImage *img = mono_assembly_get_image (assembly);
	if (!img) {
		fprintf(stderr, "failed to obtain the main image\n");
		return NULL_ASMHANDLE;
	}

	gAssemblies.emplace(handle, AsmInstanceData(img));
	return handle;
}

extern "C" {
	/*
	 * Initializes the Mono runtime
	 */
	DLLEXPORT const ASMHANDLE APICALL clrInit(
		const char *assemblyPath, const char *assemblyDir, bool enableDebug
	) {
		DPRINTF("\n");

		#if defined(WIN32) || defined(__CYGWIN__)
		initCygwin();
		#endif

		std::string asmPath = ::to_native_path(std::string(assemblyPath));
		std::string asmDir = ::to_native_path(std::string(assemblyDir));
		return initialize(asmPath, asmDir);
	}

	/*
	 * Unloads the app domain with all the assemblies it contains
	 */
	DLLEXPORT bool APICALL clrDeInit(ASMHANDLE handle) {
		DPRINTF("\n");
		DPRINTF("FIXME: not implemented");
		return false;
	}

	DLLEXPORT int APICALL runMethod(
		ASMHANDLE handle,
		const char *typeName, const char *methodName,
		int argc, const char *argv[]
	) {
		DPRINTF("\n");
		return gAssemblies.at(handle).runMethod(typeName, methodName, argc, argv);
	}
}
