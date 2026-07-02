// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_TEST_UTIL_TORCONTROL_H
#define BITCOIN_TEST_UTIL_TORCONTROL_H

#include <torcontrol.h>

#include <cstddef>
#include <deque>
#include <vector>

struct TorControlConnectionTest
{
    static std::vector<std::byte>& ReceiveBuffer(TorControlConnection& conn) { return conn.m_recv_buffer; }
    static TorControlReply& Message(TorControlConnection& conn) { return conn.m_message; }
    static std::deque<TorControlConnection::ReplyHandlerCB>& ReplyHandlers(TorControlConnection& conn) { return conn.m_reply_handlers; }
    static bool ProcessBuffer(TorControlConnection& conn) { return conn.ProcessBuffer(); }
};

#endif // BITCOIN_TEST_UTIL_TORCONTROL_H
