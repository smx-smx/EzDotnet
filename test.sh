#!/bin/bash
normal='printf \e[0m'
bold='setterm -bold'

red='printf \033[00;31m'
green='printf \033[00;32m'
yellow='printf \033[00;33m'
blue='printf \033[00;34m'
purple='printf \033[00;35m'
cyan='printf \033[00;36m'
lightgray='printf \033[00;37m'
lred='printf \033[01;31m'
lgreen='printf \033[01;32m'
lyellow='printf \033[01;33m'
lblue='printf \033[01;34m'
lpurple='printf \033[01;35m'
lcyan='printf \033[01;36m'
white='printf \033[01;37m'

EZDOTNET_CYGWIN="$PWD/build_cygwin/samples/cli/ezdotnet.exe"

HOST_WIN32_CLR="$PWD/build_win32/CLRHost/Debug/CLRHost.dll"
HOST_CYGWIN_CORECLR="$PWD/build_cygwin/CoreCLR/cygcoreclrhost.dll"
HOST_MINGW_MONO="$PWD/build_mingw/Mono/libMonoHost.dll"

SAMPLE_CSPROJ="$PWD/samples/Managed/Cygwin/Cygwin.csproj"
SAMPLE_NETFWK="$PWD/samples/Managed/Cygwin/bin/Debug/net472/Cygwin.exe"
SAMPLE_NETCORE="$PWD/samples/Managed/Cygwin/bin/Debug/net5.0/Cygwin.dll"

build_cygwin(){
	cmake -B build_cygwin -G "Unix Makefiles" && \
	cmake --build build_cygwin
}

build_win32(){
	cmd /C \
		set PATH= \& \
		call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat" \& \
		cmake -B build_win32 -G "Visual Studio 16 2019" -A "x64" \& \
		cmake --build build_win32
}

build_mingw64(){
	inner=""
	inner="${inner} cmake -B build_mingw -G 'MSYS Makefiles' &&"
	inner="${inner} cmake --build build_mingw"
	cmd /C "C:\msys64\usr\bin\bash" -c "export PATH=/mingw64/bin:/usr/bin; (${inner})"
}

build_sample(){
	dotnet build "$(cygpath -w "${SAMPLE_CSPROJ}")"
}

build(){
	$lcyan; echo "[BUILD] win32"; $normal
	build_win32
	$lcyan; echo "[BUILD] mingw64"; $normal
	build_mingw64
	$lcyan; echo "[BUILD] cygwin"; $normal
	build_cygwin
	$lcyan; echo "[BUILD] sample"; $normal
	build_sample
}

build_parallel(){
	build_win32 &
	build_mingw64 &
	build_cygwin &
	build_sample &
	wait
}

test(){
	$lcyan; echo "[TEST] Running CLRHost (win32)"; $normal
	${EZDOTNET_CYGWIN} ${HOST_WIN32_CLR} ${SAMPLE_NETFWK} \
		ManagedSample.EntryPoint Entry \
		arg1 arg2 arg3 arg4 arg5

	$lcyan; echo "[TEST] Running MonoHost (win32/mingw64)"; $normal
	PATH="/cygdrive/c/msys64/mingw64/bin:$PATH" ${EZDOTNET_CYGWIN} ${HOST_MINGW_MONO} ${SAMPLE_NETFWK} \
		ManagedSample.EntryPoint Entry \
		arg1 arg2 arg3 arg4 arg5

	$lcyan; echo "[TEST] Running CoreCLR (cygwin)"; $normal
	${EZDOTNET_CYGWIN} ${HOST_CYGWIN_CORECLR} ${SAMPLE_NETCORE} \
		ManagedSample.EntryPoint Entry \
		arg1 arg2 arg3 arg4 arg5
}

#build_parallel
build
test