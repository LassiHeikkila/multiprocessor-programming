FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update \
    && apt-get upgrade -y \
    && apt-get install -y \
    build-essential \
    curl \
    git \
    gcc \
    clang \
    make \
    gcovr \
    clang-tidy \
    clang-format \
    opencl-headers \
    && rm -rf /var/lib/apt/lists/*
