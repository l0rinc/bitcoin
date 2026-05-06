// Copyright (c) 2015-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_HTTPRPC_H
#define BITCOIN_HTTPRPC_H

#include <any>

/** Start HTTP RPC subsystem.
 * Registers JSON-RPC HTTP handlers after `InitHTTPServer()` and `StartRPC()`,
 * and before `StartHTTPServer()`.
 */
bool StartHTTPRPC(const std::any& context);
/** Interrupt HTTP RPC subsystem.
 */
void InterruptHTTPRPC();
/** Stop HTTP RPC subsystem.
 * Unregisters JSON-RPC HTTP handlers before `StopRPC()` and
 * `StopHTTPServer()`.
 */
void StopHTTPRPC();

/** Start HTTP REST subsystem.
 * Registers REST HTTP handlers after `InitHTTPServer()` and before
 * `StartHTTPServer()`.
 */
void StartREST(const std::any& context);
/** Interrupt RPC REST subsystem.
 */
void InterruptREST();
/** Stop HTTP REST subsystem.
 * Unregisters REST HTTP handlers before `StopHTTPServer()`.
 */
void StopREST();

#endif // BITCOIN_HTTPRPC_H
