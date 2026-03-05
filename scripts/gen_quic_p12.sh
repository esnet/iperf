#!/usr/bin/env bash
set -euo pipefail

# Generate self-signed cert/key + PKCS#12 bundle for iperf QUIC server.
# Defaults:
#   CRT=/tmp/iperf_quic.crt
#   KEY=/tmp/iperf_quic.key
#   P12=/tmp/iperf_quic.p12
#   CN=localhost
#
# Usage:
#   ./scripts/gen_quic_p12.sh
#   ./scripts/gen_quic_p12.sh /tmp/iperf_quic
#   ./scripts/gen_quic_p12.sh /tmp/custom_name my-hostname
#
# Notes:
# - Empty PKCS#12 password is used (matches: --quic-p12-pass "").
# - Existing files are overwritten.

if ! command -v openssl >/dev/null 2>&1; then
  echo "error: openssl not found in PATH" >&2
  exit 1
fi

base="${1:-/tmp/iperf_quic}"
cn="${2:-localhost}"

crt="${base}.crt"
key="${base}.key"
p12="${base}.p12"

mkdir -p "$(dirname "$base")"

openssl req -x509 -newkey rsa:2048 -nodes \
  -keyout "$key" \
  -out "$crt" \
  -days 365 \
  -subj "/CN=${cn}"

openssl pkcs12 -export \
  -out "$p12" \
  -inkey "$key" \
  -in "$crt" \
  -passout pass:

chmod 600 "$key" "$p12"
chmod 644 "$crt"

echo "Generated:"
echo "  CRT: $crt"
echo "  KEY: $key"
echo "  P12: $p12"
echo
echo "Server example:"
echo "  ./src/.libs/iperf3 -s -1 -V --quic --quic-p12 $p12 --quic-p12-pass \"\""
