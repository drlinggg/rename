FROM ubuntu:22.04

RUN apt-get update && apt-get install -y \
    gcc \
    make \
    git \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /workspace

COPY . .

RUN chmod +x scripts/*.sh

CMD ["make", "test"]
