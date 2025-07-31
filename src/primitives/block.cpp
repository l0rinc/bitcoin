// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <primitives/block.h>

#include <hash.h>
#include <logging.h>
#include <tinyformat.h>

#include <util/hasher.h>
#include <execinfo.h>
#include <cxxabi.h>
#include <mutex>
#include <unordered_map>
#include <cstring>      // std::memcpy
#include <dlfcn.h>
#include <dlfcn.h>
#include <cxxabi.h>
#include <iostream>

namespace {

[[nodiscard]] std::string GetStackSignature()
{
    constexpr int kSkip{1};
    constexpr int kDepth{3};

    void* buf[kSkip + kDepth];
    int captured = ::backtrace(buf, kSkip + kDepth);
    if (captured <= kSkip) return "unknown";

    std::string sig;

    for (int i{captured - 1}; i > 0; --i) {
        Dl_info info{};
        std::string name{"unknown"};

        if (dladdr(buf[i], &info) && info.dli_sname) {
            int status{};
            char* demangled = abi::__cxa_demangle(info.dli_sname, nullptr, nullptr, &status);
            name = (status == 0 && demangled) ? demangled : info.dli_sname;
            free(demangled);
        }

        /*  ─── post-process ─── */
        // strip parameter list
        if (auto pos = name.find('('); pos != std::string::npos) name.erase(pos);
        // strip namespaces / class qualifiers
        if (auto pos = name.rfind("::"); pos != std::string::npos) name.erase(0, pos + 2);
        // strip template arguments
        if (auto pos = name.find('<'); pos != std::string::npos) name.erase(pos);

        sig += name.empty() ? "unknown" : name + "()";
        if (i > 1) sig += "->";
    }
    return sig;
}

static std::mutex g_mutex;
static std::unordered_map<std::string, std::size_t> g_stack_count;
static std::unordered_map<uint256, std::unordered_map<std::string, std::size_t>, SaltedSipHasher> g_hash_stack_count;
static std::unordered_map<uint256, std::size_t, SaltedSipHasher> g_hash_total;
} // namespace

uint256 CBlockHeader::GetHash() const
{
    const auto hash{(HashWriter{} << *this).GetHash()};
    {
        std::lock_guard l{g_mutex};
        ++g_hash_total[hash];
    }

    if (g_hash_total[hash] > 100) {
        const auto stack_sig{GetStackSignature()};
        {
            std::lock_guard l{g_mutex};
            ++g_stack_count[stack_sig];
            ++g_hash_stack_count[hash][stack_sig];
        }

        LogPrintLevel_(BCLog::LogFlags::ALL, BCLog::Level::Info, /*should_ratelimit=*/false, "GetHash() called %zu times for %s from %s", g_hash_total[hash], hash.ToString(), stack_sig);
    }

    return hash;
}

std::string CBlock::ToString() const
{
    std::stringstream s;
    s << strprintf("CBlock(hash=%s, ver=0x%08x, hashPrevBlock=%s, hashMerkleRoot=%s, nTime=%u, nBits=%08x, nNonce=%u, vtx=%u)\n",
        GetHash().ToString(),
        nVersion,
        hashPrevBlock.ToString(),
        hashMerkleRoot.ToString(),
        nTime, nBits, nNonce,
        vtx.size());
    for (const auto& tx : vtx) {
        s << "  " << tx->ToString() << "\n";
    }
    return s.str();
}
