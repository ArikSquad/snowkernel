FROM debian:stable-slim
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    nasm \
    xorriso \
    grub-pc-bin \
    grub-common \
    mtools \
    wget \
    ca-certificates \
 && rm -rf /var/lib/apt/lists/*
 
RUN apt-get update && apt-get install -y gcc-multilib binutils gcc && rm -rf /var/lib/apt/lists/*
WORKDIR /work
CMD ["/bin/bash"]
