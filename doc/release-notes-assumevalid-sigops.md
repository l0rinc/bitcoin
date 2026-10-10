Validation
----------

Blocks whose script checks are skipped by assumevalid now also skip sigop counting.
The same ancestry, chainwork, and burial conditions apply, regardless of pruning.
This extends assumevalid trust to sigop limits. Use `-assumevalid=0` to check both scripts and sigop limits throughout the chain.
