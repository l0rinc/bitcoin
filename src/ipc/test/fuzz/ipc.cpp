// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <primitives/transaction.h>
#include <capnp/capability.h>
#include <capnp/rpc.h>
#include <kj/memory.h>
#include <mp/proxy-io.h>
#include <mp/proxy.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <ipc/test/fuzz/ipc_fuzz.capnp.h>
#include <ipc/test/fuzz/ipc_fuzz.capnp.proxy.h>
#include <ipc/test/fuzz/ipc_fuzz.h>
#include <test/fuzz/util.h>
#include <test/util/setup_common.h>

#include <algorithm>
#include <cstring>
#include <future>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>

namespace {
class RawIpcError : public std::runtime_error
{
    using std::runtime_error::runtime_error;
};

constexpr std::string_view INVALID_UNIVALUE_JSON_ERROR{"Invalid UniValue JSON received over IPC"};

class IpcFuzzSetup
{
public:
    IpcFuzzSetup()
    {
        std::promise<std::unique_ptr<mp::ProxyClient<test::fuzz::messages::IpcFuzzInterface>>> client_promise;
        auto client_future{client_promise.get_future()};
        m_loop_thread = std::thread([&client_promise] {
            mp::EventLoop loop("ipc-fuzz", [](mp::LogMessage message) {
                if (message.level == mp::Log::Raise) throw std::runtime_error(message.message);
            });
            auto pipe = loop.m_io_context.provider->newTwoWayPipe();

            auto server_connection = std::make_unique<mp::Connection>(
                loop,
                kj::mv(pipe.ends[0]),
                [&](mp::Connection& connection) {
                    auto server_proxy = kj::heap<mp::ProxyServer<test::fuzz::messages::IpcFuzzInterface>>(
                        std::make_shared<IpcFuzzImplementation>(), connection);
                    return capnp::Capability::Client(kj::mv(server_proxy));
                });
            server_connection->onDisconnect([&] { server_connection.reset(); });

            auto client_connection = std::make_unique<mp::Connection>(loop, kj::mv(pipe.ends[1]));
            auto client_proxy = std::make_unique<mp::ProxyClient<test::fuzz::messages::IpcFuzzInterface>>(
                client_connection->m_rpc_system->bootstrap(mp::ServerVatId().vat_id)
                    .castAs<test::fuzz::messages::IpcFuzzInterface>(),
                client_connection.get(),
                /* destroy_connection= */ true);
            (void)client_connection.release();

            client_promise.set_value(std::move(client_proxy));
            loop.loop();
        });
        m_client = client_future.get();
    }

    ~IpcFuzzSetup()
    {
        m_client.reset();
        if (m_loop_thread.joinable()) m_loop_thread.join();
    }

    std::string RawPassUniValue(std::string_view arg)
    {
        std::promise<std::string> result_promise;
        auto result_future{result_promise.get_future()};
        m_client->m_context.loop->sync([&] {
            auto request{m_client->m_client.passUniValueRequest()};
            auto arg_builder{request.initArg(arg.size())};
            std::memcpy(arg_builder.begin(), arg.data(), arg.size());
            m_client->m_context.loop->m_task_set->add(request.send().then(
                [&result_promise](::capnp::Response<test::fuzz::messages::IpcFuzzInterface::PassUniValueResults>&& response) {
                    const auto result{response.getResult()};
                    result_promise.set_value(std::string{result.cStr(), result.size()});
                },
                [&result_promise](const ::kj::Exception& e) {
                    result_promise.set_exception(std::make_exception_ptr(RawIpcError{std::string{e.getDescription().cStr()}}));
                }));
        });
        return result_future.get();
    }

    std::unique_ptr<mp::ProxyClient<test::fuzz::messages::IpcFuzzInterface>> m_client;

private:
    std::thread m_loop_thread;
};

static IpcFuzzSetup* g_ipc;

static void initialize_ipc()
{
    static const auto testing_setup = MakeNoLogFileContext<>();
    (void)testing_setup;

    // Ensure g_thread_context is destroyed after the IPC setup, since C++
    // destroys thread_local objects in reverse construction order.
    mp::ThreadContext& thread_context{mp::g_thread_context};
    (void)thread_context;

    thread_local static IpcFuzzSetup ipc; // NOLINT(bitcoin-nontrivial-threadlocal)
    g_ipc = &ipc;
}

FUZZ_TARGET(ipc, .init = initialize_ipc)
{
    auto& ipc = *g_ipc;
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    assert(ipc.m_client->passVectorUint8({}).empty());

    LIMITED_WHILE(fuzzed_data_provider.ConsumeBool(), 64)
    {
        CallOneOf(
            fuzzed_data_provider,
            [&] {
                static constexpr int MIN_ADD{-1'000'000};
                static constexpr int MAX_ADD{1'000'000};
                const int a = fuzzed_data_provider.ConsumeIntegralInRange<int>(MIN_ADD, MAX_ADD);
                const int b = fuzzed_data_provider.ConsumeIntegralInRange<int>(MIN_ADD, MAX_ADD);
                assert(ipc.m_client->add(a, b) == a + b);
            },
            [&] {
                COutPoint outpoint{Txid::FromUint256(ConsumeUInt256(fuzzed_data_provider)),
                                   fuzzed_data_provider.ConsumeIntegral<uint32_t>()};
                COutPoint expected{outpoint.hash, outpoint.n ^ 0xFFFFFFFFu};
                assert(ipc.m_client->passOutPoint(outpoint) == expected);
            },
            [&] {
                std::vector<uint8_t> value = ConsumeRandomLengthByteVector<uint8_t>(fuzzed_data_provider, 512);
                std::vector<uint8_t> expected{value.rbegin(), value.rend()};
                assert(ipc.m_client->passVectorUint8(value) == expected);
            },
            [&] {
                CScript script{ConsumeScript(fuzzed_data_provider)};
                CScript expected{script};
                expected << OP_NOP;
                assert(ipc.m_client->passScript(script) == expected);
            },
            [&] {
                UniValue value;
                const std::string json{fuzzed_data_provider.ConsumeRandomLengthString(512)};
                const bool valid_text{std::ranges::all_of(json, [](const unsigned char c) {
                    return c == '\t' || c == '\n' || c == '\r' || (c >= 0x20 && c <= 0x7e);
                })};
                if (!valid_text) return;
                const bool valid_json{value.read(json)};
                if (valid_json) {
                    assert(ipc.m_client->passUniValue(value).write() == value.write());
                    assert(ipc.RawPassUniValue(json) == value.write());
                } else {
                    try {
                        (void)ipc.RawPassUniValue(json);
                    } catch (const RawIpcError& e) {
                        assert(std::string_view{e.what()}.find(INVALID_UNIVALUE_JSON_ERROR) != std::string_view::npos);
                        return;
                    }
                    assert(false);
                }
            },
            [&] {
                const CMutableTransaction mutable_tx = ConsumeTransaction(fuzzed_data_provider, std::nullopt);
                if (mutable_tx.vin.empty()) return;
                const CTransactionRef tx = MakeTransactionRef(mutable_tx);
                assert(*ipc.m_client->passTransaction(tx) == *tx);
            });
    }
}
} // namespace
