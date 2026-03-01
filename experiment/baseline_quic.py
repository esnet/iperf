import subprocess

subprocess.run(
    "../src/iperf3 -c localhost -p 5201 --quic --json --logfile ./output/baseline_quic.log",
    shell=True
)