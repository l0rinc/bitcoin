# UNIX BUILD NOTES

Some notes on how to build Bitcoin Core in Unix.

(For BSD specific instructions, see `build-*bsd.md` in this directory.)

## To Build

```bash
cmake -B build
cmake --build build    # Append "-j N" for N parallel jobs
cmake --install build  # Optional
```

See below for instructions on how to [install the dependencies on popular Linux
distributions](#dependencies).

## Memory Requirements

C++ compilers are memory-hungry. It is recommended to have at least 1.5 GB of
memory available when compiling Bitcoin Core. On systems with less, gcc can be
tuned to conserve memory with additional `CMAKE_CXX_FLAGS`:


    cmake -B build -DCMAKE_CXX_FLAGS="--param ggc-min-expand=1 --param ggc-min-heapsize=32768"

Alternatively, or in addition, debugging information can be skipped for compilation.
For the default build type `RelWithDebInfo`, the default compile flags are
`-O2 -g`, and can be changed with:

    cmake -B build -DCMAKE_CXX_FLAGS_RELWITHDEBINFO="-O2 -g0"

Finally, clang (often less resource hungry) can be used instead of gcc, which is used by default:

    cmake -B build -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_C_COMPILER=clang

## Dependencies

You can either build from self-compiled [depends](/depends/README.md) or
install the dependencies from your distribution package manager. Dependencies
for additional features in later columns are optional.

| Package manager         | Required build dependencies | SQLite (wallet) | Cap'n Proto (IPC) | ZMQ <sup><a href="#note1">[1]</a></sup> | USDT | UPnP | Qt and libqrencode (GUI) |
| ----------------------- | --------------------------- | --------------- | ----------------- | --- | ---- | ---- | ------------------------ |
| Debian / Ubuntu (`apt`) | `build-essential cmake python3 libboost-dev` | `libsqlite3-dev` | `libcapnp-dev capnproto` | `libzmq3-dev pkgconf` | `systemtap-sdt-dev` | `libminiupnpc-dev` | `qtbase5-dev qttools5-dev qttools5-dev-tools qtwayland5 libqrencode-dev` |
| Fedora (`dnf`)          | `gcc-c++ cmake make python3 boost-devel` | `sqlite-devel` | `capnproto capnproto-devel` | `zeromq-devel pkgconf` | `systemtap-sdt-devel` | `miniupnpc-devel` | `qt5-qtbase-devel qt5-qttools-devel qt5-qtwayland qrencode-devel` |
| Alpine (`apk`)          | `build-base cmake linux-headers python3 boost-dev` | `sqlite-dev` | `capnproto capnproto-dev` | `zeromq-dev` | Not supported | `miniupnpc-dev` | `qt5-qtbase-dev qt5-qttools-dev libqrencode-dev` |
| Arch (`pacman`)         | `gcc make cmake python boost` | `sqlite` | `capnproto` | `zeromq` | `systemtap` | `miniupnpc` | `qt5-base qt5-tools qt5-wayland qrencode` |

<a id="note1"></a>1. Some ZMQ packages do not vendor the CMake config files, which requires `pkgconf` or `pkg-config` to be installed.

For Debian "oldstable", or earlier Ubuntu LTS releases, you may need to pick a
later compiler version, according to the [dependencies](/doc/dependencies.md)
documentation.

To build Bitcoin Core without the wallet, see [*Disable-wallet mode*](#disable-wallet-mode)
and use `-DENABLE_WALLET=OFF` to build without the wallet and skip the SQLite dependency.

Cap'n Proto is needed for IPC functionality (see [multiprocess.md](multiprocess.md)).
Compile with `-DENABLE_IPC=OFF` if you do not need IPC functionality.

UPnP port mapping is compiled with `-DWITH_MINIUPNPC=ON` and requires miniupnpc.

ZMQ-enabled binaries are compiled with `-DWITH_ZMQ=ON` and require libzmq.

User-Space, Statically Defined Tracing (USDT) requires the systemtap-sdt
development package and must be enabled via `-DWITH_USDT=ON`.

GUI dependencies:

Bitcoin Core includes a GUI built with the cross-platform Qt Framework. To compile the GUI, we need to install
the necessary parts of Qt, the libqrencode and pass `-DBUILD_GUI=ON`. Skip if you don't intend to use the GUI.

For Qt 6.5 and later, the xcb cursor runtime package must be installed (`libxcb-cursor0` on Debian/Ubuntu, `xcb-util-cursor` on Fedora).

Additionally, install the Qt Wayland platform plugin for modern desktop environments.

The GUI will be able to encode addresses in QR codes and requires libqrencode.
Otherwise, if you don't need QR encoding support, use the `-DWITH_QRENCODE=OFF` option to disable this feature in order to compile the GUI.

The GUI builds with Qt 5 by default. You can build with Qt 6 instead by passing
`-DWITH_QT_VERSION=6` to cmake and installing your distribution's Qt 6
development packages.

### Disable-wallet mode

When the intention is to only run a P2P node, without a wallet, Bitcoin Core can
be compiled in disable-wallet mode with:

    cmake -B build -DENABLE_WALLET=OFF

In this case there is no dependency on SQLite.

Mining is also possible in disable-wallet mode using the `getblocktemplate` RPC call.
