Block storage
-------------

Connected blocks and their block-index entries are now written to disk after
each block connection. After an unclean shutdown, the node can recover these
blocks locally instead of downloading them again. The UTXO database keeps its
existing write schedule. The additional synchronous disk writes can slow block
processing, especially during initial block download.
