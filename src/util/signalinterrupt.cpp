// Copyright (c) 2022-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <util/signalinterrupt.h>

#ifdef WIN32
#include <mutex>
#else
#include <util/tokenpipe.h>
#endif

#include <ios>
#include <optional>

namespace util {

SignalInterrupt::SignalInterrupt() : m_flag{false}
{
#ifndef WIN32
    std::optional<TokenPipe> pipe = TokenPipe::Make();
    if (!pipe) throw std::ios_base::failure("Could not create TokenPipe");
    m_pipe_r = pipe->TakeReadEnd();
    m_pipe_w = pipe->TakeWriteEnd();
#endif
}

SignalInterrupt::operator bool() const
{
    return m_flag;
}

bool SignalInterrupt::reset()
{
#ifdef WIN32
    // Keep the flag clear operation under the same mutex as operator() so a concurrent
    // console callback cannot be cleared after this function returns.
    std::lock_guard<std::mutex> lock{m_mutex};
    m_flag = false;
#else
    // Clear before draining the old token so a concurrent signal handler publishes a new token
    // instead of observing the old true flag and being lost by the reset.
    if (!m_flag.exchange(false, std::memory_order_acq_rel)) return true;
    if (!wait()) return false;
#endif
    return true;
}

bool SignalInterrupt::operator()()
{
#ifdef WIN32
    std::unique_lock<std::mutex> lk(m_mutex);
    m_flag = true;
    m_cv.notify_one();
#else
    // This must be reentrant and safe for calling in a signal handler, so using a condition variable is not safe.
    // Make sure that the token is only written once even if multiple threads call this concurrently or in
    // case of a reentrant signal.
    if (!m_flag.exchange(true)) {
        // Write an arbitrary byte to the write end of the pipe.
        int res = m_pipe_w.TokenWrite('x');
        if (res != 0) {
            return false;
        }
    }
#endif
    return true;
}

bool SignalInterrupt::wait()
{
#ifdef WIN32
    std::unique_lock<std::mutex> lk(m_mutex);
    m_cv.wait(lk, [this] { return m_flag.load(); });
#else
    int res = m_pipe_r.TokenRead();
    if (res != 'x') {
        return false;
    }
#endif
    return true;
}

} // namespace util
