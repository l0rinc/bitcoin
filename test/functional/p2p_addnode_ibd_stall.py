#!/usr/bin/env python3

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal


class AddnodeIBDStallTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 3
        self.setup_clean_chain = True

    def setup_network(self):
        self.setup_nodes()

    def run_test(self):
        node_a = self.nodes[0]
        node_b = self.nodes[1]
        node_c = self.nodes[2]

        # Mine blocks on node_c so it has a longer chain, while leaving node_a
        # and node_b at height 0.
        target_height = 10
        self.generate(node_c, target_height, sync_fun=self.no_op)
        assert_equal(node_a.getblockcount(), 0)
        assert_equal(node_b.getblockcount(), 0)
        assert_equal(node_c.getblockcount(), target_height)

        # Connect B->A first, so B starts initial headers sync with A (which has
        # nothing new, so it will respond with an empty headers message).
        self.connect_nodes(1, 0)

        a_subver = node_a.getnetworkinfo()["subversion"]

        def peerinfo_to_a():
            for peer in node_b.getpeerinfo():
                if peer["subver"] == a_subver and not peer["inbound"]:
                    return peer
            return None

        # Wait until B has sent GETHEADERS and received the empty HEADERS response.
        self.wait_until(lambda: peerinfo_to_a() and peerinfo_to_a()["bytessent_per_msg"].get("getheaders", 0) > 0)
        self.wait_until(lambda: peerinfo_to_a() and peerinfo_to_a()["bytesrecv_per_msg"].get("headers", 0) > 0)

        # Now connect B->C. B should sync from C even while still connected to A.
        self.connect_nodes(1, 2)
        self.wait_until(lambda: node_b.getblockcount() == target_height)


if __name__ == "__main__":
    AddnodeIBDStallTest(__file__).main()
