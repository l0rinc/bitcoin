// Copyright (c) 2015-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_UTIL_READWRITEFILE_H
#define BITCOIN_UTIL_READWRITEFILE_H

#include <util/fs.h>

#include <limits>
#include <string>
#include <utility>

/** Read file contents into a std::string, up to maxsize bytes.
 * Returns a pair <status, string>.
 * If an error occurred, status will be false, otherwise status will be true and the data will be returned in string.
 *
 * @param filename Path of the file to read.
 * @param maxsize Maximum number of bytes to read. If the file is larger than this, the returned string is truncated
 *         to maxsize bytes.
 */
std::pair<bool,std::string> ReadBinaryFile(const fs::path &filename, size_t maxsize=std::numeric_limits<size_t>::max());

/** Write contents of std::string to a file.
 * @return true on success.
 */
bool WriteBinaryFile(const fs::path &filename, const std::string &data);

#endif // BITCOIN_UTIL_READWRITEFILE_H
