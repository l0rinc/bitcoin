#!/usr/bin/env python3
# Copyright (c) 2026-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Exercise large JSON-RPC batch replies.

This test creates a large RPC reply and asserts it is served through the
expected HTTP reply path.
"""

import http.client
import json
from urllib.parse import urlparse

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    create_lots_of_big_transactions,
    gen_return_txouts,
    str_to_b64str,
)
from test_framework.wallet import MiniWallet


def raw_jsonrpc_request(url, payload, *, timeout):
    parsed = urlparse(url)
    assert parsed.hostname and parsed.port and None not in (parsed.username, parsed.password)

    auth_b64 = str_to_b64str(f"{parsed.username}:{parsed.password}")
    headers = {
        "Content-Type": "application/json",
        "Authorization": f"Basic {auth_b64}",
    }

    conn = http.client.HTTPConnection(parsed.hostname, parsed.port, timeout=timeout)
    conn.request("POST", "/", body=payload, headers=headers)
    response = conn.getresponse()
    while response.read(64 * 1024):
        pass
    conn.close()
    return response.status


class RPCBatchLargeReplyTest(BitcoinTestFramework):
    def add_options(self, parser):
        parser.add_argument("--target-response-mib", default=32, type=int)

    def set_test_params(self):
        self.num_nodes = 1
        self.setup_clean_chain = True

    def run_test(self):
        self.skip_if_running_under_valgrind()

        node = self.nodes[0]
        mini_wallet = MiniWallet(node)
        # Create enough mature coinbase UTXOs for multiple spends.
        self.generate(mini_wallet, 120)

        txouts = gen_return_txouts()
        fee = 100 * node.getnetworkinfo()["relayfee"]
        create_lots_of_big_transactions(mini_wallet, node, fee, tx_batch_size=8, txouts=txouts)
        self.generate(mini_wallet, 1)

        block_hash = node.getbestblockhash()
        # Fetch metadata here; the batch request materializes the block hex
        block_size = node.getblock(block_hash, 1)["size"]
        block_hex_len = block_size * 2

        # Cross the HTTP server's large-reply log threshold
        target_response_bytes = self.options.target_response_mib * 1024 * 1024
        batch_size = (target_response_bytes + block_hex_len - 1) // block_hex_len

        batch = [
            {"jsonrpc": "2.0", "id": i, "method": "getblock", "params": [block_hash, 0]}
            for i in range(batch_size)
        ]
        payload = json.dumps(batch).encode()

        with node.assert_debug_log(["Large HTTP reply body copied:"]):
            status = raw_jsonrpc_request(node.url, payload, timeout=120)
        assert_equal(status, 200)
        assert node.process.poll() is None


if __name__ == "__main__":
    RPCBatchLargeReplyTest(__file__).main()
