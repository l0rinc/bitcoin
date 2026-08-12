#!/usr/bin/env python3
# Copyright (c) 2026-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Stress aggregate memory retained by incomplete P2P messages."""

import math
import os
from pathlib import Path
import resource
import socket
import struct
import time

from test_framework.messages import MAGIC_BYTES, MAX_PROTOCOL_MESSAGE_LENGTH
from test_framework.test_framework import BitcoinTestFramework, SkipTest
from test_framework.util import assert_equal, p2p_port


PARTIAL_PAYLOAD_SIZE = 3_800_000
PEER_HEADROOM = 64
PAGE_SIZE = 4096


def read_meminfo():
    result = {}
    try:
        for line in Path("/proc/meminfo").read_text().splitlines():
            key, value = line.split(":", 1)
            if key in ("MemAvailable", "MemTotal", "SwapFree", "SwapTotal"):
                result[key] = int(value.split()[0])
    except FileNotFoundError:
        return None
    return result


def proc_memory(pid):
    result = {}
    try:
        for line in Path(f"/proc/{pid}/status").read_text().splitlines():
            if line.startswith(("VmHWM:", "VmRSS:")):
                key, value = line.split(":", 1)
                result[key] = int(value.split()[0])
    except FileNotFoundError:
        pass
    return result


def oom_kills():
    for line in Path("/proc/vmstat").read_text().splitlines():
        key, value = line.split()
        if key == "oom_kill":
            return int(value)
    raise AssertionError("/proc/vmstat does not report oom_kill")


def cgroup_memory_max():
    for line in Path("/proc/self/cgroup").read_text().splitlines():
        hierarchy, controllers, cgroup = line.split(":", 2)
        if hierarchy == "0" and not controllers:
            path = Path("/sys/fs/cgroup") / cgroup.lstrip("/") / "memory.max"
            return path.read_text().strip()
    return None


class P2PReceiveMemoryStressTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.setup_clean_chain = True
        self.memory = read_meminfo()
        capacity = sum(self.memory[key] for key in ("MemAvailable", "SwapFree")) * 1024 if self.memory else 0
        self.peer_count = math.ceil(capacity / PARTIAL_PAYLOAD_SIZE) + PEER_HEADROOM
        self.extra_args = [[f"-maxconnections={self.peer_count + 16}"]]

    def add_options(self, parser):
        parser.add_argument(
            "--expect-result",
            choices=("oom", "bounded"),
            help="expected vulnerable or fixed behavior",
        )

    def skip_test_if_missing_module(self):
        if self.options.expect_result is None:
            raise SkipTest("stress test requires --expect-result")
        if self.memory is None:
            raise SkipTest("stress test requires Linux /proc")
        if not 512 * 1024 <= self.memory["MemTotal"] <= 6 * 1024 * 1024:
            raise SkipTest("stress test is limited to disposable 1 GiB or 4 GiB hosts")
        if cgroup_memory_max() != "max":
            raise SkipTest("stress test refuses an artificial cgroup memory limit")
        if resource.getrlimit(resource.RLIMIT_AS)[0] != resource.RLIM_INFINITY:
            raise SkipTest("stress test refuses an artificial address-space limit")
        required_fds = self.peer_count + 256
        if resource.getrlimit(resource.RLIMIT_NOFILE)[0] < required_fds:
            raise SkipTest(f"stress test requires at least {required_fds} file descriptors")

    def run_test(self):
        node = self.nodes[0]
        oom_before = oom_kills()
        rss_before = proc_memory(node.process.pid)
        max_rss_kib = rss_before.get("VmRSS", 0)

        header = (
            MAGIC_BYTES["regtest"]
            + b"version".ljust(12, b"\x00")
            + struct.pack("<I", MAX_PROTOCOL_MESSAGE_LENGTH)
            + b"\x00" * 4
        )
        payload = bytearray(os.urandom(PARTIAL_PAYLOAD_SIZE))
        peers = []
        payloads_sent = 0
        send_errors = 0

        for peer_index in range(self.peer_count):
            if node.process.poll() is not None:
                break
            for offset in range(0, len(payload) - 7, PAGE_SIZE):
                struct.pack_into("<Q", payload, offset, peer_index)
            peer = None
            try:
                peer = socket.create_connection(("127.0.0.1", p2p_port(0)), timeout=10)
                peer.settimeout(30)
                peer.sendall(header)
                peer.sendall(payload)
                payloads_sent += 1
                peers.append(peer)
            except OSError:
                send_errors += 1
                if peer is not None:
                    peer.close()
            max_rss_kib = max(max_rss_kib, proc_memory(node.process.pid).get("VmRSS", 0))
            if oom_kills() > oom_before:
                break

        if node.process.poll() is None:
            time.sleep(2)

        returncode = node.process.poll()
        max_rss_kib = max(max_rss_kib, proc_memory(node.process.pid).get("VmRSS", 0))
        oom_after = oom_kills()
        live_peers = len(node.getpeerinfo()) if returncode is None else 0

        self.log.info(
            "P2P_RECEIVE_MEMORY_RESULT mem_total_kib=%d mem_available_kib=%d "
            "swap_total_kib=%d swap_free_kib=%d expected=%s requested_peers=%d "
            "requested_payload_bytes=%d payloads_sent=%d send_errors=%d "
            "live_peers=%d process_returncode=%s rss_before_kib=%s "
            "max_sampled_rss_kib=%d rss_after_kib=%s oom_kills_before=%d oom_kills_after=%d",
            self.memory["MemTotal"],
            self.memory["MemAvailable"],
            self.memory["SwapTotal"],
            self.memory["SwapFree"],
            self.options.expect_result,
            self.peer_count,
            self.peer_count * PARTIAL_PAYLOAD_SIZE,
            payloads_sent,
            send_errors,
            live_peers,
            returncode,
            rss_before,
            max_rss_kib,
            proc_memory(node.process.pid),
            oom_before,
            oom_after,
        )

        for peer in peers:
            peer.close()

        if self.options.expect_result == "oom":
            if returncode is None:
                if oom_after > oom_before:
                    raise AssertionError("the kernel OOM-killed another process while bitcoind remained alive")
                raise AssertionError("node survived the expected machine OOM")
            if oom_after <= oom_before:
                raise AssertionError("node exited without a kernel OOM kill")
            node.wait_until_stopped(expected_ret_code=-9)
            return

        assert_equal(returncode, None)
        assert_equal(payloads_sent + send_errors, self.peer_count)
        if live_peers >= payloads_sent:
            raise AssertionError("aggregate limit did not disconnect a buffered peer")
        assert_equal(oom_after, oom_before)
        node.getblockcount()


if __name__ == "__main__":
    P2PReceiveMemoryStressTest(__file__).main()
