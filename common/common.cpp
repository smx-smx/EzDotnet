#include <cstring>
#include <cstddef>
#include <cstdint>
#include <string>

#include "common.h"

size_t str_hash(const char *str){
	unsigned long hash = 5381;
	int c;

	while (c = *str++)
		hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

	return hash;
}

char *to_native_path(const char *path){
	std::string str(path);
	std::string nativePath = ::to_native_path(str);
	return strdup(nativePath.c_str());
}