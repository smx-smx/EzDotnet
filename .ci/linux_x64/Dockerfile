FROM debian:11.6

RUN apt-get update && \
	apt-get install -y --no-install-recommends curl && \
	apt-get clean && \
    rm -rf /var/lib/apt/lists/*

RUN curl -Lo packages-microsoft-prod.deb https://packages.microsoft.com/config/debian/11/packages-microsoft-prod.deb \
	dpkg -i packages-microsoft-prod.deb && \
	rm packages-microsoft-prod.deb

RUN apt-get update && \
    apt-get install -y --no-install-recommends \
	build-essential gcc g++ cmake \
	dotnet-sdk-7.0 dotnet-runtime-7.0 && \
	apt-get clean && \
    rm -rf /var/lib/apt/lists/*