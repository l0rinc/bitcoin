## Index

- A fully rebuilt transaction index (`-txindex`) now uses less than half as
  much disk space. The index is backwards compatible,
  so existing users will not see the space saving unless the index is recreated.
  To do so, stop the node, delete the `<datadir>/indexes/txindex` directory, and
  restart; rebuilding can take up to a few hours depending on hardware. Once
  rebuilt, the index can no longer be read by previous releases, so downgrading
  will require rebuilding it again. (#35531)
