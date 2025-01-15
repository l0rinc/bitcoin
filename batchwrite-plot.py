import os
import re
import shutil
import statistics
import subprocess
import time
import datetime
import matplotlib.pyplot as plt  # python3.12 -m pip install matplotlib --break-system-packages

DBBATCH_VALUES = [16777216, 67108864]  # original, proposed

UTXO_FILE = "/mnt/my_storage/utxo-840000.dat"
BITCOIN_CORE_PATH = "/mnt/my_storage/bitcoin"
DATADIR = BITCOIN_CORE_PATH + "/demo"
DEBUG_LOG = DATADIR + "/debug.log"
BITCOIN_CLI = "build/src/bitcoin-cli"
BITCOIND = "build/src/bitcoind"

BATCHWRITE_REGEX = re.compile(r"^(\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}Z) BatchWrite took=(\d+)ms, maxMem=(\d+)MiB")


def parse_log(archive):
    start_time = None

    elapsed = []
    batchwrite_times = []
    usage_snapshots = []
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


def plot_results(results):
    if len(results) != 2:
        print("plot_results() requires exactly 2 runs for comparison.")
        return

    (dbbatch0, elapsed0, flush0, mem0) = results[0]
    (dbbatch1, elapsed1, flush1, mem1) = results[1]

    fig, ax1 = plt.subplots(figsize=(16, 8))
    ax2 = ax1.twinx()

    ax1.plot(elapsed0, flush0, color="red", linestyle="-", label=f"Flush Times (dbbatch={dbbatch0})")
    mean_flush0 = round(statistics.mean(flush0))
    ax1.axhline(linewidth=2 , y=mean_flush0, color="red", linestyle="--", alpha=0.5, label=f"Mean flush ({dbbatch0})={mean_flush0}ms")

    ax2.plot(elapsed0, mem0, color="blue", linestyle="-", label=f"Memory (dbbatch={dbbatch0})")
    max_mem0 = max(mem0)
    ax2.axhline(linewidth=2, y=max_mem0, color="blue", linestyle="--", alpha=0.5, label=f"Max mem ({dbbatch0})={max_mem0}MiB")


    ax1.plot(elapsed1, flush1, color="coral", linestyle="-", label=f"Flush Times (dbbatch={dbbatch1})")
    mean_flush1 = round(statistics.mean(flush1))
    ax1.axhline(linewidth=2, y=mean_flush1, color="coral", linestyle="--", alpha=0.5, label=f"Mean flush ({dbbatch1})={mean_flush1}ms")

    ax2.plot(elapsed1, mem1, color="navy", linestyle="-", label=f"Memory (dbbatch={dbbatch1})")
    max_mem1 = max(mem1)
    ax2.axhline(linewidth=2, y=max_mem1, color="navy", linestyle="--", alpha=0.5, label=f"Max mem ({dbbatch1})={max_mem1}MiB")

    ax1.set_xlabel("Elapsed Time (seconds)")
    ax1.set_ylabel("Flush Times (ms)", color="red")
    ax2.set_ylabel("Memory Usage (MiB)", color="blue")
    ax1.grid(True)

    lines1, labels1 = ax1.get_legend_handles_labels()
    lines2, labels2 = ax2.get_legend_handles_labels()
    ax1.legend(lines1 + lines2, labels1 + labels2, loc='center', bbox_to_anchor=(0.5, 0.5), ncol=2)

    plt.title("Comparison of Flush Times and Memory Usage for different `dbbatchsize` values")
    fig.tight_layout()
    out_file = f"{BITCOIN_CORE_PATH}/plot_compare.png"
    plt.savefig(out_file)
    print(f"Plot saved as {out_file}")
    plt.close()


def loadtxoutset(dbbatchsize):
    print("Cleaning up previous run")
    subprocess.run([BITCOIN_CLI, f"-datadir={DATADIR}", "stop"], cwd=BITCOIN_CORE_PATH)
    time.sleep(5)
    for subdir in ["chainstate", "chainstate_snapshot"]:
        shutil.rmtree(os.path.join(DATADIR, subdir), ignore_errors=True)

    print("Preparing UTXO load")
    subprocess.run([BITCOIND, f"-datadir={DATADIR}", "-stopatheight=1"], cwd=BITCOIN_CORE_PATH)
    os.remove(DEBUG_LOG)

    print(f"Starting bitcoind with dbbatchsize={dbbatchsize}")
    subprocess.run([BITCOIND, f"-datadir={DATADIR}", "-daemon", "-blocksonly=1", "-connect=0", f"-dbbatchsize={dbbatchsize}", f"-dbcache={440}"], cwd=BITCOIN_CORE_PATH)
    time.sleep(5)

    print("Loading UTXO set")
    subprocess.run([BITCOIN_CLI, f"-datadir={DATADIR}", "loadtxoutset", UTXO_FILE], cwd=BITCOIN_CORE_PATH)
    subprocess.run([BITCOIN_CLI, f"-datadir={DATADIR}", "stop"], cwd=BITCOIN_CORE_PATH)
    time.sleep(5)

# mkdir -p demo
if __name__ == "__main__":
    print("Building Bitcoin Core...")
    subprocess.run(["cmake", "-B", "build", "-DCMAKE_BUILD_TYPE=Release"], cwd=BITCOIN_CORE_PATH, check=True)
    subprocess.run(["cmake", "--build", "build", "-j", str(os.cpu_count())], cwd=BITCOIN_CORE_PATH, check=True)

    results = []
    for dbbatchsize in DBBATCH_VALUES:
        loadtxoutset(dbbatchsize)

        archive = f"{BITCOIN_CORE_PATH}/results_dbbatch-{dbbatchsize}.log"
        print(f"Archiving logs to {archive}")
        shutil.copy2(DEBUG_LOG, archive)

        elapsed, batchwrite_times, usage_snapshots = parse_log(archive)
        results.append((dbbatchsize, elapsed, batchwrite_times, usage_snapshots))

    plot_results(results)

    print("All configurations processed.")
