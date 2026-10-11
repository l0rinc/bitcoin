P2P and network changes
----------------------

- The default block download window has increased from 1024 to 2048 blocks, allowing more download lookahead.
  Downloaded blocks ahead of the connected tip are stored on disk and cannot yet be pruned, so a larger window may increase temporary disk usage above the prune target during initial block download.
  The debug option `-blockdownloadwindow` can adjust the window, with values clamped between 160 and 32768 blocks.
