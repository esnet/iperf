# Instructions:
# - Configure for a static binary: ./configure --enable-static "LDFLAGS=--static" --disable-shared --without-openssl
# - Build: make
# - Build Docker image: docker build -t iperf3 -f contrib/Dockerfile .
#
# Example invocations:
# - Help: docker run iperf3 --help
# - Server: docker run -p 5201:5201 -it iperf3 -s
# - Client: docker run -it iperf3 -c 192.168.1.1 (note: since this is a minimal image and does not include DNS, name resolution will not work)

# Step 1: Build stage

# Building using docker: docker buildx build -t iperf3 -f ./contrib/Dockerfile .

FROM debian:bullseye-slim AS builder

ARG BRANCH=master

# Install dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    curl \
    ca-certificates \
    libssl-dev \
    libtool \
    autoconf \
    automake \
    git \
    --no-install-recommends

WORKDIR /build

# Clone iperf3 source code
RUN git clone https://github.com/esnet/iperf.git .

# Checkout stable version
RUN git checkout ${BRANCH}

# Build iperf3
RUN ./bootstrap.sh && \
    ./configure && \
    make && \
    make install

# Step 2: Create the minimal image
FROM gcr.io/distroless/static-debian12:nonroot

# Copy the compiled iperf3 binary from the builder image
COPY --from=builder /usr/local/bin/iperf3 /usr/local/bin/iperf3

# Copy the required libraries from the builder image
COPY --from=builder /usr/local/lib/libiperf.so.0 /usr/local/lib/libiperf.so.0
COPY --from=builder /lib/x86_64-linux-gnu/libc.so.6 /lib/x86_64-linux-gnu/libc.so.6
COPY --from=builder /lib64/ld-linux-x86-64.so.2 /lib64/ld-linux-x86-64.so.2
COPY --from=builder /lib/x86_64-linux-gnu/libm.so.6 /lib/x86_64-linux-gnu/libm.so.6
COPY --from=builder /lib/x86_64-linux-gnu/libdl.so.2 /lib/x86_64-linux-gnu/libdl.so.2
COPY --from=builder /lib/x86_64-linux-gnu/libpthread.so.0 /lib/x86_64-linux-gnu/libpthread.so.0

# Copy required OpenSSL libraries
COPY --from=builder /usr/lib/x86_64-linux-gnu/libcrypto.so.1.1 /usr/lib/x86_64-linux-gnu/libcrypto.so.1.1
COPY --from=builder /usr/lib/x86_64-linux-gnu/libssl.so.1.1 /usr/lib/x86_64-linux-gnu/libssl.so.1.1

# Set the library path
ENV LD_LIBRARY_PATH=/usr/local/lib:/usr/lib/x86_64-linux-gnu

# Expose the default iperf3 port
EXPOSE 5201

# Set the entrypoint
ENTRYPOINT ["/usr/local/bin/iperf3"]
# Server Mode
CMD [ "-s" ]