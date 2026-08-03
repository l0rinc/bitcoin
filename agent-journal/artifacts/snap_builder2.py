#!/usr/bin/env python3
"""Replicate feature_assumeutxo.py's canonical chain to height 299 and dump utxos.dat."""
import sys
sys.path.insert(0, '/mnt/my_storage/bitcoin/test/functional')
from test_framework.test_framework import BitcoinTestFramework
from test_framework.wallet import MiniWallet

SNAPSHOT_BASE_HEIGHT = 299
START_HEIGHT = 199

class SnapBuilder(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
    def setup_network(self):
        self.add_nodes(1)
        self.start_nodes()
    def run_test(self):
        n0 = self.nodes[0]
        self.mini_wallet = MiniWallet(n0)
        assert n0.getblockcount() == START_HEIGHT, n0.getblockcount()
        n0.setmocktime(n0.getblockheader(n0.getbestblockhash())['time'])
        for i in range(100):
            if i % 3 == 0:
                self.mini_wallet.send_self_transfer(from_node=n0)
            self.generate(n0, nblocks=1, sync_fun=self.no_op)
            if i == 4:
                temp_invalid = n0.getbestblockhash()
                n0.invalidateblock(temp_invalid)
                stale_hash = self.generateblock(n0, output="raw(aaaa)", transactions=[], sync_fun=self.no_op)["hash"]
                n0.invalidateblock(stale_hash)
                n0.reconsiderblock(temp_invalid)
        assert n0.getblockcount() == SNAPSHOT_BASE_HEIGHT, n0.getblockcount()
        res = n0.dumptxoutset('/tmp/snapA/utxos.dat', "latest")
        self.log.info(f"snapshot base_hash={res['base_hash']}")
        # committed regtest hash at 299:
        assert res['base_hash'] == "0c552ced4721c249a389eb9b08cb8da261cd46f0e7b5f9d064d48f3113406853", res['base_hash']

if __name__ == '__main__':
    SnapBuilder(__file__).main()
