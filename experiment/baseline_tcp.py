import subprocess

subprocess.run(
    "../src/iperf3 -c localhost -p 5201 --json --logfile ./output/baseline_tcp.log",
    shell=True,
    capture_output=True
)