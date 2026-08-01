FROM ubuntu:22.04 AS builder
ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    g++ cmake git libjsoncpp-dev uuid-dev zlib1g-dev openssl libssl-dev \
    && rm -rf /var/lib/apt/lists/*

RUN git clone https://github.com/drogonframework/drogon.git /tmp/drogon && \
    cd /tmp/drogon && git submodule update --init && \
    mkdir build && cd build && cmake .. -DBUILD_TESTING=OFF && make -j2 && make install && \
    rm -rf /tmp/drogon

WORKDIR /app
COPY . .

RUN mkdir build && cd build && cmake .. && make -j2

FROM ubuntu:22.04
RUN apt-get update && apt-get install -y libjsoncpp25 uuid-runtime openssl ca-certificates && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY --from=builder /app/build/mines_server /app/mines_server
COPY --from=builder /app/public /app/public

EXPOSE 8080
CMD ["./mines_server"]
