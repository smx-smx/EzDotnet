#escape=`

FROM cirrusci/windowsservercore:cmake-2022.06.23
SHELL ["powershell", "-Command", "$ErrorActionPreference = 'Stop';"]

ARG MSYS_PACKAGES
ARG CYGWIN_PACKAGES

RUN Invoke-WebRequest https://github.com/msys2/msys2-installer/releases/download/2024-07-27/msys2-base-x86_64-20240727.sfx.exe -OutFile C:/msys2-base-x86_64.sfx.exe; `
	& C:/msys2-base-x86_64.sfx.exe -y -oC:\; `
	function msys() { C:\msys64\usr\bin\bash.exe @('-lc') + @Args; } `
	Remove-Item -Force 'C:/msys2-base-x86_64.sfx.exe'; `
	((Get-Content -path C:\msys64\etc\post-install\07-pacman-key.post -Raw) -replace '--refresh-keys', '--version') | Set-Content -Path C:\msys64\etc\post-install\07-pacman-key.post; `
	msys ' '; `
	msys 'sed "s/^CheckSpace/#CheckSpace/g" -i /etc/pacman.conf'; `
    msys 'pacman --noconfirm -Syuu --overwrite=*'; `
	msys 'mv -f /etc/pacman.conf.pacnew /etc/pacman.conf || true'; `
    echo 'Killing msys2 subprocesses'; `
    taskkill /F /FI 'MODULES eq msys-2.0.dll'; `
	msys 'pacman --noconfirm -Syuu --overwrite=*'; `
	$msys_packages = $env:MSYS_PACKAGES; `
	echo "Installing $msys_packages"; `
	msys "pacman --noconfirm -S --needed $msys_packages"; `
	del -Force -ErrorAction SilentlyContinue C:\msys64\etc\mtab; `
	del -Force -ErrorAction SilentlyContinue C:\msys64\dev\fd; `
	del -Force -ErrorAction SilentlyContinue C:\msys64\dev\stderr; `
	del -Force -ErrorAction SilentlyContinue C:\msys64\dev\stdin; `
	del -Force -ErrorAction SilentlyContinue C:\msys64\dev\stdout; `
	msys 'pacman --noconfirm -Scc'; `
	del -Force -Recurse -ErrorAction SilentlyContinue C:\msys64\var\cache\pacman\pkg; `
	taskkill /F /FI 'MODULES eq msys-2.0.dll'; `
	echo 'Clearing Recycle Bin (see https://github.com/docker/for-win/issues/8910)'; `
    If (Test-Path 'C:\$Recycle.Bin') { Remove-Item -Force -Recurse -Path 'C:\$Recycle.Bin'; };

RUN Invoke-WebRequest https://cygwin.com/setup-x86_64.exe -OutFile C:/setup.exe; `
	$cygwin_packages = ($env:CYGWIN_PACKAGES.Split('', [System.StringSplitOptions]::RemoveEmptyEntries) | % { $_.Trim() }) -Join(','); `
	$args = @( `
		'-qgnO', `
		'-s', 'http://mirrors.kernel.org/sourceware/cygwin/', `
		'-l', 'C:/cygwin-packages', `
		'-R', 'C:/cygwin64', `
		'-P', $cygwin_packages`
	); `
	& C:/setup.exe $args | Out-Default; `
	Remove-Item -Recurse -Force 'C:/cygwin-packages'; `
	taskkill /F /FI 'MODULES eq cygwin1.dll'

RUN choco upgrade --no-progress -fy dotnet-6.0-sdk dotnet-6.0-runtime cmake; `
	choco upgrade --no-progress -fy dotnet-6.0-runtime --x86; `
	Remove-Item -Force -Recurse $env:LOCALAPPDATA\Temp\chocolatey
