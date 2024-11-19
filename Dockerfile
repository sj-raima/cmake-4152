FROM ubuntu:22.04

# Install base development system
ENV DEBIAN_FRONTEND noninteractive
RUN apt-get update && apt-get install -y \
    git \
    wget \
    ninja-build \
    make \
    pkg-config \
    g++ \
    g++-i686-linux-gnu \
    g++-aarch64-linux-gnu \
    g++-arm-linux-gnueabi \
    g++-arm-linux-gnueabihf \
    g++-s390x-linux-gnu \
    openjdk-8-jre \
    openjdk-8-jdk \
    locales-all \
    libssl-dev \
    libz-dev \
    libcrypt-dev \
    libicu70 \
    curl \
    unzip \
    ant \
    jq \
    makeself \
    libxml-simple-perl \
    libconvert-binary-c-perl \
    libswitch-perl \
    libipc-run-perl \
    libtext-diff-perl && \
cpan -Tf install Parse::Lex

# Installing CMake from Kitware
RUN wget https://github.com/Kitware/CMake/releases/download/v3.28.6/cmake-3.28.6-linux-x86_64.sh && \
    sh cmake-3.28.6-linux-x86_64.sh --skip-license --exclude-subdir --prefix=/usr && \
    rm cmake-3.28.6-linux-x86_64.sh

# Install PowerShell from Microsoft
RUN wget https://github.com/PowerShell/PowerShell/releases/download/v7.4.6/powershell_7.4.6-1.deb_amd64.deb -O powershell.deb && \
    dpkg -i powershell.deb && \
    rm powershell.deb

# Change permission to allow RDM to be installed without elevated privileges
RUN chmod go+w /opt

# Add support for s390x
RUN apt-get install -y \
    qemu-user-static
    
# Add i386 architecture and install necessary libraries for 32-bit applications
RUN dpkg --add-architecture i386 && \
    apt-get update && \
    apt-get install -y \
    libc6:i386 \
    libstdc++6:i386 \
    zlib1g:i386 \
    libgcc1:i386
