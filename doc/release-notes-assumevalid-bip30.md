Validation
----------

- Historical blocks covered by the existing assumevalid ancestry, minimum-chainwork and burial conditions skip BIP30 overwrite lookups alongside script verification.
  This extends assumevalid trust to the absence of overwrites of unspent transaction outputs, independently of pruning.
  With `-assumevalid=0` or ineligible blocks, the existing BIP30 enforcement remains unchanged.
