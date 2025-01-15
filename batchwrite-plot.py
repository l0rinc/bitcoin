import os
import re
import shutil
import statistics
import subprocess
import time
import datetime
import argparse
import matplotlib.pyplot as plt  # python3.12 -m pip install matplotlib --break-system-packages

# Regex to parse logs
BATCHWRITE_REGEX = re.compile(r"^(\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}Z) BatchWrite took=(\d+)ms, maxMem=(\d+)MiB")


def parse_log(archive):
    """Parse the log file to extract elapsed times, flush times, and memory usage."""
    start_time = None
    elapsed, batchwrite_times, usage_snapshots = [], [], []
    with open(archive, "r") as f:
        for line in f:
            if m := BATCHWRITE_REGEX.search(line):
                dt = datetime.datetime.strptime(m.group(1), "%Y-%m-%dT%H:%M:%SZ")
                if start_time is None:
                    start_time = dt
                elapsed.append((dt - start_time).total_seconds())
                batchwrite_times.append(int(m.group(2)))
                usage_snapshots.append(int(m.group(3)))
    return elapsed, batchwrite_times, usage_snapshots


def plot_results(results, output_dir):
    """Create separate plots for flush times and memory usage."""
    if len(results) != 2:
        print("plot_results() requires exactly 2 runs for comparison.")
        return

    (dbbatch0, elapsed0, flush0, mem0) = results[0]
    (dbbatch1, elapsed1, flush1, mem1) = results[1]

    # Compute percentage differences
    avg_flush0, avg_flush1 = statistics.mean(flush0), statistics.mean(flush1)
    max_mem0, max_mem1 = max(mem0), max(mem1)
    flush_improvement = round(((avg_flush0 - avg_flush1) / avg_flush0) * 100, 1)
    mem_increase = round(((max_mem1 - max_mem0) / max_mem0) * 100, 1)

    # Plot flush times
    plt.figure(figsize=(16, 8))
    plt.plot(elapsed0, flush0, color="red", linestyle="-", label=f"Flush Times (dbbatch={dbbatch0})")
    plt.axhline(y=avg_flush0, color="red", linestyle="--", alpha=0.5, label=f"Mean ({dbbatch0})={avg_flush0:.1f}ms")
    plt.plot(elapsed1, flush1, color="orange", linestyle="-", label=f"Flush Times (dbbatch={dbbatch1})")
    plt.axhline(y=avg_flush1, color="orange", linestyle="--", alpha=0.5, label=f"Mean ({dbbatch1})={avg_flush1:.1f}ms")
    plt.title(f"Flush Times (dbbatch {dbbatch0} vs {dbbatch1}) — {abs(flush_improvement)}% {'faster' if flush_improvement > 0 else 'slower'}")
    plt.xlabel("Elapsed Time (seconds)")
    plt.ylabel("Flush Times (ms)")
    plt.legend()
    plt.grid(True)
    plt.tight_layout()
    flush_out_file = os.path.join(output_dir, "plot_flush_times.png")
    plt.savefig(flush_out_file)
    print(f"Flush Times plot saved as {flush_out_file}")
    plt.close()

    # Plot memory usage
    plt.figure(figsize=(16, 8))
    plt.plot(elapsed0, mem0, color="blue", linestyle="-", label=f"Memory (dbbatch={dbbatch0})")
    plt.axhline(y=max_mem0, color="blue", linestyle="--", alpha=0.5, label=f"Max Mem ({dbbatch0})={max_mem0}MiB")
    plt.plot(elapsed1, mem1, color="green", linestyle="-", label=f"Memory (dbbatch={dbbatch1})")
    plt.axhline(y=max_mem1, color="green", linestyle="--", alpha=0.5, label=f"Max Mem ({dbbatch1})={max_mem1}MiB")
    plt.title(f"Memory Usage (dbbatch {dbbatch0} vs {dbbatch1}) — {abs(mem_increase)}% {'higher' if mem_increase > 0 else 'lower'}")
    plt.xlabel("Elapsed Time (seconds)")
    plt.ylabel("Memory Usage (MiB)")
    plt.legend()
    plt.grid(True)
    plt.tight_layout()
    mem_out_file = os.path.join(output_dir, "plot_memory_usage.png")
    plt.savefig(mem_out_file)
    print(f"Memory Usage plot saved as {mem_out_file}")
    plt.close()


def loadtxoutset(dbbatchsize, datadir, bitcoin_cli, bitcoind, utxo_file):
    """Load the UTXO set and run the Bitcoin node."""
    archive = os.path.join(datadir, f"results_dbbatch-{dbbatchsize}.log")

    # Skip if logs already exist
    if os.path.exists(archive):
        print(f"Log file {archive} already exists. Skipping loadtxoutset for dbbatchsize={dbbatchsize}.")
        return

    os.makedirs(datadir, exist_ok=True)
    debug_log = os.path.join(datadir, "debug.log")

    try:
        print("Cleaning up previous run")
        for subdir in ["chainstate", "chainstate_snapshot"]:
            shutil.rmtree(os.path.join(datadir, subdir), ignore_errors=True)

        print("Preparing UTXO load")
        subprocess.run([bitcoind, f"-datadir={datadir}", "-stopatheight=1"], cwd=bitcoin_core_path)
        os.remove(debug_log)

        print(f"Starting bitcoind with dbbatchsize={dbbatchsize}")
        subprocess.run([bitcoind, f"-datadir={datadir}", "-daemon", "-blocksonly=1", "-connect=0", f"-dbbatchsize={dbbatchsize}", f"-dbcache={440}"], cwd=bitcoin_core_path)
        time.sleep(5)

        print("Loading UTXO set")
        subprocess.run([bitcoin_cli, f"-datadir={datadir}", "loadtxoutset", utxo_file], cwd=bitcoin_core_path)
    except Exception as e:
        print(f"Error during loadtxoutset for dbbatchsize={dbbatchsize}: {e}")
        raise
    finally:
        print("Stopping bitcoind...")
        subprocess.run([bitcoin_cli, f"-datadir={datadir}", "stop"], cwd=bitcoin_core_path, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        time.sleep(2)

    shutil.copy2(debug_log, archive)
    print(f"Archived logs to {archive}")


if __name__ == "__main__":
    # Parse script arguments
    parser = argparse.ArgumentParser(description="Benchmark Bitcoin dbbatchsize configurations.")
    parser.add_argument("--utxo-file", required=True, help="Path to the UTXO snapshot file.")
    parser.add_argument("--bitcoin-core-path", required=True, help="Path to the Bitcoin Core project directory.")
    args = parser.parse_args()

    utxo_file = args.utxo_file
    bitcoin_core_path = args.bitcoin_core_path
    datadir = os.path.join(bitcoin_core_path, "demo")
    debug_log = os.path.join(datadir, "debug.log")
    bitcoin_cli = os.path.join(bitcoin_core_path, "build/src/bitcoin-cli")
    bitcoind = os.path.join(bitcoin_core_path, "build/src/bitcoind")

    # Build Bitcoin Core
    print("Building Bitcoin Core...")
    subprocess.run(["cmake", "-B", "build", "-DCMAKE_BUILD_TYPE=Release"], cwd=bitcoin_core_path, check=True)
    subprocess.run(["cmake", "--build", "build", "-j", str(os.cpu_count())], cwd=bitcoin_core_path, check=True)

    # Run tests for each dbbatchsize
    results = []
    for dbbatchsize in [16777216, 67108864]:  # Original and proposed
        loadtxoutset(dbbatchsize, datadir, bitcoin_cli, bitcoind, utxo_file)
        archive = os.path.join(datadir, f"results_dbbatch-{dbbatchsize}.log")
        elapsed, batchwrite_times, usage_snapshots = parse_log(archive)
        results.append((dbbatchsize, elapsed, batchwrite_times, usage_snapshots))

    # Plot results
    plot_results(results, bitcoin_core_path)
    print("All configurations processed.")
