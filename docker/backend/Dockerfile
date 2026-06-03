FROM ubuntu:22.04 AS builder

RUN apt-get update && apt-get install -y \
    build-essential cmake git \
    libmysqlclient-dev libhiredis-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY backend/ ./backend/

RUN cd backend && mkdir build && cd build && \
    cmake .. -DCMAKE_BUILD_TYPE=Release && \
    make -j$(nproc)

FROM ubuntu:22.04

RUN apt-get update && apt-get install -y \
    libmysqlclient21 libhiredis0.14 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY --from=builder /app/backend/build/lightning_server /app/lightning_server

ENV LS_HOST=0.0.0.0
ENV LS_PORT=8080
ENV LS_THREAD_NUM=4
ENV MYSQL_HOST=mysql
ENV MYSQL_PORT=3306
ENV MYSQL_USER=lightning
ENV MYSQL_PASSWORD=lightning123
ENV MYSQL_DATABASE=lightning_db
ENV REDIS_HOST=redis
ENV REDIS_PORT=6379

EXPOSE 8080
CMD ["./lightning_server"]
