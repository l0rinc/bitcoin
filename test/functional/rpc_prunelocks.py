#!/usr/bin/env python3
# Copyright (c) 2026 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test the listprunelocks and setprunelock RPCs."""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_raises_rpc_error,
)


class PruneLocksRPCTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1

    def locks_by_id(self):
        return {
            lock["id"]: lock
            for lock in self.nodes[0].listprunelocks()["prune_locks"]
        }

    def run_test(self):
        node = self.nodes[0]
        assert_equal(node.listprunelocks(), {"prune_locks": []})

        self.log.info("Test persisted and temporary prune locks")
        assert_equal(node.setprunelock("persist", {
            "desc": "Persisted lock",
            "height": [10, 20],
            "sync": True,
        }), {"success": True})
        assert_equal(node.setprunelock("temp", {
            "desc": "Temporary lock",
            "height": [30],
            "temporary": True,
            "sync": True,
        }), {"success": True})

        locks = self.locks_by_id()
        assert_equal(locks["persist"], {
            "id": "persist",
            "desc": "Persisted lock",
            "height": [10, 20],
            "temporary": False,
        })
        assert_equal(locks["temp"], {
            "id": "temp",
            "desc": "Temporary lock",
            "height": [30],
            "temporary": True,
        })

        self.restart_node(0)
        node = self.nodes[0]
        locks = self.locks_by_id()
        assert_equal(set(locks.keys()), {"persist"})
        assert_equal(locks["persist"]["temporary"], False)

        self.log.info("Test changing a persisted prune lock to temporary removes it from disk")
        assert_equal(node.setprunelock("persist", {
            "desc": "Demoted lock",
            "height": [40],
            "temporary": True,
            "sync": True,
        }), {"success": True})
        locks = self.locks_by_id()
        assert_equal(locks["persist"], {
            "id": "persist",
            "desc": "Demoted lock",
            "height": [40],
            "temporary": True,
        })

        self.restart_node(0)
        node = self.nodes[0]
        assert_equal(node.listprunelocks(), {"prune_locks": []})

        self.log.info("Test delete-all and invalid wildcard usage")
        assert_equal(node.setprunelock("first", {
            "desc": "First lock",
            "height": [1],
        }), {"success": True})
        assert_equal(node.setprunelock("second", {
            "desc": "Second lock",
            "height": [2],
        }), {"success": True})
        assert_equal(set(self.locks_by_id().keys()), {"first", "second"})
        assert_equal(node.setprunelock("*", {}), {"success": True})
        assert_equal(node.listprunelocks(), {"prune_locks": []})
        assert_raises_rpc_error(
            -8,
            'id "*" only makes sense when deleting',
            node.setprunelock,
            "*",
            {"desc": "Invalid wildcard", "height": [1]},
        )


if __name__ == '__main__':
    PruneLocksRPCTest(__file__).main()
