#!/usr/bin/env python3
# Copyright (c) 2026-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Exercise large JSON-RPC batch replies.

bitcoin/bitcoin#31041 reported out-of-memory termination triggered by large
JSON-RPC batch replies. This test creates a large RPC reply and asserts it is
served through the expected HTTP reply path.
"""

import base64
import http.client
import json
from urllib.parse import urlparse

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    create_lots_of_big_transactions,
    gen_return_txouts,
)
from test_framework.wallet import MiniWallet


def raw_jsonrpc_request(url, payload, *, timeout):
    parsed = urlparse(url)
    assert parsed.hostname
    assert parsed.port
    assert parsed.username is not None
    assert parsed.password is not None

    auth_b64 = base64.b64encode(f"{parsed.username}:{parsed.password}".encode()).decode()
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


class RPCBatchMemoryTest(BitcoinTestFramework):
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
        # Avoid calling getblock(verbosity=0) here because it would load the
        # block hex into this process, defeating the purpose of exercising the
        # node's reply path with a large response.
        block_size = node.getblock(block_hash, 1)["size"]
        block_hex_len = block_size * 2

        # Ensure the response body is large enough to trigger the large-reply
        # log line in the HTTP server.
        target_response_bytes = 32 * 1024 * 1024
        batch_size = (target_response_bytes + block_hex_len - 1) // block_hex_len

        batch = [
            {"jsonrpc": "2.0", "id": i, "method": "getblock", "params": [block_hash, 0]}
            for i in range(batch_size)
        ]
        payload = json.dumps(batch).encode()

        assert node.process.poll() is None
        with node.assert_debug_log(["Large HTTP reply body copied:"]):
            status = raw_jsonrpc_request(node.url, payload, timeout=120)
        assert_equal(status, 200)
        assert node.process.poll() is None


if __name__ == "__main__":
    RPCBatchMemoryTest(__file__).main()
