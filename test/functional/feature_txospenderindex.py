#!/usr/bin/env python3
"""Test txospenderindex recovery after an unclean reorg restart."""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal
from test_framework.wallet import MiniWallet


def prevout(txid, vout):
    return {"txid": txid, "vout": vout}


class TxoSpenderIndexTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.extra_args = [["-persistmempool=0", "-txospenderindex=1"]]

    def run_test(self):
        node = self.nodes[0]
        wallet = MiniWallet(node)
        root = wallet.get_utxo(confirmed_only=True)

        tx = wallet.send_self_transfer_multi(
            from_node=node, utxos_to_spend=[root], num_outputs=1
        )
        block = self.generate(wallet, 1)[0]
        self.wait_until(lambda: node.getindexinfo()["txospenderindex"]["synced"])

        node.invalidateblock(block)
        node.gettxoutsetinfo()
        node.kill_process()

        self.start_node(0)
        self.wait_until(lambda: node.getindexinfo()["txospenderindex"]["synced"])
        assert_equal(
            node.gettxspendingprevout(
                [prevout(root["txid"], root["vout"])], return_spending_tx=True
            ),
            [{"txid": root["txid"], "vout": root["vout"]}],
        )


if __name__ == "__main__":
    TxoSpenderIndexTest(__file__).main()
