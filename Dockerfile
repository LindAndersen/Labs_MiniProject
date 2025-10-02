FROM ubuntu:22.04

WORKDIR /app

VOLUME /app

RUN apt-get update && \
    apt-get install -y \
    cmake \
    vim \
    build-essential && \
    rm -rf /var/lib/apt/lists/*

CMD ["bash"]