# libFuzzer-based fuzz targets for iperf3.
# Requires clang and a configured source tree (see README.md in this directory).

CC       ?= clang
CFLAGS   ?= -g -O1 -fsanitize=fuzzer-no-link,address
LDFLAGS  ?= -fsanitize=fuzzer,address
SSL_LIBS = -lssl -lcrypto

FUZZERS = cjson_fuzzer auth_fuzzer

all: $(FUZZERS)

cjson_fuzzer: cjson_fuzzer.o cjson.o
	$(CC) $(LDFLAGS) $^ -lm -o $@

auth_fuzzer: auth_fuzzer.o iperf_auth.o
	$(CC) $(LDFLAGS) $^ $(SSL_LIBS) -o $@

cjson_fuzzer.o: cjson_fuzzer.c
	$(CC) $(CFLAGS) -I../src -c $< -o $@

cjson.o: ../src/cjson.c ../src/cjson.h
	$(CC) $(CFLAGS) -I../src -c $< -o $@

auth_fuzzer.o: auth_fuzzer.c
	$(CC) $(CFLAGS) -DHAVE_SSL -I../src -c $< -o $@

iperf_auth.o: ../src/iperf_auth.c ../src/iperf_auth.h
	$(CC) $(CFLAGS) -DHAVE_SSL -I../src -c $< -o $@

clean:
	rm -f $(FUZZERS) *.o

.PHONY: all clean
