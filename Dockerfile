FROM ubuntu:24.04 AS build

RUN apt-get update && apt-get install -y --no-install-recommends \
        build-essential \
        cmake \
        ca-certificates \
        nlohmann-json3-dev \
        libxxhash-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /src
COPY CMakeLists.txt ./
COPY src ./src
COPY tests ./tests

RUN cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
    && cmake --build build -j "$(nproc)"

FROM ubuntu:24.04 AS runtime

RUN apt-get update && apt-get install -y --no-install-recommends \
        libxxhash0 \
    && rm -rf /var/lib/apt/lists/* \
    && mkdir -p /data /etc/codeghost /var/log/codeghost /var/lib/codeghost

COPY --from=build /src/build/codeghost /usr/local/bin/codeghost
COPY config/config.json /etc/codeghost/config.json

ENTRYPOINT ["codeghost"]
CMD ["--config=/etc/codeghost/config.json"]
