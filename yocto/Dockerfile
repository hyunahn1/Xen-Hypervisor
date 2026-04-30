FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive
ENV LANG=en_US.UTF-8
ENV LC_ALL=en_US.UTF-8

# Yocto Scarthgap 공식 의존성 (Ubuntu 22.04 기준)
RUN apt-get update && apt-get install -y \
    gawk \
    wget \
    git \
    diffstat \
    unzip \
    texinfo \
    gcc \
    build-essential \
    chrpath \
    socat \
    cpio \
    python3 \
    python3-pip \
    python3-pexpect \
    xz-utils \
    debianutils \
    iputils-ping \
    python3-git \
    python3-jinja2 \
    python3-subunit \
    zstd \
    liblz4-tool \
    file \
    locales \
    libacl1 \
    lz4 \
    # bblayers.conf / bitbake UI
    pylint \
    python3-yaml \
    # 추가 도구
    curl \
    sudo \
    vim \
    && locale-gen en_US.UTF-8 \
    && update-locale LANG=en_US.UTF-8 \
    && rm -rf /var/lib/apt/lists/*

# 호스트 UID/GID와 일치하는 비root 사용자 생성
# (Yocto는 root로 실행 불가)
ARG UID=1000
ARG GID=1000
ARG USERNAME=yocto

RUN groupadd -g ${GID} ${USERNAME} 2>/dev/null || true \
    && useradd -m -u ${UID} -g ${GID} -s /bin/bash ${USERNAME} \
    && echo "${USERNAME} ALL=(ALL) NOPASSWD:ALL" >> /etc/sudoers

USER ${USERNAME}
WORKDIR /home/${USERNAME}
