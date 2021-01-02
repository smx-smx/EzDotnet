#pragma once

#ifdef __cplusplus
#include <string>
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

std::string to_native_path(const std::string& path);
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __i386__
  #define APICALL __attribute__((__cdecl__))
#else
  #define APICALL
#endif

#ifdef DEBUG
#define DPRINTF(fmt, ...) \
	fprintf(stderr, "[%s]: " fmt, __func__, ##__VA_ARGS__)
#else
#define DPRINTF(fmt, ...)
#endif

#define NULL_ASMHANDLE 0
typedef size_t ASMHANDLE;
size_t str_hash(const char *str);
char *to_native_path(const char *path);
char *to_windows_path(const char *path);

#if defined(WIN32) || defined(__CYGWIN__)
#include "common_win32.h"
#else
#include "common_unix.h"
#endif

#ifdef __cplusplus
}
#endif