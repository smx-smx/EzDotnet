<?php
/**
 * Copyright(C) 2021 Stefano Moioli <smxdev4@gmail.com>
 */
abstract class CommandMode {
	public const WIN32_32 = 0;
	public const WIN32_64 = 1;
	public const MINGW32 = 2;
	public const MINGW64 = 3;
	public const CYGWIN = 4;
}

abstract class LdrType {
	public const CLRHost = 0;
	public const MonoHost = 1;
	public const CoreCLRHost = 2;
}

function run_cmd(string $cmd, bool $return = false){
	print("> {$cmd}\n");
	if($return){
		return shell_exec($cmd);
	} else {
		passthru($cmd);
	}
}

function win32_64_cmd(string $cmd, $return = false){
	$oldPath = getenv("PATH");
	$newPath = "C:\\msys64\\mingw64\\bin;{$oldPath}";
	putenv("PATH={$newPath}");
	$result = run_cmd($cmd, $return);
	putenv("PATH={$oldPath}");
	return $result;
}

function win32_32_cmd(string $cmd, $return = false){
	$oldPath = getenv("PATH");
	$newPath = "C:\\msys64\\mingw32\\bin;{$oldPath}";
	putenv("PATH={$newPath}");
	$result = run_cmd($cmd, $return);
	putenv("PATH={$oldPath}");
	return $result;
}

function cygwin_cmd(string $cmd, $return = false){
	$inner = escapeshellarg(
		"pushd . > /dev/null;"
		. ". /etc/profile > /dev/null;"
		. "popd > /dev/null;"
		. $cmd
	);
	$final = "C:/cygwin64/bin/bash -c {$inner}";
	return run_cmd($final, $return);
}

function mingw32_cmd(string $cmd, $return = false){
	$inner = escapeshellarg("export PATH=/mingw32/bin:/usr/bin; {$cmd}");
	$final = "C:/msys64/usr/bin/bash -c {$inner}";
	return run_cmd($final, $return);
}

function mingw64_cmd(string $cmd, $return = false){
	$inner = escapeshellarg("export PATH=/mingw64/bin:/usr/bin; {$cmd}");
	$final = "C:/msys64/usr/bin/bash -c {$inner}";
	return run_cmd($final, $return);
}

function cmd($mode, ...$cmd){
	$str = implode(' ', $cmd);
	$str = preg_replace("/\r?\n/", '', $str);
	switch($mode){
		case CommandMode::WIN32_32: return win32_32_cmd($str);
		case CommandMode::WIN32_64: return win32_64_cmd($str);
		case CommandMode::MINGW32: return mingw32_cmd($str);
		case CommandMode::MINGW64: return mingw64_cmd($str);
		case CommandMode::CYGWIN: return cygwin_cmd($str);
	}
	return null;
}

function path_concat(...$parts){ return implode(DIRECTORY_SEPARATOR, $parts); }

function to_msys_path(string $path){
	$argPath = escapeshellarg(str_replace('\\', '\\\\\\', $path));
	return rtrim(mingw64_cmd("cygpath -u {$argPath}", true));
}

function to_cygwin_path(string $path){
	$argPath = escapeshellarg(str_replace('\\', '\\\\\\', $path));
	return rtrim(cygwin_cmd("cygpath -u {$argPath}", true));
}

function convert_path($mode, string $path){
	switch($mode){
		case CommandMode::CYGWIN: return to_cygwin_path($path);
		case CommandMode::MINGW32:
		case CommandMode::MINGW64:
			return to_msys_path($path);
		case CommandMode::WIN32_32:
		case CommandMode::WIN32_64:
			return str_replace('/', '\\', $path);
	}
	return $path;
}

$buildCommands = array(
	CommandMode::WIN32_32 => <<<EOC
	call "C:\\Program Files (x86)\\Microsoft Visual Studio\\2019\\Community\\VC\\Auxiliary\\Build\\vcvars32.bat" &
	cmake -B build_win32 -G "Visual Studio 16 2019" -A "Win32" &
	cmake --build build_win32
	EOC,
	CommandMode::WIN32_64 => <<<EOC
	call "C:\\Program Files (x86)\\Microsoft Visual Studio\\2019\\Community\\VC\\Auxiliary\\Build\\vcvars64.bat" &
	cmake -B build_win64 -G "Visual Studio 16 2019" -A "x64" &
	cmake --build build_win64
	EOC,
	CommandMode::MINGW32 => <<<EOC
	cmake -B build_mingw32 -G 'MSYS Makefiles' && 
	cmake --build build_mingw32
	EOC,
	CommandMode::MINGW64 => <<<EOC
	cmake -B build_mingw64 -G 'MSYS Makefiles' && 
	cmake --build build_mingw64
	EOC,
	CommandMode::CYGWIN => <<<EOC
	cmake -B build_cygwin -G 'Unix Makefiles' && 
	cmake --build build_cygwin
	EOC
);

$cliPaths = array(
	CommandMode::WIN32_32 => __DIR__ . "/build_win32/samples/cli/Debug/ezdotnet.exe",
	CommandMode::WIN32_64 => __DIR__ . "/build_win64/samples/cli/Debug/ezdotnet.exe",
	CommandMode::MINGW32 => __DIR__ . "/build_mingw32/samples/cli/ezdotnet.exe",
	CommandMode::MINGW64 => __DIR__ . "/build_mingw64/samples/cli/ezdotnet.exe",
	CommandMode::CYGWIN => __DIR__ . "/build_cygwin/samples/cli/ezdotnet.exe"
);

$clrHostPaths = array(
	CommandMode::WIN32_32 => __DIR__ . "/build_win32/CLRHost/Debug/CLRHost.dll",
	CommandMode::WIN32_64 => __DIR__ . "/build_win64/CLRHost/Debug/CLRHost.dll",
);
$monoHostPaths = array(
	CommandMode::MINGW32 => __DIR__ . "/build_mingw32/Mono/libMonoHost.dll",
	CommandMode::MINGW64 => __DIR__ . "/build_mingw64/Mono/libMonoHost.dll" 
);
$coreClrHostPaths = array(
	CommandMode::WIN32_32 => __DIR__ . "/build_win32/CoreCLR/Debug/coreclrhost.dll",
	CommandMode::WIN32_64 => __DIR__ . "/build_win64/CoreCLR/Debug/coreclrhost.dll",
	CommandMode::MINGW32 => __DIR__ . "/build_mingw32/CoreCLR/libcoreclrhost.dll",
	CommandMode::MINGW64 => __DIR__ . "/build_mingw64/CoreCLR/libcoreclrhost.dll",
	CommandMode::CYGWIN => __DIR__ . "/build_cygwin/CoreCLR/cygcoreclrhost.dll"
);

function build($mode){
	global $buildCommands;
	return cmd($mode, $buildCommands[$mode]);
}

function getLoaderPath($ldrType, $ldrMode){
	global $clrHostPaths;
	global $monoHostPaths;
	global $coreClrHostPaths;

	if($ldrType === LdrType::CLRHost){
		return $clrHostPaths[CommandMode::WIN32_32];
	} else if($ldrType === LdrType::MonoHost){
		return $monoHostPaths[CommandMode::MINGW64];
	} else {
		return $coreClrHostPaths[$ldrMode];
	}
}

function getCliPath($cliMode){
	global $cliPaths;
	$cliPath = $cliPaths[$cliMode];
	return convert_path($cliMode, $cliPath);
}

function getSamplePath($cliMode, $ldrType){
	// AnyCPU
	$buildType = "Debug";
	if($ldrType == LdrType::CoreCLRHost){
		switch($cliMode){
			case CommandMode::MINGW32:
			case CommandMode::WIN32_32:
				$buildType = "x86/Debug";
				break;
			case CommandMode::MINGW64:
			case CommandMode::WIN32_64:
				$buildType = "x64/Debug";
				break;
		}
	}

	$basePath = __DIR__ . "/samples/Managed/Cygwin/bin/{$buildType}";
	if($ldrType === LdrType::CoreCLRHost){
		$samplePath = "{$basePath}/net5.0/Cygwin.dll";
	} else {
		$samplePath = "{$basePath}/net472/Cygwin.exe";
	}
	return convert_path($cliMode, $samplePath);
}

function test($cliMode, $ldrMode, $ldrType, $netLibPath){
	if($cliMode !== CommandMode::CYGWIN && $ldrMode == CommandMode::CYGWIN){
		print("== skipping ==\n");
		// cannot invoke Cygwin DLLs from Win32
		return;
	}

	$cliPath = getCliPath($cliMode);
	$ldrPath = convert_path($cliMode, getLoaderPath($ldrType, $ldrMode));

	$args = [
		'echo Hello | ',
		$cliPath, $ldrPath, $netLibPath,
		'ManagedSample.EntryPoint', 'Entry',
		'arg1', 'arg2', 'arg3', 'arg4', 'arg5'
	];
	var_dump($args);
	cmd($cliMode, ...$args);
}

function matrix($a, $b, $c){
	$r = array();
	foreach($a as $i){
		foreach($b as $j){
			foreach($c as $k){
				$r[] = [$i, $j, $j];
			}
		}
	}
	return $r;
}

function build_managed_sample(){
	$sample_path = path_concat(__DIR__, 'samples', 'Managed', 'Cygwin');
	$argSamplePath = escapeshellarg($sample_path);

	/** build sample in AnyCpu, x86, x64 variants */
	win32_64_cmd("cd {$argSamplePath} & dotnet build -p:Platform=AnyCpu");
	win32_64_cmd("cd {$argSamplePath} & dotnet build -p:Platform=x86");
	win32_64_cmd("cd {$argSamplePath} & dotnet build -p:Platform=x64");
	
	/** copy 32 and 64bit hostfxr */
	$hostfxr_version = "5.0.10";
	$hostfxr_path_suffix = "dotnet/host/fxr/{$hostfxr_version}/hostfxr.dll";

	$hostfxr_32 = "C:/Program Files (x86)/{$hostfxr_path_suffix}";
	$hostfxr_64 = "C:/Program Files/{$hostfxr_path_suffix}";

	$sample32 = getSamplePath(CommandMode::WIN32_32, LdrType::CoreCLRHost);
	$sample64 = getSamplePath(CommandMode::WIN32_64, LdrType::CoreCLRHost);

	$sample_dir32 = dirname($sample32);
	$sample_dir64 = dirname($sample64);

	copy($hostfxr_32, "{$sample_dir32}/hostfxr.dll");
	copy($hostfxr_64, "{$sample_dir64}/hostfxr.dll");
}

chdir(__DIR__);

build_managed_sample();
foreach([
	'build_cygwin',
	'build_mingw32', 'build_mingw64',
	'build_win32', 'build_win64'
] as $dir){
	@mkdir($dir);
}

$allModes = [CommandMode::WIN32_32, CommandMode::WIN32_64, CommandMode::MINGW32, CommandMode::MINGW64, CommandMode::CYGWIN];
foreach($allModes as $m){
	build($m);
}

$allLoaders = [LdrType::CLRHost, LdrType::MonoHost, LdrType::CoreCLRHost];
$allModeNames = array_flip((new ReflectionClass(CommandMode::class))->getConstants());
$allLoaderNames = array_flip((new ReflectionClass(LdrType::class))->getConstants());

// cliMode, ldrMode, ldrType
$m = matrix($allModes, $allModes, $allLoaders);
foreach($m as $combo){
	list($cliMode, $ldrMode, $ldrType) = $combo;

	$samplePath = getSamplePath($cliMode, $ldrType);

	$cliModeName = $allModeNames[$cliMode];
	$ldrModeName = $allModeNames[$ldrMode];
	$ldrTypeName = $allLoaderNames[$ldrType];
	print("== TEST BEGIN: cli:{$cliModeName},mode:{$ldrModeName},build:{$ldrTypeName}\n");
	print("Press <ENTER> to begin: ");
	//fgets(STDIN);

	test($cliMode, $ldrMode, $ldrType, $samplePath);
	print("== TEST END  : cli:{$cliModeName},mode:{$ldrModeName},build:{$ldrTypeName}\n");
}


?>