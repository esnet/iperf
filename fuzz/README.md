# Fuzzing iperf3

This directory contains libFuzzer-based fuzz targets for parts of the
iperf3 code base. The same targets are also built and run by
[OSS-Fuzz](https://github.com/google/oss-fuzz).

## Targets

* `cjson_fuzzer` — feeds arbitrary input to the bundled cJSON parser
  (`src/cjson.c`) through `cJSON_Parse`/`cJSON_Delete`.
* `auth_fuzzer` — feeds arbitrary input to `Base64Decode` from
  `src/iperf_auth.c` (requires OpenSSL).

## Build

The targets include the generated `src/iperf_config.h`, so configure and
build the project first from the top-level directory:

    ./configure
    make -j$(nproc)

Then build the fuzz targets from this directory (clang is required):

    make

## Run

    ./cjson_fuzzer <CORPUS DIR>
    ./auth_fuzzer <CORPUS DIR>

where `<CORPUS DIR>` is a directory with input files, for example JSON
documents for `cjson_fuzzer` or base64-encoded data for `auth_fuzzer`.

If a fuzzer crashes, a `crash-*` file is written. Reproduce it with:

    ./<target> <crash-file>
