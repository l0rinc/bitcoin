#!/usr/bin/env python3
# Copyright (c) 2014-2021 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test fee estimates persistence.

By default, bitcoind will dump fee estimates on shutdown and
then reload it on startup.

Test is as follows:

  - start node0
  - call the savefeeestimates RPC and verify the RPC succeeds and
    that the file exists
  - make the file unwritable and attempt to call the savefeeestimates RPC
    with the expectation that it will fail
  - move the unwritable file and shut down the node, verify the node writes
    on shutdown a file that is identical to the one we saved via the RPC

"""

import filecmp

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_raises_rpc_error


class FeeEstimatesPersistTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1

    def run_test(self):
        fee_estimatesdat = self.nodes[0].chain_path / 'fee_estimates.dat'
        self.log.debug('Verify the fee_estimates.dat file does not exist on startup')
        assert not fee_estimatesdat.is_file()
        self.nodes[0].savefeeestimates()
        self.log.debug('Verify the fee_estimates.dat file exists after calling savefeeestimates RPC')
        assert fee_estimatesdat.is_file()
        self.log.debug("Prevent bitcoind from writing fee_estimates.dat to disk. Verify that `savefeeestimates` fails")
        fee_estimatesdatold = fee_estimatesdat.with_suffix(fee_estimatesdat.suffix + '.old')
        fee_estimatesdat.rename(fee_estimatesdatold)
        fee_estimatesdat.mkdir()
        assert_raises_rpc_error(-1, "Unable to dump fee estimates to disk", self.nodes[0].savefeeestimates)
        fee_estimatesdat.rmdir()
        self.stop_nodes()
        self.log.debug("Verify that fee_estimates are written on shutdown")
        assert fee_estimatesdat.is_file()
        self.log.debug("Verify that the fee estimates from a shutdown are identical from the ones from savefeeestimates")
        assert filecmp.cmp(fee_estimatesdat, fee_estimatesdatold)


if __name__ == "__main__":
    FeeEstimatesPersistTest(__file__).main()
