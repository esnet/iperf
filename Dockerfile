FROM ubuntu:latest
WORKDIR /iperf
RUN --mount=type=cache,target=/var/cache/apt,sharing=locked \
  --mount=type=cache,target=/var/lib/apt,sharing=locked \
  apt update && apt-get install -y gcc && apt-get --no-install-recommends install -y make
COPY . .
RUN ./bootstrap.sh && ./configure && make && make install
CMD ["./src/iperf3", "-s"]