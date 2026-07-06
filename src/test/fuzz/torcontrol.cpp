// Copyright (c) 2020-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/util/setup_common.h>
#include <test/util/torcontrol.h>
#include <torcontrol.h>
#include <util/strencodings.h>
#include <util/string.h>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {
constexpr size_t MAX_TOR_LINE_LENGTH{100000};
constexpr size_t MAX_TOR_LINE_COUNT{1000};

struct TorProcessBufferModel {
    std::optional<std::string> error;
    size_t consumed{0};
    size_t remaining_handlers{0};
    std::vector<TorControlReply> handled_replies;
};

bool IsExpectedProcessBufferRuntimeError(std::string_view error)
{
    return error == "max_line_length exceeded by LineReader" ||
           error == "Control port reply exceeded 1000 lines, disconnecting";
}

TorProcessBufferModel ModelTorProcessBuffer(const std::vector<std::byte>& recv_buffer, size_t handler_count)
{
    TorProcessBufferModel model;
    model.remaining_handlers = handler_count;

    TorControlReply message;
    util::LineReader reader(recv_buffer, MAX_TOR_LINE_LENGTH);
    try {
        while (auto line = reader.ReadLine()) {
            if (message.lines.size() == MAX_TOR_LINE_COUNT) {
                throw std::runtime_error("Control port reply exceeded 1000 lines, disconnecting");
            }
            if (line->size() < 4) continue;

            message.code = ToIntegral<int>(line->substr(0, 3)).value_or(0);
            message.lines.emplace_back(line->substr(4));
            if ((*line)[3] == ' ') {
                if (message.code < 600 && model.remaining_handlers > 0) {
                    model.handled_replies.push_back(message);
                    --model.remaining_handlers;
                }
                message.Clear();
            }
        }
        model.consumed = reader.Consumed();
    } catch (const std::runtime_error& e) {
        assert(IsExpectedProcessBufferRuntimeError(e.what()));
        model.error = e.what();
    }

    return model;
}

void AssertProcessBufferContracts(const std::vector<std::byte>& recv_buffer, size_t handler_count)
{
    CThreadInterrupt interrupt;
    TorControlConnection conn{interrupt};
    TorControlConnectionTest::ReceiveBuffer(conn) = recv_buffer;

    std::vector<TorControlReply> handled_replies;
    for (size_t i{0}; i < handler_count; ++i) {
        TorControlConnectionTest::ReplyHandlers(conn).emplace_back(
            [&](TorControlConnection&, const TorControlReply& reply) {
                handled_replies.push_back(reply);
            });
    }

    const TorProcessBufferModel model{ModelTorProcessBuffer(recv_buffer, handler_count)};
    try {
        assert(TorControlConnectionTest::ProcessBuffer(conn));
        assert(!model.error);
    } catch (const std::runtime_error& e) {
        assert(IsExpectedProcessBufferRuntimeError(e.what()));
        assert(model.error);
        assert(*model.error == e.what());
        assert(TorControlConnectionTest::ReceiveBuffer(conn) == recv_buffer);
        return;
    }

    assert(model.consumed <= recv_buffer.size());
    const std::vector<std::byte> expected_remaining{recv_buffer.begin() + model.consumed, recv_buffer.end()};
    assert(TorControlConnectionTest::ReceiveBuffer(conn) == expected_remaining);
    assert(TorControlConnectionTest::ReplyHandlers(conn).size() == model.remaining_handlers);
    assert(handled_replies.size() == model.handled_replies.size());
    for (size_t i{0}; i < handled_replies.size(); ++i) {
        assert(handled_replies[i].code == model.handled_replies[i].code);
        assert(handled_replies[i].lines == model.handled_replies[i].lines);
    }
}
} // namespace

void initialize_torcontrol()
{
    static const auto testing_setup = MakeNoLogFileContext<>();
}

FUZZ_TARGET(torcontrol, .init = initialize_torcontrol)
{
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};

    TorController tor_controller;
    CThreadInterrupt interrupt;
    TorControlConnection conn{interrupt};

    AssertProcessBufferContracts(
        ConsumeRandomLengthByteVector<std::byte>(fuzzed_data_provider, /*max_length=*/4000),
        fuzzed_data_provider.ConsumeIntegralInRange<size_t>(0, 64));

    LIMITED_WHILE(fuzzed_data_provider.ConsumeBool(), 10000)
    {
        TorControlReply tor_control_reply;
        CallOneOf(
            fuzzed_data_provider,
            [&] {
                tor_control_reply.code = TOR_REPLY_OK;
            },
            [&] {
                tor_control_reply.code = TOR_REPLY_UNRECOGNIZED;
            },
            [&] {
                tor_control_reply.code = TOR_REPLY_SYNTAX_ERROR;
            },
            [&] {
                tor_control_reply.code = fuzzed_data_provider.ConsumeIntegral<int>();
            });
        tor_control_reply.lines = ConsumeRandomLengthStringVector(fuzzed_data_provider);

        CallOneOf(
            fuzzed_data_provider,
            [&] {
                tor_controller.add_onion_cb(conn, tor_control_reply, /*pow_was_enabled=*/true);
            },
            [&] {
                tor_controller.add_onion_cb(conn, tor_control_reply, /*pow_was_enabled=*/false);
            },
            [&] {
                tor_controller.auth_cb(conn, tor_control_reply);
            },
            [&] {
                tor_controller.authchallenge_cb(conn, tor_control_reply);
            },
            [&] {
                tor_controller.protocolinfo_cb(conn, tor_control_reply);
            },
            [&] {
                tor_controller.get_socks_cb(conn, tor_control_reply);
            });
    }
}
