FROM ubuntu:latest
WORKDIR /iperf
COPY . .
RUN apt-get update && apt-get install -y gcc
RUN apt-get install -y make
RUN ./bootstrap.sh && ./configure && make && make install
CMD ["./src/iperf3"]