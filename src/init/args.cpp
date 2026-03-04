// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bitcoin-build-config.h> // IWYU pragma: keep

#include <init.h>

#include <addrman.h>
#include <banman.h>
#include <blockfilter.h>
#include <chainparams.h>
#include <chainparamsbase.h>
#include <common/args.h>
#include <common/system.h>
#include <hash.h>
#include <httprpc.h>
#include <httpserver.h>
#include <index/blockfilterindex.h>
#include <index/coinstatsindex.h>
#include <index/txindex.h>
#include <init/common.h>
#include <kernel/caches.h>
#include <logging.h>
#include <mapport.h>
#include <net.h>
#include <net_permissions.h>
#include <net_processing.h>
#include <netbase.h>
#include <node/blockmanager_args.h>
#include <node/caches.h>
#include <node/chainstatemanager_args.h>
#include <node/kernel_notifications.h>
#include <node/mempool_persist_args.h>
#include <node/mempool_args.h>
#include <node/miner.h>
#include <policy/feerate.h>
#include <policy/fees/block_policy_estimator.h>
#include <policy/fees/block_policy_estimator_args.h>
#include <policy/policy.h>
#include <policy/settings.h>
#include <protocol.h>
#include <rpc/blockchain.h>
#include <rpc/server.h>
#include <rpc/util.h>
#include <script/sigcache.h>
#include <torcontrol.h>
#include <txmempool.h>
#include <util/asmap.h>
#include <util/moneystr.h>
#include <util/string.h>
#include <util/time.h>
#include <util/translation.h>
#include <validation.h>
#include <walletinitinterface.h>

#include <string>
#include <vector>

#ifdef ENABLE_ZMQ
#include <zmq/zmqabstractnotifier.h>
#endif

using node::DEFAULT_PRINT_MODIFIED_FEE;
using node::DEFAULT_PERSIST_MEMPOOL;
using node::DEFAULT_STOPATHEIGHT;
using util::Join;

namespace {
constexpr bool DEFAULT_PROXYRANDOMIZE{true};
constexpr bool DEFAULT_REST_ENABLE{false};
constexpr bool DEFAULT_I2P_ACCEPT_INCOMING{true};
constexpr bool DEFAULT_STOPAFTERBLOCKIMPORT{false};
constexpr const char* BITCOIN_PID_FILENAME{"bitcoind.pid"};
} // namespace

void SetupServerArgs(ArgsManager& argsman, bool can_listen_ipc)
{
    SetupHelpOptions(argsman);
    argsman.AddArg("-help-debug", "Print help message with debugging options and exit", ArgsManager::ALLOW_ANY, OptionsCategory::DEBUG_TEST); // server-only for now

    init::AddLoggingArgs(argsman);

    const auto defaultBaseParams = CreateBaseChainParams(ChainType::MAIN);
    const auto testnetBaseParams = CreateBaseChainParams(ChainType::TESTNET);
    const auto testnet4BaseParams = CreateBaseChainParams(ChainType::TESTNET4);
    const auto signetBaseParams = CreateBaseChainParams(ChainType::SIGNET);
    const auto regtestBaseParams = CreateBaseChainParams(ChainType::REGTEST);
    const auto defaultChainParams = CreateChainParams(argsman, ChainType::MAIN);
    const auto testnetChainParams = CreateChainParams(argsman, ChainType::TESTNET);
    const auto testnet4ChainParams = CreateChainParams(argsman, ChainType::TESTNET4);
    const auto signetChainParams = CreateChainParams(argsman, ChainType::SIGNET);
    const auto regtestChainParams = CreateChainParams(argsman, ChainType::REGTEST);

    // Hidden Options
    std::vector<std::string> hidden_args = {
        "-dbcrashratio", "-forcecompactdb",
        // GUI args. These will be overwritten by SetupUIArgs for the GUI
        "-choosedatadir", "-lang=<lang>", "-min", "-resetguisettings", "-splash", "-uiplatform"};

    argsman.AddArg("-version", "Print version and exit", ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
#if HAVE_SYSTEM
    argsman.AddArg("-alertnotify=<cmd>", "Execute command when an alert is raised (%s in cmd is replaced by message)", ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
#endif
    argsman.AddArg("-assumevalid=<hex>", strprintf("If this block is in the chain assume that it and its ancestors are valid and potentially skip their script verification (0 to verify all, default: %s, testnet3: %s, testnet4: %s, signet: %s)", defaultChainParams->GetConsensus().defaultAssumeValid.GetHex(), testnetChainParams->GetConsensus().defaultAssumeValid.GetHex(), testnet4ChainParams->GetConsensus().defaultAssumeValid.GetHex(), signetChainParams->GetConsensus().defaultAssumeValid.GetHex()), ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    argsman.AddArg("-blocksdir=<dir>", "Specify directory to hold blocks subdirectory for *.dat files (default: <datadir>)", ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    argsman.AddArg("-blocksxor",
                   strprintf("Whether an XOR-key applies to blocksdir *.dat files. "
                             "The created XOR-key will be zeros for an existing blocksdir or when `-blocksxor=0` is "
                             "set, and random for a freshly initialized blocksdir. "
                             "(default: %u)",
                             kernel::DEFAULT_XOR_BLOCKSDIR),
                   ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    argsman.AddArg("-fastprune", "Use smaller block files and lower minimum prune height for testing purposes", ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::DEBUG_TEST);
#if HAVE_SYSTEM
    argsman.AddArg("-blocknotify=<cmd>", "Execute command when the best block changes (%s in cmd is replaced by block hash)", ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
#endif
    argsman.AddArg("-blockreconstructionextratxn=<n>", strprintf("Extra transactions to keep in memory for compact block reconstructions (default: %u)", DEFAULT_BLOCK_RECONSTRUCTION_EXTRA_TXN), ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    argsman.AddArg("-blocksonly", strprintf("Whether to reject transactions from network peers. Disables automatic broadcast and rebroadcast of transactions, unless the source peer has the 'forcerelay' permission. RPC transactions are not affected. (default: %u)", DEFAULT_BLOCKSONLY), ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    argsman.AddArg("-coinstatsindex", strprintf("Maintain coinstats index used by the gettxoutsetinfo RPC (default: %u)", DEFAULT_COINSTATSINDEX), ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    argsman.AddArg("-conf=<file>", strprintf("Specify path to read-only configuration file. Relative paths will be prefixed by datadir location (only useable from command line, not configuration file) (default: %s)", BITCOIN_CONF_FILENAME), ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    argsman.AddArg("-datadir=<dir>", "Specify data directory", ArgsManager::ALLOW_ANY | ArgsManager::DISALLOW_NEGATION, OptionsCategory::OPTIONS);
    argsman.AddArg("-dbbatchsize", strprintf("Maximum database write batch size in bytes (default: %u)", DEFAULT_DB_CACHE_BATCH), ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::OPTIONS);
    argsman.AddArg("-dbcache=<n>", strprintf("Maximum database cache size <n> MiB (minimum %d, default: %d). Make sure you have enough RAM. In addition, unused memory allocated to the mempool is shared with this cache (see -maxmempool).", MIN_DB_CACHE >> 20, DEFAULT_DB_CACHE >> 20), ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    argsman.AddArg("-includeconf=<file>", "Specify additional configuration file, relative to the -datadir path (only useable from configuration file, not command line)", ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    argsman.AddArg("-allowignoredconf", strprintf("For backwards compatibility, treat an unused %s file in the datadir as a warning, not an error.", BITCOIN_CONF_FILENAME), ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    argsman.AddArg("-loadblock=<file>", "Imports blocks from external file on startup", ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    argsman.AddArg("-maxmempool=<n>", strprintf("Keep the transaction memory pool below <n> megabytes (default: %u)", DEFAULT_MAX_MEMPOOL_SIZE_MB), ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    argsman.AddArg("-mempoolexpiry=<n>", strprintf("Do not keep transactions in the mempool longer than <n> hours (default: %u)", DEFAULT_MEMPOOL_EXPIRY_HOURS), ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    argsman.AddArg("-minimumchainwork=<hex>", strprintf("Minimum work assumed to exist on a valid chain in hex (default: %s, testnet3: %s, testnet4: %s, signet: %s)", defaultChainParams->GetConsensus().nMinimumChainWork.GetHex(), testnetChainParams->GetConsensus().nMinimumChainWork.GetHex(), testnet4ChainParams->GetConsensus().nMinimumChainWork.GetHex(), signetChainParams->GetConsensus().nMinimumChainWork.GetHex()), ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::OPTIONS);
    argsman.AddArg("-par=<n>", strprintf("Set the number of script verification threads (0 = auto, up to %d, <0 = leave that many cores free, default: %d)",
        MAX_SCRIPTCHECK_THREADS, DEFAULT_SCRIPTCHECK_THREADS), ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    argsman.AddArg("-persistmempool", strprintf("Whether to save the mempool on shutdown and load on restart (default: %u)", DEFAULT_PERSIST_MEMPOOL), ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    argsman.AddArg("-persistmempoolv1",
                   strprintf("Whether a mempool.dat file created by -persistmempool or the savemempool RPC will be written in the legacy format "
                             "(version 1) or the current format (version 2). This temporary option will be removed in the future. (default: %u)",
                             DEFAULT_PERSIST_V1_DAT),
                   ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    argsman.AddArg("-pid=<file>", strprintf("Specify pid file. Relative paths will be prefixed by a net-specific datadir location. (default: %s)", BITCOIN_PID_FILENAME), ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    argsman.AddArg("-prune=<n>", strprintf("Reduce storage requirements by enabling pruning (deleting) of old blocks. This allows the pruneblockchain RPC to be called to delete specific blocks and enables automatic pruning of old blocks if a target size in MiB is provided. This mode is incompatible with -txindex. "
            "Warning: Reverting this setting requires re-downloading the entire blockchain. "
            "(default: 0 = disable pruning blocks, 1 = allow manual pruning via RPC, >=%u = automatically prune block files to stay under the specified target size in MiB)", MIN_DISK_SPACE_FOR_BLOCK_FILES / 1024 / 1024), ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    argsman.AddArg("-reindex", "If enabled, wipe chain state and block index, and rebuild them from blk*.dat files on disk. Also wipe and rebuild other optional indexes that are active. If an assumeutxo snapshot was loaded, its chainstate will be wiped as well. The snapshot can then be reloaded via RPC.", ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    argsman.AddArg("-reindex-chainstate", "If enabled, wipe chain state, and rebuild it from blk*.dat files on disk. If an assumeutxo snapshot was loaded, its chainstate will be wiped as well. The snapshot can then be reloaded via RPC.", ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    argsman.AddArg("-settings=<file>", strprintf("Specify path to dynamic settings data file. Can be disabled with -nosettings. File is written at runtime and not meant to be edited by users (use %s instead for custom settings). Relative paths will be prefixed by datadir location. (default: %s)", BITCOIN_CONF_FILENAME, BITCOIN_SETTINGS_FILENAME), ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
#if HAVE_SYSTEM
    argsman.AddArg("-startupnotify=<cmd>", "Execute command on startup.", ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    argsman.AddArg("-shutdownnotify=<cmd>", "Execute command immediately before beginning shutdown. The need for shutdown may be urgent, so be careful not to delay it long (if the command doesn't require interaction with the server, consider having it fork into the background).", ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
#endif
    argsman.AddArg("-txindex", strprintf("Maintain a full transaction index, used by the getrawtransaction rpc call (default: %u)", DEFAULT_TXINDEX), ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    argsman.AddArg("-blockfilterindex=<type>",
                 strprintf("Maintain an index of compact filters by block (default: %s, values: %s).", DEFAULT_BLOCKFILTERINDEX, ListBlockFilterTypes()) +
                 " If <type> is not supplied or if <type> = 1, indexes for all known types are enabled.",
                 ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);

    argsman.AddArg("-addnode=<ip>", strprintf("Add a node to connect to and attempt to keep the connection open (see the addnode RPC help for more info). This option can be specified multiple times to add multiple nodes; connections are limited to %u at a time and are counted separately from the -maxconnections limit.", MAX_ADDNODE_CONNECTIONS), ArgsManager::ALLOW_ANY | ArgsManager::NETWORK_ONLY, OptionsCategory::CONNECTION);
    argsman.AddArg("-asmap=<file>", strprintf("Specify asn mapping used for bucketing of the peers. Relative paths will be prefixed by the net-specific datadir location.%s",
                #ifdef ENABLE_EMBEDDED_ASMAP
                    " If a bool arg is given (-asmap or -asmap=1), the embedded mapping data in the binary will be used."
                #else
                    ""
                #endif
                ), ArgsManager::ALLOW_ANY, OptionsCategory::CONNECTION);
    argsman.AddArg("-bantime=<n>", strprintf("Default duration (in seconds) of manually configured bans (default: %u)", DEFAULT_MISBEHAVING_BANTIME), ArgsManager::ALLOW_ANY, OptionsCategory::CONNECTION);
    argsman.AddArg("-bind=<addr>[:<port>][=onion]", strprintf("Bind to given address and always listen on it (default: 0.0.0.0). Use [host]:port notation for IPv6. Append =onion to tag any incoming connections to that address and port as incoming Tor connections (default: 127.0.0.1:%u=onion, testnet3: 127.0.0.1:%u=onion, testnet4: 127.0.0.1:%u=onion, signet: 127.0.0.1:%u=onion, regtest: 127.0.0.1:%u=onion)", defaultChainParams->GetDefaultPort() + 1, testnetChainParams->GetDefaultPort() + 1, testnet4ChainParams->GetDefaultPort() + 1, signetChainParams->GetDefaultPort() + 1, regtestChainParams->GetDefaultPort() + 1), ArgsManager::ALLOW_ANY | ArgsManager::NETWORK_ONLY, OptionsCategory::CONNECTION);
    argsman.AddArg("-cjdnsreachable", "If set, then this host is configured for CJDNS (connecting to fc00::/8 addresses would lead us to the CJDNS network, see doc/cjdns.md) (default: 0)", ArgsManager::ALLOW_ANY, OptionsCategory::CONNECTION);
    argsman.AddArg("-connect=<ip>", "Connect only to the specified node; -noconnect disables automatic connections (the rules for this peer are the same as for -addnode). This option can be specified multiple times to connect to multiple nodes.", ArgsManager::ALLOW_ANY | ArgsManager::NETWORK_ONLY, OptionsCategory::CONNECTION);
    argsman.AddArg("-discover", "Discover own IP addresses (default: 1 when listening and no -externalip or -proxy)", ArgsManager::ALLOW_ANY, OptionsCategory::CONNECTION);
    argsman.AddArg("-dns", strprintf("Allow DNS lookups for -addnode, -seednode and -connect (default: %u)", DEFAULT_NAME_LOOKUP), ArgsManager::ALLOW_ANY, OptionsCategory::CONNECTION);
    argsman.AddArg("-dnsseed", strprintf("Query for peer addresses via DNS lookup, if low on addresses (default: %u unless -connect used or -maxconnections=0)", DEFAULT_DNSSEED), ArgsManager::ALLOW_ANY, OptionsCategory::CONNECTION);
    argsman.AddArg("-externalip=<ip>", "Specify your own public address", ArgsManager::ALLOW_ANY, OptionsCategory::CONNECTION);
    argsman.AddArg("-fixedseeds", strprintf("Allow fixed seeds if DNS seeds don't provide peers (default: %u)", DEFAULT_FIXEDSEEDS), ArgsManager::ALLOW_ANY, OptionsCategory::CONNECTION);
    argsman.AddArg("-forcednsseed", strprintf("Always query for peer addresses via DNS lookup (default: %u)", DEFAULT_FORCEDNSSEED), ArgsManager::ALLOW_ANY, OptionsCategory::CONNECTION);
    argsman.AddArg("-listen", strprintf("Accept connections from outside (default: %u if no -proxy, -connect or -maxconnections=0)", DEFAULT_LISTEN), ArgsManager::ALLOW_ANY, OptionsCategory::CONNECTION);
    argsman.AddArg("-listenonion", strprintf("Automatically create Tor onion service (default: %d)", DEFAULT_LISTEN_ONION), ArgsManager::ALLOW_ANY, OptionsCategory::CONNECTION);
    argsman.AddArg("-maxconnections=<n>", strprintf("Maintain at most <n> automatic connections to peers (default: %u). This limit does not apply to connections manually added via -addnode or the addnode RPC, which have a separate limit of %u. It does not apply to short-lived private broadcast connections either, which have a separate limit of %u.", DEFAULT_MAX_PEER_CONNECTIONS, MAX_ADDNODE_CONNECTIONS, MAX_PRIVATE_BROADCAST_CONNECTIONS), ArgsManager::ALLOW_ANY, OptionsCategory::CONNECTION);
    argsman.AddArg("-maxreceivebuffer=<n>", strprintf("Maximum per-connection receive buffer, <n>*1000 bytes (default: %u)", DEFAULT_MAXRECEIVEBUFFER), ArgsManager::ALLOW_ANY, OptionsCategory::CONNECTION);
    argsman.AddArg("-maxsendbuffer=<n>", strprintf("Maximum per-connection memory usage for the send buffer, <n>*1000 bytes (default: %u)", DEFAULT_MAXSENDBUFFER), ArgsManager::ALLOW_ANY, OptionsCategory::CONNECTION);
    argsman.AddArg("-maxuploadtarget=<n>", strprintf("Tries to keep outbound traffic under the given target per 24h. Limit does not apply to peers with 'download' permission or blocks created within past week. 0 = no limit (default: %s). Optional suffix units [k|K|m|M|g|G|t|T] (default: M). Lowercase is 1000 base while uppercase is 1024 base", DEFAULT_MAX_UPLOAD_TARGET), ArgsManager::ALLOW_ANY, OptionsCategory::CONNECTION);
#ifdef HAVE_SOCKADDR_UN
    argsman.AddArg("-onion=<ip:port|path>", "Use separate SOCKS5 proxy to reach peers via Tor onion services, set -noonion to disable (default: -proxy). May be a local file path prefixed with 'unix:'.", ArgsManager::ALLOW_ANY, OptionsCategory::CONNECTION);
#else
    argsman.AddArg("-onion=<ip:port>", "Use separate SOCKS5 proxy to reach peers via Tor onion services, set -noonion to disable (default: -proxy)", ArgsManager::ALLOW_ANY, OptionsCategory::CONNECTION);
#endif
    argsman.AddArg("-i2psam=<ip:port>", "I2P SAM proxy to reach I2P peers and accept I2P connections", ArgsManager::ALLOW_ANY, OptionsCategory::CONNECTION);
    argsman.AddArg("-i2pacceptincoming", strprintf("Whether to accept inbound I2P connections (default: %i). Ignored if -i2psam is not set. Listening for inbound I2P connections is done through the SAM proxy, not by binding to a local address and port.", DEFAULT_I2P_ACCEPT_INCOMING), ArgsManager::ALLOW_ANY, OptionsCategory::CONNECTION);
    argsman.AddArg("-onlynet=<net>", "Make automatic outbound connections only to network <net> (" + Join(GetNetworkNames(), ", ") + "). Inbound and manual connections are not affected by this option. It can be specified multiple times to allow multiple networks.", ArgsManager::ALLOW_ANY, OptionsCategory::CONNECTION);
    argsman.AddArg("-v2transport", strprintf("Support v2 transport (default: %u)", DEFAULT_V2_TRANSPORT), ArgsManager::ALLOW_ANY, OptionsCategory::CONNECTION);
    argsman.AddArg("-peerbloomfilters", strprintf("Support filtering of blocks and transaction with bloom filters (default: %u)", DEFAULT_PEERBLOOMFILTERS), ArgsManager::ALLOW_ANY, OptionsCategory::CONNECTION);
    argsman.AddArg("-peerblockfilters", strprintf("Serve compact block filters to peers per BIP 157 (default: %u)", DEFAULT_PEERBLOCKFILTERS), ArgsManager::ALLOW_ANY, OptionsCategory::CONNECTION);
    argsman.AddArg("-txreconciliation", strprintf("Enable transaction reconciliations per BIP 330 (default: %d)", DEFAULT_TXRECONCILIATION_ENABLE), ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::CONNECTION);
    argsman.AddArg("-port=<port>", strprintf("Listen for connections on <port> (default: %u, testnet3: %u, testnet4: %u, signet: %u, regtest: %u). Not relevant for I2P (see doc/i2p.md). If set to a value x, the default onion listening port will be set to x+1.", defaultChainParams->GetDefaultPort(), testnetChainParams->GetDefaultPort(), testnet4ChainParams->GetDefaultPort(), signetChainParams->GetDefaultPort(), regtestChainParams->GetDefaultPort()), ArgsManager::ALLOW_ANY | ArgsManager::NETWORK_ONLY, OptionsCategory::CONNECTION);
    const std::string proxy_doc_for_value =
#ifdef HAVE_SOCKADDR_UN
        "<ip>[:<port>]|unix:<path>";
#else
        "<ip>[:<port>]";
#endif
    const std::string proxy_doc_for_unix_socket =
#ifdef HAVE_SOCKADDR_UN
        "May be a local file path prefixed with 'unix:' if the proxy supports it. ";
#else
        "";
#endif
    argsman.AddArg("-proxy=" + proxy_doc_for_value + "[=<network>]",
                   "Connect through SOCKS5 proxy, set -noproxy to disable. " +
                   proxy_doc_for_unix_socket +
                   "Could end in =network to set the proxy only for that network. " +
                   "The network can be any of ipv4, ipv6, tor or cjdns. " +
                   "(default: disabled)",
                   ArgsManager::ALLOW_ANY | ArgsManager::DISALLOW_ELISION,
                   OptionsCategory::CONNECTION);
    argsman.AddArg("-proxyrandomize", strprintf("Randomize credentials for every proxy connection. This enables Tor stream isolation (default: %u)", DEFAULT_PROXYRANDOMIZE), ArgsManager::ALLOW_ANY, OptionsCategory::CONNECTION);
    argsman.AddArg("-seednode=<ip>", "Connect to a node to retrieve peer addresses, and disconnect. This option can be specified multiple times to connect to multiple nodes. During startup, seednodes will be tried before dnsseeds.", ArgsManager::ALLOW_ANY, OptionsCategory::CONNECTION);
    argsman.AddArg("-networkactive", "Enable all P2P network activity (default: 1). Can be changed by the setnetworkactive RPC command", ArgsManager::ALLOW_ANY, OptionsCategory::CONNECTION);
    argsman.AddArg("-timeout=<n>", strprintf("Specify socket connection timeout in milliseconds. If an initial attempt to connect is unsuccessful after this amount of time, drop it (minimum: 1, default: %d)", DEFAULT_CONNECT_TIMEOUT), ArgsManager::ALLOW_ANY, OptionsCategory::CONNECTION);
    argsman.AddArg("-peertimeout=<n>", strprintf("Specify a p2p connection timeout delay in seconds. After connecting to a peer, wait this amount of time before considering disconnection based on inactivity (minimum: 1, default: %d)", DEFAULT_PEER_CONNECT_TIMEOUT), ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::CONNECTION);
    argsman.AddArg("-torcontrol=<ip>:<port>", strprintf("Tor control host and port to use if onion listening enabled (default: %s). If no port is specified, the default port of %i will be used.", DEFAULT_TOR_CONTROL, DEFAULT_TOR_CONTROL_PORT), ArgsManager::ALLOW_ANY, OptionsCategory::CONNECTION);
    argsman.AddArg("-torpassword=<pass>", "Tor control port password (default: empty)", ArgsManager::ALLOW_ANY | ArgsManager::SENSITIVE, OptionsCategory::CONNECTION);
    argsman.AddArg("-natpmp", strprintf("Use PCP or NAT-PMP to map the listening port (default: %u)", DEFAULT_NATPMP), ArgsManager::ALLOW_ANY, OptionsCategory::CONNECTION);
    argsman.AddArg("-whitebind=<[permissions@]addr>", "Bind to the given address and add permission flags to the peers connecting to it. "
        "Use [host]:port notation for IPv6. Allowed permissions: " + Join(NET_PERMISSIONS_DOC, ", ") + ". "
        "Specify multiple permissions separated by commas (default: download,noban,mempool,relay). Can be specified multiple times.", ArgsManager::ALLOW_ANY, OptionsCategory::CONNECTION);

    argsman.AddArg("-whitelist=<[permissions@]IP address or network>", "Add permission flags to the peers using the given IP address (e.g. 1.2.3.4) or "
        "CIDR-notated network (e.g. 1.2.3.0/24). Uses the same permissions as "
        "-whitebind. "
        "Additional flags \"in\" and \"out\" control whether permissions apply to incoming connections and/or manual (default: incoming only). "
        "Can be specified multiple times.", ArgsManager::ALLOW_ANY, OptionsCategory::CONNECTION);

    g_wallet_init_interface.AddWalletOptions(argsman);

#ifdef ENABLE_ZMQ
    argsman.AddArg("-zmqpubhashblock=<address>", "Enable publish hash block in <address>", ArgsManager::ALLOW_ANY, OptionsCategory::ZMQ);
    argsman.AddArg("-zmqpubhashtx=<address>", "Enable publish hash transaction in <address>", ArgsManager::ALLOW_ANY, OptionsCategory::ZMQ);
    argsman.AddArg("-zmqpubrawblock=<address>", "Enable publish raw block in <address>", ArgsManager::ALLOW_ANY, OptionsCategory::ZMQ);
    argsman.AddArg("-zmqpubrawtx=<address>", "Enable publish raw transaction in <address>", ArgsManager::ALLOW_ANY, OptionsCategory::ZMQ);
    argsman.AddArg("-zmqpubsequence=<address>", "Enable publish hash block and tx sequence in <address>", ArgsManager::ALLOW_ANY, OptionsCategory::ZMQ);
    argsman.AddArg("-zmqpubhashblockhwm=<n>", strprintf("Set publish hash block outbound message high water mark (default: %d)", CZMQAbstractNotifier::DEFAULT_ZMQ_SNDHWM), ArgsManager::ALLOW_ANY, OptionsCategory::ZMQ);
    argsman.AddArg("-zmqpubhashtxhwm=<n>", strprintf("Set publish hash transaction outbound message high water mark (default: %d)", CZMQAbstractNotifier::DEFAULT_ZMQ_SNDHWM), ArgsManager::ALLOW_ANY, OptionsCategory::ZMQ);
    argsman.AddArg("-zmqpubrawblockhwm=<n>", strprintf("Set publish raw block outbound message high water mark (default: %d)", CZMQAbstractNotifier::DEFAULT_ZMQ_SNDHWM), ArgsManager::ALLOW_ANY, OptionsCategory::ZMQ);
    argsman.AddArg("-zmqpubrawtxhwm=<n>", strprintf("Set publish raw transaction outbound message high water mark (default: %d)", CZMQAbstractNotifier::DEFAULT_ZMQ_SNDHWM), ArgsManager::ALLOW_ANY, OptionsCategory::ZMQ);
    argsman.AddArg("-zmqpubsequencehwm=<n>", strprintf("Set publish hash sequence message high water mark (default: %d)", CZMQAbstractNotifier::DEFAULT_ZMQ_SNDHWM), ArgsManager::ALLOW_ANY, OptionsCategory::ZMQ);
#else
    hidden_args.emplace_back("-zmqpubhashblock=<address>");
    hidden_args.emplace_back("-zmqpubhashtx=<address>");
    hidden_args.emplace_back("-zmqpubrawblock=<address>");
    hidden_args.emplace_back("-zmqpubrawtx=<address>");
    hidden_args.emplace_back("-zmqpubsequence=<n>");
    hidden_args.emplace_back("-zmqpubhashblockhwm=<n>");
    hidden_args.emplace_back("-zmqpubhashtxhwm=<n>");
    hidden_args.emplace_back("-zmqpubrawblockhwm=<n>");
    hidden_args.emplace_back("-zmqpubrawtxhwm=<n>");
    hidden_args.emplace_back("-zmqpubsequencehwm=<n>");
#endif

    argsman.AddArg("-checkblocks=<n>", strprintf("How many blocks to check at startup (default: %u, 0 = all)", DEFAULT_CHECKBLOCKS), ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::DEBUG_TEST);
    argsman.AddArg("-checklevel=<n>", strprintf("How thorough the block verification of -checkblocks is: %s (0-4, default: %u)", Join(CHECKLEVEL_DOC, ", "), DEFAULT_CHECKLEVEL), ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::DEBUG_TEST);
    argsman.AddArg("-checkblockindex", strprintf("Do a consistency check for the block tree, chainstate, and other validation data structures every <n> operations. Use 0 to disable. (default: %u, regtest: %u)", defaultChainParams->DefaultConsistencyChecks(), regtestChainParams->DefaultConsistencyChecks()), ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::DEBUG_TEST);
    argsman.AddArg("-checkaddrman=<n>", strprintf("Run addrman consistency checks every <n> operations. Use 0 to disable. (default: %u)", DEFAULT_ADDRMAN_CONSISTENCY_CHECKS), ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::DEBUG_TEST);
    argsman.AddArg("-checkmempool=<n>", strprintf("Run mempool consistency checks every <n> transactions. Use 0 to disable. (default: %u, regtest: %u)", defaultChainParams->DefaultConsistencyChecks(), regtestChainParams->DefaultConsistencyChecks()), ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::DEBUG_TEST);
    // Checkpoints were removed. We keep `-checkpoints` as a hidden arg to display a more user friendly error when set.
    argsman.AddArg("-checkpoints", "", ArgsManager::ALLOW_ANY, OptionsCategory::HIDDEN);
    argsman.AddArg("-deprecatedrpc=<method>", "Allows deprecated RPC method(s) to be used", ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::DEBUG_TEST);
    argsman.AddArg("-stopafterblockimport", strprintf("Stop running after importing blocks from disk (default: %u)", DEFAULT_STOPAFTERBLOCKIMPORT), ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::DEBUG_TEST);
    argsman.AddArg("-stopatheight", strprintf("Stop running after reaching the given height in the main chain (default: %u). Blocks after target height may be processed during shutdown.", DEFAULT_STOPATHEIGHT), ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::DEBUG_TEST);
    argsman.AddArg("-limitancestorcount=<n>", strprintf("Deprecated setting to not accept transactions if number of in-mempool ancestors is <n> or more (default: %u); replaced by cluster limits (see -limitclustercount) and only used by wallet for coin selection", DEFAULT_ANCESTOR_LIMIT), ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::DEBUG_TEST);
    // Ancestor and descendant size limits were removed. We keep
    // -limitancestorsize/-limitdescendantsize as hidden args to display a more
    // user friendly error when set.
    argsman.AddArg("-limitancestorsize", "", ArgsManager::ALLOW_ANY, OptionsCategory::HIDDEN);
    argsman.AddArg("-limitdescendantsize", "", ArgsManager::ALLOW_ANY, OptionsCategory::HIDDEN);
    argsman.AddArg("-limitdescendantcount=<n>", strprintf("Deprecated setting to not accept transactions if any ancestor would have <n> or more in-mempool descendants (default: %u); replaced by cluster limits (see -limitclustercount) and only used by wallet for coin selection", DEFAULT_DESCENDANT_LIMIT), ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::DEBUG_TEST);
    argsman.AddArg("-test=<option>", "Pass a test-only option. Options include : " + Join(TEST_OPTIONS_DOC, ", ") + ".", ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::DEBUG_TEST);
    argsman.AddArg("-limitclustercount=<n>", strprintf("Do not accept transactions into mempool which are directly or indirectly connected to <n> or more other unconfirmed transactions (default: %u, maximum: %u)", DEFAULT_CLUSTER_LIMIT, MAX_CLUSTER_COUNT_LIMIT), ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::DEBUG_TEST);
    argsman.AddArg("-limitclustersize=<n>", strprintf("Do not accept transactions whose virtual size with all in-mempool connected transactions exceeds <n> kilobytes (default: %u)", DEFAULT_CLUSTER_SIZE_LIMIT_KVB), ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::DEBUG_TEST);
    argsman.AddArg("-capturemessages", "Capture all P2P messages to disk", ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::DEBUG_TEST);
    argsman.AddArg("-mocktime=<n>", "Replace actual time with " + UNIX_EPOCH_TIME + " (default: 0)", ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::DEBUG_TEST);
    argsman.AddArg("-maxsigcachesize=<n>", strprintf("Limit sum of signature cache and script execution cache sizes to <n> MiB (default: %u)", DEFAULT_VALIDATION_CACHE_BYTES >> 20), ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::DEBUG_TEST);
    argsman.AddArg("-maxtipage=<n>",
                   strprintf("Maximum tip age in seconds to consider node in initial block download (default: %u)",
                             Ticks<std::chrono::seconds>(DEFAULT_MAX_TIP_AGE)),
                   ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::DEBUG_TEST);
    argsman.AddArg("-printpriority", strprintf("Log transaction fee rate in %s/kvB when mining blocks (default: %u)", CURRENCY_UNIT, DEFAULT_PRINT_MODIFIED_FEE), ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::DEBUG_TEST);
    argsman.AddArg("-uacomment=<cmt>", "Append comment to the user agent string", ArgsManager::ALLOW_ANY, OptionsCategory::DEBUG_TEST);

    SetupChainParamsBaseOptions(argsman);

    argsman.AddArg("-acceptnonstdtxn", strprintf("Relay and mine \"non-standard\" transactions (test networks only; default: %u)", DEFAULT_ACCEPT_NON_STD_TXN), ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::NODE_RELAY);
    argsman.AddArg("-incrementalrelayfee=<amt>", strprintf("Fee rate (in %s/kvB) used to define cost of relay, used for mempool limiting and replacement policy. (default: %s)", CURRENCY_UNIT, FormatMoney(DEFAULT_INCREMENTAL_RELAY_FEE)), ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::NODE_RELAY);
    argsman.AddArg("-dustrelayfee=<amt>", strprintf("Fee rate (in %s/kvB) used to define dust, the value of an output such that it will cost more than its value in fees at this fee rate to spend it. (default: %s)", CURRENCY_UNIT, FormatMoney(DUST_RELAY_TX_FEE)), ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::NODE_RELAY);
    argsman.AddArg("-acceptstalefeeestimates", strprintf("Read fee estimates even if they are stale (%sdefault: %u) fee estimates are considered stale if they are %s hours old", "regtest only; ", DEFAULT_ACCEPT_STALE_FEE_ESTIMATES, Ticks<std::chrono::hours>(MAX_FILE_AGE)), ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::DEBUG_TEST);
    argsman.AddArg("-bytespersigop", strprintf("Equivalent bytes per sigop in transactions for relay and mining (default: %u)", DEFAULT_BYTES_PER_SIGOP), ArgsManager::ALLOW_ANY, OptionsCategory::NODE_RELAY);
    argsman.AddArg("-datacarrier", strprintf("Relay and mine data carrier transactions (default: %u)", DEFAULT_ACCEPT_DATACARRIER), ArgsManager::ALLOW_ANY, OptionsCategory::NODE_RELAY);
    argsman.AddArg("-datacarriersize",
                   strprintf("Relay and mine transactions whose data-carrying raw scriptPubKeys in aggregate "
                             "are of this size or less, allowing multiple outputs (default: %u)",
                             MAX_OP_RETURN_RELAY),
                   ArgsManager::ALLOW_ANY, OptionsCategory::NODE_RELAY);
    argsman.AddArg("-permitbaremultisig", strprintf("Relay transactions creating non-P2SH multisig outputs (default: %u)", DEFAULT_PERMIT_BAREMULTISIG), ArgsManager::ALLOW_ANY,
                   OptionsCategory::NODE_RELAY);
    argsman.AddArg("-minrelaytxfee=<amt>", strprintf("Fees (in %s/kvB) smaller than this are considered zero fee for relaying, mining and transaction creation (default: %s)",
        CURRENCY_UNIT, FormatMoney(DEFAULT_MIN_RELAY_TX_FEE)), ArgsManager::ALLOW_ANY, OptionsCategory::NODE_RELAY);
    argsman.AddArg("-privatebroadcast",
                   strprintf(
                       "Broadcast transactions submitted via sendrawtransaction RPC using short-lived "
                       "connections through the Tor or I2P networks, without putting them in the mempool first. "
                       "Transactions submitted through the wallet are not affected by this option "
                       "(default: %u)",
                   DEFAULT_PRIVATE_BROADCAST),
                   ArgsManager::ALLOW_ANY,
                   OptionsCategory::NODE_RELAY);
    argsman.AddArg("-whitelistforcerelay", strprintf("Add 'forcerelay' permission to whitelisted peers with default permissions. This will relay transactions even if the transactions were already in the mempool. (default: %d)", DEFAULT_WHITELISTFORCERELAY), ArgsManager::ALLOW_ANY, OptionsCategory::NODE_RELAY);
    argsman.AddArg("-whitelistrelay", strprintf("Add 'relay' permission to whitelisted peers with default permissions. This will accept relayed transactions even when not relaying transactions (default: %d)", DEFAULT_WHITELISTRELAY), ArgsManager::ALLOW_ANY, OptionsCategory::NODE_RELAY);


    argsman.AddArg("-blockmaxweight=<n>", strprintf("Set maximum BIP141 block weight (default: %d)", DEFAULT_BLOCK_MAX_WEIGHT), ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::BLOCK_CREATION);
    argsman.AddArg("-blockreservedweight=<n>", strprintf("Reserve space for the fixed-size block header plus the largest coinbase transaction the mining software may add to the block. Only affects mining RPC clients, not IPC clients. (default: %d).", DEFAULT_BLOCK_RESERVED_WEIGHT), ArgsManager::ALLOW_ANY, OptionsCategory::BLOCK_CREATION);
    argsman.AddArg("-blockmintxfee=<amt>", strprintf("Set lowest fee rate (in %s/kvB) for transactions to be included in block creation. (default: %s)", CURRENCY_UNIT, FormatMoney(DEFAULT_BLOCK_MIN_TX_FEE)), ArgsManager::ALLOW_ANY, OptionsCategory::BLOCK_CREATION);
    argsman.AddArg("-blockversion=<n>", "Override block version to test forking scenarios", ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::BLOCK_CREATION);

    argsman.AddArg("-rest", strprintf("Accept public REST requests (default: %u)", DEFAULT_REST_ENABLE), ArgsManager::ALLOW_ANY, OptionsCategory::RPC);
    argsman.AddArg("-rpcallowip=<ip>", "Allow JSON-RPC connections from specified source. Valid values for <ip> are a single IP (e.g. 1.2.3.4), a network/netmask (e.g. 1.2.3.4/255.255.255.0), a network/CIDR (e.g. 1.2.3.4/24), all ipv4 (0.0.0.0/0), or all ipv6 (::/0). RFC4193 is allowed only if -cjdnsreachable=0. This option can be specified multiple times", ArgsManager::ALLOW_ANY, OptionsCategory::RPC);
    argsman.AddArg("-rpcauth=<userpw>", "Username and HMAC-SHA-256 hashed password for JSON-RPC connections. The field <userpw> comes in the format: <USERNAME>:<SALT>$<HASH>. A canonical python script is included in share/rpcauth. The client then connects normally using the rpcuser=<USERNAME>/rpcpassword=<PASSWORD> pair of arguments. This option can be specified multiple times", ArgsManager::ALLOW_ANY | ArgsManager::SENSITIVE, OptionsCategory::RPC);
    argsman.AddArg("-rpcbind=<addr>[:port]", "Bind to given address to listen for JSON-RPC connections. Do not expose the RPC server to untrusted networks such as the public internet! This option is ignored unless -rpcallowip is also passed. Port is optional and overrides -rpcport. Use [host]:port notation for IPv6. This option can be specified multiple times (default: 127.0.0.1 and ::1 i.e., localhost)", ArgsManager::ALLOW_ANY | ArgsManager::NETWORK_ONLY, OptionsCategory::RPC);
    argsman.AddArg("-rpcdoccheck", strprintf("Throw a non-fatal error at runtime if the documentation for an RPC is incorrect (default: %u)", DEFAULT_RPC_DOC_CHECK), ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::RPC);
    argsman.AddArg("-rpccookiefile=<loc>", "Location of the auth cookie. Relative paths will be prefixed by a net-specific datadir location. (default: data dir)", ArgsManager::ALLOW_ANY, OptionsCategory::RPC);
    argsman.AddArg("-rpccookieperms=<readable-by>", strprintf("Set permissions on the RPC auth cookie file so that it is readable by [owner|group|all] (default: owner [via umask 0077])"), ArgsManager::ALLOW_ANY, OptionsCategory::RPC);
    argsman.AddArg("-rpcpassword=<pw>", "Password for JSON-RPC connections", ArgsManager::ALLOW_ANY | ArgsManager::SENSITIVE, OptionsCategory::RPC);
    argsman.AddArg("-rpcport=<port>", strprintf("Listen for JSON-RPC connections on <port> (default: %u, testnet3: %u, testnet4: %u, signet: %u, regtest: %u)", defaultBaseParams->RPCPort(), testnetBaseParams->RPCPort(), testnet4BaseParams->RPCPort(), signetBaseParams->RPCPort(), regtestBaseParams->RPCPort()), ArgsManager::ALLOW_ANY | ArgsManager::NETWORK_ONLY, OptionsCategory::RPC);
    argsman.AddArg("-rpcservertimeout=<n>", strprintf("Timeout during HTTP requests (default: %d)", DEFAULT_HTTP_SERVER_TIMEOUT), ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::RPC);
    argsman.AddArg("-rpcthreads=<n>", strprintf("Set the number of threads to service RPC calls (default: %d)", DEFAULT_HTTP_THREADS), ArgsManager::ALLOW_ANY, OptionsCategory::RPC);
    argsman.AddArg("-rpcuser=<user>", "Username for JSON-RPC connections", ArgsManager::ALLOW_ANY | ArgsManager::SENSITIVE, OptionsCategory::RPC);
    argsman.AddArg("-rpcwhitelist=<whitelist>", "Set a whitelist to filter incoming RPC calls for a specific user. The field <whitelist> comes in the format: <USERNAME>:<rpc 1>,<rpc 2>,...,<rpc n>. If multiple whitelists are set for a given user, they are set-intersected. See -rpcwhitelistdefault documentation for information on default whitelist behavior.", ArgsManager::ALLOW_ANY, OptionsCategory::RPC);
    argsman.AddArg("-rpcwhitelistdefault", "Sets default behavior for rpc whitelisting. Unless rpcwhitelistdefault is set to 0, if any -rpcwhitelist is set, the rpc server acts as if all rpc users are subject to empty-unless-otherwise-specified whitelists. If rpcwhitelistdefault is set to 1 and no -rpcwhitelist is set, rpc server acts as if all rpc users are subject to empty whitelists.", ArgsManager::ALLOW_ANY, OptionsCategory::RPC);
    argsman.AddArg("-rpcworkqueue=<n>", strprintf("Set the maximum depth of the work queue to service RPC calls (default: %d)", DEFAULT_HTTP_WORKQUEUE), ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::RPC);
    argsman.AddArg("-server", "Accept command line and JSON-RPC commands", ArgsManager::ALLOW_ANY, OptionsCategory::RPC);
    if (can_listen_ipc) {
        argsman.AddArg("-ipcbind=<address>", "Bind to Unix socket address and listen for incoming connections. Valid address values are \"unix\" to listen on the default path, <datadir>/node.sock, or \"unix:/custom/path\" to specify a custom path. Can be specified multiple times to listen on multiple paths. Default behavior is not to listen on any path. If relative paths are specified, they are interpreted relative to the network data directory. If paths include any parent directory components and the parent directories do not exist, they will be created.", ArgsManager::ALLOW_ANY, OptionsCategory::IPC);
    }

#if HAVE_DECL_FORK
    argsman.AddArg("-daemon", strprintf("Run in the background as a daemon and accept commands (default: %d)", DEFAULT_DAEMON), ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    argsman.AddArg("-daemonwait", strprintf("Wait for initialization to be finished before exiting. This implies -daemon (default: %d)", DEFAULT_DAEMONWAIT), ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
#else
    hidden_args.emplace_back("-daemon");
    hidden_args.emplace_back("-daemonwait");
#endif

    // Add the hidden options
    argsman.AddHiddenArgs(hidden_args);
}
