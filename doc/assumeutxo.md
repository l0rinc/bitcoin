# Assumeutxo Usage

Assumeutxo is a feature that allows fast bootstrapping of a validating bitcoind
instance.

For notes on the design of Assumeutxo, please refer to [the design doc](/doc/design/assumeutxo.md).

## Loading a snapshot

There is currently no canonical source for snapshots, but any downloaded snapshot
will be checked against the supported commitment for the selected network. Each
release supports only its newest snapshot commitment. If there is no source for
the snapshot you need, you can generate it yourself using
`dumptxoutset` on another node that is already synced (see
[Generating a snapshot](#generating-a-snapshot)).

Once you've obtained the snapshot, you can use the RPC command `loadtxoutset` to
load it.

```
$ bitcoin-cli -rpcclienttimeout=0 loadtxoutset /path/to/input
```

After the snapshot has loaded, the syncing process of both the snapshot chain
and the background IBD chain can be monitored with the `getchainstates` RPC.

### Pruning

A pruned node can load a snapshot. To save space, it's possible to delete the
snapshot file as soon as `loadtxoutset` finishes.

The minimum `-prune` setting is 550 MiB, but this functionality ignores that
minimum and uses at least 1100 MiB.

As the background sync continues there will be temporarily two chainstate
directories, each multiple gigabytes in size (likely growing larger than the
downloaded snapshot).

### Indexes

Indexes work but don't take advantage of this feature. They always start building
from the genesis block and can only apply blocks in order. Once the background
validation reaches the snapshot block, indexes will continue to build all the
way to the tip.


For indexes that support pruning, note that these indexes only allow blocks that
were already indexed to be pruned. Blocks that are not indexed yet will also
not be pruned.

This means that, if the snapshot is old, then a lot of blocks after the snapshot
block will need to be downloaded, and these blocks can't be pruned until they
are indexed, so they could consume a lot of disk space until indexing catches up
to the snapshot block.

## Checking the supported commitment during full IBD

When full block validation reaches the supported snapshot height, Bitcoin Core
flushes the block and coin state, scans the UTXO set, and compares its hash with
the compiled commitment. The check is enabled by default. A successful check logs
the hash and elapsed time. Problems are logged as errors. Flushing and scanning
can take several minutes.
Use `-checkassumeutxo=0` or `-nocheckassumeutxo` to skip this check.

Independent full-IBD nodes can reproduce the commitment, and a single honest
participant can report a mismatch for investigation. Mismatches are diagnostic
and do not change block acceptance. Results are logged locally and are not
announced automatically to peers.

The check runs when the committed block is connected. It does not retroactively
scan a node whose tip has already passed that height. Reindexing or a reorg that
connects the block again can repeat the check. Snapshot imports always verify
their content hash, regardless of this option. Background validation of imported
snapshots continues in this change.

## Generating a snapshot

The RPC command `dumptxoutset` can be used to generate a snapshot for the current
tip (using type "latest") or a recent height (using type "rollback"). A generated
snapshot can be loaded on another node if its base block and content hash match
the supported commitment in chainparams. Snapshots from older
heights require a compatible release that still supports them. An unfinished
snapshot chainstate based on a retired commitment also requires such a release
or rebuilding with `-reindex`.

Using the type parameter "rollback", `dumptxoutset` can also be used to verify the
hardcoded snapshot hash in the source code by regenerating the snapshot and
comparing the hash.

Example usage:

```
$ bitcoin-cli -rpcclienttimeout=0 dumptxoutset /path/to/output rollback
```

Rollback snapshots are generated using a temporary UTXO database. The active
chain remains intact and network activity continues. Historical block and undo
data must be available for the requested rollback.

`dumptxoutset` takes some time to complete, independent of hardware and
what parameter is chosen. Because of that it is recommended to increase the RPC
client timeout value (use `-rpcclienttimeout=0` for no timeout).
