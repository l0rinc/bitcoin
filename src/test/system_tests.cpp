// Copyright (c) 2019-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
//

#include <bitcoin-build-config.h> // IWYU pragma: keep

#include <common/run_command.h>
#include <test/util/common.h>
#include <test/util/setup_common.h>
#include <univalue.h>
#include <util/fs.h>
#include <util/string.h>

#if defined(ENABLE_EXTERNAL_SIGNER) || defined(ENABLE_TOR_SUBPROCESS)
#include <util/subprocess.h>
#endif

#include <boost/test/unit_test.hpp>

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>
#include <thread>

#ifndef WIN32
#include <cerrno>
#include <fcntl.h>
#include <unistd.h>
#endif

namespace {
// When set in the environment, test_bitcoin acts as a mock subprocess for the
// run_command test below instead of running unit tests.
constexpr const char* MOCK_PROCESS_ENV = "BITCOIN_TEST_MOCK_PROCESS";

const bool g_maybe_run_mock_dispatcher_before_main{[]() {
    const char* name = std::getenv(MOCK_PROCESS_ENV);
    if (!name) return false;
    const std::string_view n{name};
    if (n == "valid_json") {
        std::cout << R"({"success": true})" << std::endl;
        std::_Exit(EXIT_SUCCESS);
    }
    if (n == "nonzeroexit_nooutput") {
        std::_Exit(EXIT_FAILURE);
    }
    if (n == "nonzeroexit_stderroutput") {
        std::cerr << "err" << std::endl;
        std::_Exit(EXIT_FAILURE);
    }
    if (n == "invalid_json") {
        std::cout << "{" << std::endl;
        std::_Exit(EXIT_SUCCESS);
    }
    if (n == "pass_stdin_to_stdout") {
        std::string s;
        std::getline(std::cin, s);
        std::cout << s << std::endl;
        std::_Exit(EXIT_SUCCESS);
    }
    if (n == "sleep_forever") {
        while (true) {
            std::this_thread::sleep_for(std::chrono::hours{1});
        }
    }
#ifndef WIN32
    if (n == "fd_status") {
        const char* fd_env{std::getenv("BITCOIN_TEST_FD_TO_CHECK")};
        if (!fd_env) std::_Exit(EXIT_FAILURE);
        errno = 0;
        const bool closed{fcntl(std::stoi(fd_env), F_GETFD) == -1 && errno == EBADF};
        std::cout << (closed ? R"({"closed": true})" : R"({"closed": false})") << std::endl;
        std::_Exit(EXIT_SUCCESS);
    }
#endif
    std::cerr << "Unknown mock process: " << n << std::endl;
    std::_Exit(EXIT_FAILURE);
}()};
} // namespace

BOOST_FIXTURE_TEST_SUITE(system_tests, BasicTestingSetup)

#if defined(ENABLE_EXTERNAL_SIGNER) || defined(ENABLE_TOR_SUBPROCESS)

static std::vector<std::string> mock_executable(const std::string& name)
{
#if defined(WIN32)
    _putenv_s(MOCK_PROCESS_ENV, name.c_str());
#else
    setenv(MOCK_PROCESS_ENV, name.c_str(), /*overwrite=*/1);
#endif
    return {boost::unit_test::framework::master_test_suite().argv[0]};
}

#endif

#ifdef ENABLE_EXTERNAL_SIGNER

BOOST_AUTO_TEST_CASE(run_command)
{
    {
        const UniValue result = RunCommandParseJSON({});
        BOOST_CHECK(result.isNull());
    }
    {
        const UniValue result = RunCommandParseJSON(mock_executable("valid_json"));
        BOOST_CHECK(result.isObject());
        const UniValue& success = result.find_value("success");
        BOOST_CHECK(!success.isNull());
        BOOST_CHECK_EQUAL(success.get_bool(), true);
    }
    {
        // An invalid command is handled by cpp-subprocess
#ifdef WIN32
        const std::string expected{"CreateProcess failed: "};
#else
        const std::string expected{"execve failed: "};
#endif
        BOOST_CHECK_EXCEPTION(RunCommandParseJSON({"invalid_command"}), subprocess::CalledProcessError, HasReason(expected));
    }
    {
        // Return non-zero exit code, no output to stderr
        const std::vector<std::string> command = mock_executable("nonzeroexit_nooutput");
        BOOST_CHECK_EXCEPTION(RunCommandParseJSON(command), std::runtime_error, [&](const std::runtime_error& e) {
            const std::string what{e.what()};
            BOOST_CHECK(what.find(strprintf("RunCommandParseJSON error: process(%s) returned %d: \n", util::Join(command, " "), EXIT_FAILURE)) != std::string::npos);
            return true;
        });
    }
    {
        // Return non-zero exit code, with error message for stderr
        const std::vector<std::string> command = mock_executable("nonzeroexit_stderroutput");
        const std::string expected{"err"};
        BOOST_CHECK_EXCEPTION(RunCommandParseJSON(command), std::runtime_error, [&](const std::runtime_error& e) {
            const std::string what(e.what());
            BOOST_CHECK(what.find(strprintf("RunCommandParseJSON error: process(%s) returned %s: %s", util::Join(command, " "), EXIT_FAILURE, "err")) != std::string::npos);
            BOOST_CHECK(what.find(expected) != std::string::npos);
            return true;
        });
    }
    {
        // Unable to parse JSON
        BOOST_CHECK_EXCEPTION(RunCommandParseJSON(mock_executable("invalid_json")), std::runtime_error, HasReason("Unable to parse JSON: {"));
    }
    {
        // Test stdin
        const UniValue result = RunCommandParseJSON(mock_executable("pass_stdin_to_stdout"), "{\"success\": true}");
        BOOST_CHECK(result.isObject());
        const UniValue& success = result.find_value("success");
        BOOST_CHECK(!success.isNull());
        BOOST_CHECK_EQUAL(success.get_bool(), true);
    }
}
#endif // ENABLE_EXTERNAL_SIGNER

#if defined(ENABLE_EXTERNAL_SIGNER) || defined(ENABLE_TOR_SUBPROCESS)

BOOST_AUTO_TEST_CASE(subprocess_poll_and_kill)
{
    auto poll_until_exit = [](subprocess::Popen& process) {
        int ret{-1};
        for (int i{0}; i < 100 && ret == -1; ++i) {
            std::this_thread::sleep_for(std::chrono::milliseconds{10});
            ret = process.poll();
        }
        return ret;
    };

    subprocess::Popen exited{mock_executable("valid_json")};
    BOOST_CHECK_EQUAL(poll_until_exit(exited), 0);

    subprocess::Popen sleeper{mock_executable("sleep_forever")};
    BOOST_CHECK_EQUAL(sleeper.poll(), -1);
    sleeper.kill();
    BOOST_CHECK_EQUAL(poll_until_exit(sleeper), 9);
}

#ifndef WIN32
BOOST_AUTO_TEST_CASE(subprocess_close_fds)
{
    const fs::path fd_path{m_args.GetDataDirBase() / "subprocess_close_fds"};
    std::FILE* inherited_file{fsbridge::fopen(fd_path, "w")};
    BOOST_REQUIRE(inherited_file != nullptr);
    const int inherited_fd{fileno(inherited_file)};
    BOOST_REQUIRE_GT(inherited_fd, 2);

    const std::string fd_env{strprintf("%d", inherited_fd)};
    BOOST_REQUIRE_EQUAL(setenv("BITCOIN_TEST_FD_TO_CHECK", fd_env.c_str(), 1), 0);

    auto fd_is_closed = [&](bool close_fds) {
        subprocess::Popen child{
            mock_executable("fd_status"),
            subprocess::output{subprocess::PIPE},
            subprocess::error{subprocess::PIPE},
            subprocess::close_fds{close_fds},
        };
        const auto [out, err]{child.communicate()};
        BOOST_REQUIRE_EQUAL(child.retcode(), 0);
        BOOST_CHECK(err.buf.empty());
        UniValue result;
        BOOST_REQUIRE(result.read(std::string{out.buf.begin(), out.buf.end()}));
        return result.find_value("closed").get_bool();
    };

    BOOST_CHECK(!fd_is_closed(/*close_fds=*/false));
    BOOST_CHECK(fd_is_closed(/*close_fds=*/true));

#ifdef ENABLE_EXTERNAL_SIGNER
    const UniValue result = RunCommandParseJSON(mock_executable("fd_status"));
    BOOST_CHECK(result.find_value("closed").get_bool());
#endif

    unsetenv("BITCOIN_TEST_FD_TO_CHECK");
    BOOST_CHECK_EQUAL(std::fclose(inherited_file), 0);
}
#endif

#endif

BOOST_AUTO_TEST_SUITE_END()
