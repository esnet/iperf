import subprocess

subprocess.run(
    "./src/iperf3 -c localhost -p 5201 --udp --json --logfile ./output/baseline_udp.log",
    shell=True
)