FROM cirrusci/windowsservercore:visualstudio2022

RUN @"%SystemRoot%\System32\WindowsPowerShell\v1.0\powershell.exe" \
	-NoProfile -InputFormat None \
	-ExecutionPolicy Bypass \
	-Command "iex ((New-Object System.Net.WebClient).DownloadString('https://chocolatey.org/install.ps1'))" && SET "PATH=%PATH%;%ALLUSERSPROFILE%\chocolatey\bin"

RUN choco install -y cmake cygwin msys2

RUN curl https://raw.githubusercontent.com/transcode-open/apt-cyg/master/apt-cyg -o apt-cyg && \
 	/tools/cygwin/bin/install apt-cyg /bin && \
	del apt-cyg

WORKDIR "/tools/cygwin"
RUN cygwinsetup.exe -q -P \
	cmake gcc-core gcc-g++