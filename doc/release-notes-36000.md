Performance Improvements
------------------------

- Background threads can now prefetch subsequent blocks from disk while another
  block is being connected, speeding up reindexing and initial block download.
  The `-blockfetchpar=<n>` option sets both the thread count and read-ahead queue
  size. It defaults to 4 and disables read-ahead when set to 0. (#36000)
