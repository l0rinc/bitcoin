Performance Improvements
------------------------

Block and undo files are now committed to disk in the background when a block file
fills, allowing block processing to continue during these disk operations.
