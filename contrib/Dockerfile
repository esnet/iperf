# Instructions:
# - Configure for a static binary: ./configure --enable-static "LDFLAGS=--static" --disable-shared --without-openssl
# - Build: make
# - Build Docker image: docker build -t iperf3 -f contrib/Dockerfile .
#
# Example invocations:
# - Help: docker run iperf3 --help
# - Server: docker run -p 5201:5201 -it iperf3 -s
# - Client: docker run -it iperf3 -c 192.168.1.1 (note: since this is a minimal image and does not include DNS, name resolution will not work)
FROM scratch
COPY src/iperf3 /iperf3
COPY tmp /tmp
ENTRYPOINT ["/iperf3"]
EXPOSE 5201
CMD ["-s"]
