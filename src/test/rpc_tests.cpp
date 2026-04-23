// Copyright (c) 2012-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <core_io.h>
#include <interfaces/chain.h>
#include <node/context.h>
#include <rpc/blockchain.h>
#include <rpc/client.h>
#include <rpc/server.h>
#include <rpc/util.h>
#include <test/util/common.h>
#include <test/util/setup_common.h>
#include <univalue.h>
#include <util/time.h>

#include <any>
#include <string_view>

#include <boost/test/unit_test.hpp>

using util::SplitString;

static UniValue JSON(std::string_view json)
{
    UniValue value;
    CHECK(value.read(json));
    return value;
}

class HasJSON
{
public:
    explicit HasJSON(std::string json) : m_json(std::move(json)) {}
    bool operator()(const UniValue& value) const
    {
        std::string json{value.write()};
        CHECK_EQUAL(json, m_json);
        return json == m_json;
    };

private:
    const std::string m_json;
};

class RPCTestingSetup : public TestingSetup
{
public:
    UniValue TransformParams(const UniValue& params, std::vector<std::pair<std::string, bool>> arg_names) const;
    UniValue CallRPC(std::string args);
};

UniValue RPCTestingSetup::TransformParams(const UniValue& params, std::vector<std::pair<std::string, bool>> arg_names) const
{
    UniValue transformed_params;
    CRPCTable table;
    CRPCCommand command{"category", "method", [&](const JSONRPCRequest& request, UniValue&, bool) -> bool { transformed_params = request.params; return true; }, arg_names, /*unique_id=*/0};
    table.appendCommand("method", &command);
    JSONRPCRequest request;
    request.strMethod = "method";
    request.params = params;
    if (RPCIsInWarmup(nullptr)) SetRPCWarmupFinished();
    table.execute(request);
    return transformed_params;
}

UniValue RPCTestingSetup::CallRPC(std::string args)
{
    std::vector<std::string> vArgs{SplitString(args, ' ')};
    std::string strMethod = vArgs[0];
    vArgs.erase(vArgs.begin());
    JSONRPCRequest request;
    request.context = &m_node;
    request.strMethod = strMethod;
    request.params = RPCConvertValues(strMethod, vArgs);
    if (RPCIsInWarmup(nullptr)) SetRPCWarmupFinished();
    try {
        UniValue result = tableRPC.execute(request);
        return result;
    }
    catch (const UniValue& objError) {
        throw std::runtime_error(objError.find_value("message").get_str());
    }
}


BOOST_FIXTURE_TEST_SUITE(rpc_tests, RPCTestingSetup)

BOOST_AUTO_TEST_CASE(rpc_namedparams)
{
    const std::vector<std::pair<std::string, bool>> arg_names{{"arg1", false}, {"arg2", false}, {"arg3", false}, {"arg4", false}, {"arg5", false}};

    // Make sure named arguments are transformed into positional arguments in correct places separated by nulls
    CHECK_EQUAL(TransformParams(JSON(R"({"arg2": 2, "arg4": 4})"), arg_names).write(), std::string_view{"[null,2,null,4]"});

    // Make sure named argument specified multiple times raises an exception
    CHECK_EXCEPTION(TransformParams(JSON(R"({"arg2": 2, "arg2": 4})"), arg_names), UniValue,
                          HasJSON(R"({"code":-8,"message":"Parameter arg2 specified multiple times"})"));

    // Make sure named and positional arguments can be combined.
    CHECK_EQUAL(TransformParams(JSON(R"({"arg5": 5, "args": [1, 2], "arg4": 4})"), arg_names).write(), std::string_view{"[1,2,null,4,5]"});

    // Make sure a unknown named argument raises an exception
    CHECK_EXCEPTION(TransformParams(JSON(R"({"arg2": 2, "unknown": 6})"), arg_names), UniValue,
                          HasJSON(R"({"code":-8,"message":"Unknown named parameter unknown"})"));

    // Make sure an overlap between a named argument and positional argument raises an exception
    CHECK_EXCEPTION(TransformParams(JSON(R"({"args": [1,2,3], "arg4": 4, "arg2": 2})"), arg_names), UniValue,
                          HasJSON(R"({"code":-8,"message":"Parameter arg2 specified twice both as positional and named argument"})"));

    // Make sure extra positional arguments can be passed through to the method implementation, as long as they don't overlap with named arguments.
    CHECK_EQUAL(TransformParams(JSON(R"({"args": [1,2,3,4,5,6,7,8,9,10]})"), arg_names).write(), std::string_view{"[1,2,3,4,5,6,7,8,9,10]"});
    CHECK_EQUAL(TransformParams(JSON(R"([1,2,3,4,5,6,7,8,9,10])"), arg_names).write(), std::string_view{"[1,2,3,4,5,6,7,8,9,10]"});
}

BOOST_AUTO_TEST_CASE(rpc_namedonlyparams)
{
    const std::vector<std::pair<std::string, bool>> arg_names{{"arg1", false}, {"arg2", false}, {"opt1", true}, {"opt2", true}, {"options", false}};

    // Make sure optional parameters are really optional.
    CHECK_EQUAL(TransformParams(JSON(R"({"arg1": 1, "arg2": 2})"), arg_names).write(), std::string_view{"[1,2]"});

    // Make sure named-only parameters are passed as options.
    CHECK_EQUAL(TransformParams(JSON(R"({"arg1": 1, "arg2": 2, "opt1": 10, "opt2": 20})"), arg_names).write(), std::string_view{R"([1,2,{"opt1":10,"opt2":20}])"});

    // Make sure options can be passed directly.
    CHECK_EQUAL(TransformParams(JSON(R"({"arg1": 1, "arg2": 2, "options":{"opt1": 10, "opt2": 20}})"), arg_names).write(), std::string_view{R"([1,2,{"opt1":10,"opt2":20}])"});

    // Make sure options and named parameters conflict.
    CHECK_EXCEPTION(TransformParams(JSON(R"({"arg1": 1, "arg2": 2, "opt1": 10, "options":{"opt1": 10}})"), arg_names), UniValue,
                          HasJSON(R"({"code":-8,"message":"Parameter options conflicts with parameter opt1"})"));

    // Make sure options object specified through args array conflicts.
    CHECK_EXCEPTION(TransformParams(JSON(R"({"args": [1, 2, {"opt1": 10}], "opt2": 20})"), arg_names), UniValue,
                          HasJSON(R"({"code":-8,"message":"Parameter options specified twice both as positional and named argument"})"));
}

BOOST_AUTO_TEST_CASE(rpc_rawparams)
{
    // Test raw transaction API argument handling
    UniValue r;

    CHECK_THROW(CallRPC("getrawtransaction"), std::runtime_error);
    CHECK_THROW(CallRPC("getrawtransaction not_hex"), std::runtime_error);
    CHECK_THROW(CallRPC("getrawtransaction a3b807410df0b60fcb9736768df5823938b2f838694939ba45f3c0a1bff150ed not_int"), std::runtime_error);

    CHECK_THROW(CallRPC("createrawtransaction"), std::runtime_error);
    CHECK_THROW(CallRPC("createrawtransaction null null"), std::runtime_error);
    CHECK_THROW(CallRPC("createrawtransaction not_array"), std::runtime_error);
    CHECK_THROW(CallRPC("createrawtransaction {} {}"), std::runtime_error);
    CHECK_NO_THROW(CallRPC("createrawtransaction [] {}"));
    CHECK_THROW(CallRPC("createrawtransaction [] {} extra"), std::runtime_error);

    CHECK_THROW(CallRPC("decoderawtransaction"), std::runtime_error);
    CHECK_THROW(CallRPC("decoderawtransaction null"), std::runtime_error);
    CHECK_THROW(CallRPC("decoderawtransaction DEADBEEF"), std::runtime_error);
    std::string rawtx = "0100000001a15d57094aa7a21a28cb20b59aab8fc7d1149a3bdbcddba9c622e4f5f6a99ece010000006c493046022100f93bb0e7d8db7bd46e40132d1f8242026e045f03a0efe71bbb8e3f475e970d790221009337cd7f1f929f00cc6ff01f03729b069a7c21b59b1736ddfee5db5946c5da8c0121033b9b137ee87d5a812d6f506efdd37f0affa7ffc310711c06c7f3e097c9447c52ffffffff0100e1f505000000001976a9140389035a9225b3839e2bbf32d826a1e222031fd888ac00000000";
    CHECK_NO_THROW(r = CallRPC(std::string("decoderawtransaction ")+rawtx));
    CHECK_EQUAL(r.get_obj().find_value("size").getInt<int>(), std::remove_cvref_t<decltype(r.get_obj().find_value("size").getInt<int>())>{193});
    CHECK_EQUAL(r.get_obj().find_value("version").getInt<int>(), std::remove_cvref_t<decltype(r.get_obj().find_value("version").getInt<int>())>{1});
    CHECK_EQUAL(r.get_obj().find_value("locktime").getInt<int>(), std::remove_cvref_t<decltype(r.get_obj().find_value("locktime").getInt<int>())>{0});
    CHECK_THROW(CallRPC(std::string("decoderawtransaction ")+rawtx+" extra"), std::runtime_error);
    CHECK_NO_THROW(r = CallRPC(std::string("decoderawtransaction ")+rawtx+" false"));
    CHECK_THROW(r = CallRPC(std::string("decoderawtransaction ")+rawtx+" false extra"), std::runtime_error);

    // Only check failure cases for sendrawtransaction, there's no network to send to...
    CHECK_THROW(CallRPC("sendrawtransaction"), std::runtime_error);
    CHECK_THROW(CallRPC("sendrawtransaction null"), std::runtime_error);
    CHECK_THROW(CallRPC("sendrawtransaction DEADBEEF"), std::runtime_error);
    CHECK_THROW(CallRPC(std::string("sendrawtransaction ")+rawtx+" extra"), std::runtime_error);
}

BOOST_AUTO_TEST_CASE(rpc_togglenetwork)
{
    UniValue r;

    r = CallRPC("getnetworkinfo");
    bool netState = r.get_obj().find_value("networkactive").get_bool();
    CHECK_EQUAL(netState, std::remove_cvref_t<decltype(netState)>{true});

    CHECK_NO_THROW(CallRPC("setnetworkactive false"));
    r = CallRPC("getnetworkinfo");
    int numConnection = r.get_obj().find_value("connections").getInt<int>();
    CHECK_EQUAL(numConnection, std::remove_cvref_t<decltype(numConnection)>{0});

    netState = r.get_obj().find_value("networkactive").get_bool();
    CHECK_EQUAL(netState, std::remove_cvref_t<decltype(netState)>{false});

    CHECK_NO_THROW(CallRPC("setnetworkactive true"));
    r = CallRPC("getnetworkinfo");
    netState = r.get_obj().find_value("networkactive").get_bool();
    CHECK_EQUAL(netState, std::remove_cvref_t<decltype(netState)>{true});
}

BOOST_AUTO_TEST_CASE(rpc_rawsign)
{
    UniValue r;
    // input is a 1-of-2 multisig (so is output):
    std::string prevout =
      "[{\"txid\":\"b4cc287e58f87cdae59417329f710f3ecd75a4ee1d2872b7248f50977c8493f3\","
      "\"vout\":1,\"scriptPubKey\":\"a914b10c9df5f7edf436c697f02f1efdba4cf399615187\","
      "\"redeemScript\":\"512103debedc17b3df2badbcdd86d5feb4562b86fe182e5998abd8bcd4f122c6155b1b21027e940bb73ab8732bfdf7f9216ecefca5b94d6df834e77e108f68e66f126044c052ae\"}]";
    r = CallRPC(std::string("createrawtransaction ")+prevout+" "+
      "{\"3HqAe9LtNBjnsfM4CyYaWTnvCaUYT7v4oZ\":11}");
    std::string notsigned = r.get_str();
    std::string privkey1 = "\"KzsXybp9jX64P5ekX1KUxRQ79Jht9uzW7LorgwE65i5rWACL6LQe\"";
    std::string privkey2 = "\"Kyhdf5LuKTRx4ge69ybABsiUAWjVRK4XGxAKk2FQLp2HjGMy87Z4\"";
    r = CallRPC(std::string("signrawtransactionwithkey ")+notsigned+" [] "+prevout);
    CHECK(r.get_obj().find_value("complete").get_bool() == false);
    r = CallRPC(std::string("signrawtransactionwithkey ")+notsigned+" ["+privkey1+","+privkey2+"] "+prevout);
    CHECK(r.get_obj().find_value("complete").get_bool() == true);
}

BOOST_AUTO_TEST_CASE(rpc_createraw_op_return)
{
    CHECK_NO_THROW(CallRPC("createrawtransaction [{\"txid\":\"a3b807410df0b60fcb9736768df5823938b2f838694939ba45f3c0a1bff150ed\",\"vout\":0}] {\"data\":\"68656c6c6f776f726c64\"}"));

    // Key not "data" (bad address)
    CHECK_THROW(CallRPC("createrawtransaction [{\"txid\":\"a3b807410df0b60fcb9736768df5823938b2f838694939ba45f3c0a1bff150ed\",\"vout\":0}] {\"somedata\":\"68656c6c6f776f726c64\"}"), std::runtime_error);

    // Bad hex encoding of data output
    CHECK_THROW(CallRPC("createrawtransaction [{\"txid\":\"a3b807410df0b60fcb9736768df5823938b2f838694939ba45f3c0a1bff150ed\",\"vout\":0}] {\"data\":\"12345\"}"), std::runtime_error);
    CHECK_THROW(CallRPC("createrawtransaction [{\"txid\":\"a3b807410df0b60fcb9736768df5823938b2f838694939ba45f3c0a1bff150ed\",\"vout\":0}] {\"data\":\"12345g\"}"), std::runtime_error);

    // Data 81 bytes long
    CHECK_NO_THROW(CallRPC("createrawtransaction [{\"txid\":\"a3b807410df0b60fcb9736768df5823938b2f838694939ba45f3c0a1bff150ed\",\"vout\":0}] {\"data\":\"010203040506070809101112131415161718192021222324252627282930313233343536373839404142434445464748495051525354555657585960616263646566676869707172737475767778798081\"}"));
}

BOOST_AUTO_TEST_CASE(rpc_format_monetary_values)
{
    CHECK(ValueFromAmount(0LL).write() == "0.00000000");
    CHECK(ValueFromAmount(1LL).write() == "0.00000001");
    CHECK(ValueFromAmount(17622195LL).write() == "0.17622195");
    CHECK(ValueFromAmount(50000000LL).write() == "0.50000000");
    CHECK(ValueFromAmount(89898989LL).write() == "0.89898989");
    CHECK(ValueFromAmount(100000000LL).write() == "1.00000000");
    CHECK(ValueFromAmount(2099999999999990LL).write() == "20999999.99999990");
    CHECK(ValueFromAmount(2099999999999999LL).write() == "20999999.99999999");

    CHECK_EQUAL(ValueFromAmount(0).write(), std::string_view{"0.00000000"});
    CHECK_EQUAL(ValueFromAmount((COIN/10000)*123456789).write(), std::string_view{"12345.67890000"});
    CHECK_EQUAL(ValueFromAmount(-COIN).write(), std::string_view{"-1.00000000"});
    CHECK_EQUAL(ValueFromAmount(-COIN/10).write(), std::string_view{"-0.10000000"});

    CHECK_EQUAL(ValueFromAmount(COIN*100000000).write(), std::string_view{"100000000.00000000"});
    CHECK_EQUAL(ValueFromAmount(COIN*10000000).write(), std::string_view{"10000000.00000000"});
    CHECK_EQUAL(ValueFromAmount(COIN*1000000).write(), std::string_view{"1000000.00000000"});
    CHECK_EQUAL(ValueFromAmount(COIN*100000).write(), std::string_view{"100000.00000000"});
    CHECK_EQUAL(ValueFromAmount(COIN*10000).write(), std::string_view{"10000.00000000"});
    CHECK_EQUAL(ValueFromAmount(COIN*1000).write(), std::string_view{"1000.00000000"});
    CHECK_EQUAL(ValueFromAmount(COIN*100).write(), std::string_view{"100.00000000"});
    CHECK_EQUAL(ValueFromAmount(COIN*10).write(), std::string_view{"10.00000000"});
    CHECK_EQUAL(ValueFromAmount(COIN).write(), std::string_view{"1.00000000"});
    CHECK_EQUAL(ValueFromAmount(COIN/10).write(), std::string_view{"0.10000000"});
    CHECK_EQUAL(ValueFromAmount(COIN/100).write(), std::string_view{"0.01000000"});
    CHECK_EQUAL(ValueFromAmount(COIN/1000).write(), std::string_view{"0.00100000"});
    CHECK_EQUAL(ValueFromAmount(COIN/10000).write(), std::string_view{"0.00010000"});
    CHECK_EQUAL(ValueFromAmount(COIN/100000).write(), std::string_view{"0.00001000"});
    CHECK_EQUAL(ValueFromAmount(COIN/1000000).write(), std::string_view{"0.00000100"});
    CHECK_EQUAL(ValueFromAmount(COIN/10000000).write(), std::string_view{"0.00000010"});
    CHECK_EQUAL(ValueFromAmount(COIN/100000000).write(), std::string_view{"0.00000001"});

    CHECK_EQUAL(ValueFromAmount(std::numeric_limits<CAmount>::max()).write(), std::string_view{"92233720368.54775807"});
    CHECK_EQUAL(ValueFromAmount(std::numeric_limits<CAmount>::max() - 1).write(), std::string_view{"92233720368.54775806"});
    CHECK_EQUAL(ValueFromAmount(std::numeric_limits<CAmount>::max() - 2).write(), std::string_view{"92233720368.54775805"});
    CHECK_EQUAL(ValueFromAmount(std::numeric_limits<CAmount>::max() - 3).write(), std::string_view{"92233720368.54775804"});
    // ...
    CHECK_EQUAL(ValueFromAmount(std::numeric_limits<CAmount>::min() + 3).write(), std::string_view{"-92233720368.54775805"});
    CHECK_EQUAL(ValueFromAmount(std::numeric_limits<CAmount>::min() + 2).write(), std::string_view{"-92233720368.54775806"});
    CHECK_EQUAL(ValueFromAmount(std::numeric_limits<CAmount>::min() + 1).write(), std::string_view{"-92233720368.54775807"});
    CHECK_EQUAL(ValueFromAmount(std::numeric_limits<CAmount>::min()).write(), std::string_view{"-92233720368.54775808"});
}

static UniValue ValueFromString(const std::string& str) noexcept
{
    UniValue value;
    value.setNumStr(str);
    return value;
}

BOOST_AUTO_TEST_CASE(rpc_parse_monetary_values)
{
    CHECK_THROW(AmountFromValue(ValueFromString("-0.00000001")), UniValue);
    CHECK_EQUAL(AmountFromValue(ValueFromString("0")), 0LL);
    CHECK_EQUAL(AmountFromValue(ValueFromString("0.00000000")), 0LL);
    CHECK_EQUAL(AmountFromValue(ValueFromString("0.00000001")), 1LL);
    CHECK_EQUAL(AmountFromValue(ValueFromString("0.17622195")), 17622195LL);
    CHECK_EQUAL(AmountFromValue(ValueFromString("0.5")), 50000000LL);
    CHECK_EQUAL(AmountFromValue(ValueFromString("0.50000000")), 50000000LL);
    CHECK_EQUAL(AmountFromValue(ValueFromString("0.89898989")), 89898989LL);
    CHECK_EQUAL(AmountFromValue(ValueFromString("1.00000000")), 100000000LL);
    CHECK_EQUAL(AmountFromValue(ValueFromString("20999999.9999999")), 2099999999999990LL);
    CHECK_EQUAL(AmountFromValue(ValueFromString("20999999.99999999")), 2099999999999999LL);

    CHECK_EQUAL(AmountFromValue(ValueFromString("1e-8")), COIN/100000000);
    CHECK_EQUAL(AmountFromValue(ValueFromString("0.1e-7")), COIN/100000000);
    CHECK_EQUAL(AmountFromValue(ValueFromString("0.01e-6")), COIN/100000000);
    CHECK_EQUAL(AmountFromValue(ValueFromString("0.00000000000000000000000000000000000001e+30")), std::remove_cvref_t<decltype(AmountFromValue(ValueFromString("0.00000000000000000000000000000000000001e+30")))>{1});
    CHECK_EQUAL(AmountFromValue(ValueFromString("0.0000000000000000000000000000000000000000000000000000000000000000000000000001e+68")), COIN/100000000);
    CHECK_EQUAL(AmountFromValue(ValueFromString("10000000000000000000000000000000000000000000000000000000000000000e-64")), COIN);
    CHECK_EQUAL(AmountFromValue(ValueFromString("0.000000000000000000000000000000000000000000000000000000000000000100000000000000000000000000000000000000000000000000000e64")), COIN);

    CHECK_THROW(AmountFromValue(ValueFromString("1e-9")), UniValue); //should fail
    CHECK_THROW(AmountFromValue(ValueFromString("0.000000019")), UniValue); //should fail
    CHECK_EQUAL(AmountFromValue(ValueFromString("0.00000001000000")), 1LL); //should pass, cut trailing 0
    CHECK_THROW(AmountFromValue(ValueFromString("19e-9")), UniValue); //should fail
    CHECK_EQUAL(AmountFromValue(ValueFromString("0.19e-6")), std::remove_cvref_t<decltype(AmountFromValue(ValueFromString("0.19e-6")))>{19}); //should pass, leading 0 is present
    CHECK_EXCEPTION(AmountFromValue(".19e-6"), UniValue, HasJSON(R"({"code":-3,"message":"Invalid amount"})")); //should fail, no leading 0

    CHECK_THROW(AmountFromValue(ValueFromString("92233720368.54775808")), UniValue); //overflow error
    CHECK_THROW(AmountFromValue(ValueFromString("1e+11")), UniValue); //overflow error
    CHECK_THROW(AmountFromValue(ValueFromString("1e11")), UniValue); //overflow error signless
    CHECK_THROW(AmountFromValue(ValueFromString("93e+9")), UniValue); //overflow error
}

BOOST_AUTO_TEST_CASE(rpc_ban)
{
    CHECK_NO_THROW(CallRPC(std::string("clearbanned")));

    UniValue r;
    CHECK_NO_THROW(r = CallRPC(std::string("setban 127.0.0.0 add")));
    CHECK_THROW(r = CallRPC(std::string("setban 127.0.0.0:8334")), std::runtime_error); //portnumber for setban not allowed
    CHECK_NO_THROW(r = CallRPC(std::string("listbanned")));
    UniValue ar = r.get_array();
    UniValue o1 = ar[0].get_obj();
    UniValue adr = o1.find_value("address");
    CHECK_EQUAL(adr.get_str(), std::string_view{"127.0.0.0/32"});
    CHECK_NO_THROW(CallRPC(std::string("setban 127.0.0.0 remove")));
    CHECK_NO_THROW(r = CallRPC(std::string("listbanned")));
    ar = r.get_array();
    CHECK_EQUAL(ar.size(), 0U);

    CHECK_NO_THROW(r = CallRPC(std::string("setban 127.0.0.0/24 add 9907731200 true")));
    CHECK_NO_THROW(r = CallRPC(std::string("listbanned")));
    ar = r.get_array();
    o1 = ar[0].get_obj();
    adr = o1.find_value("address");
    int64_t banned_until{o1.find_value("banned_until").getInt<int64_t>()};
    CHECK_EQUAL(adr.get_str(), std::string_view{"127.0.0.0/24"});
    CHECK_EQUAL(banned_until, std::remove_cvref_t<decltype(banned_until)>{9907731200}); // absolute time check

    CHECK_NO_THROW(CallRPC(std::string("clearbanned")));

    auto now = 10'000s;
    SetMockTime(now);
    CHECK_NO_THROW(r = CallRPC(std::string("setban 127.0.0.0/24 add 200")));
    SetMockTime(now += 2s);
    const int64_t time_remaining_expected{198};
    CHECK_NO_THROW(r = CallRPC(std::string("listbanned")));
    ar = r.get_array();
    o1 = ar[0].get_obj();
    adr = o1.find_value("address");
    banned_until = o1.find_value("banned_until").getInt<int64_t>();
    const int64_t ban_created{o1.find_value("ban_created").getInt<int64_t>()};
    const int64_t ban_duration{o1.find_value("ban_duration").getInt<int64_t>()};
    const int64_t time_remaining{o1.find_value("time_remaining").getInt<int64_t>()};
    CHECK_EQUAL(adr.get_str(), std::string_view{"127.0.0.0/24"});
    CHECK_EQUAL(banned_until, time_remaining_expected + now.count());
    CHECK_EQUAL(ban_duration, banned_until - ban_created);
    CHECK_EQUAL(time_remaining, time_remaining_expected);

    // must throw an exception because 127.0.0.1 is in already banned subnet range
    CHECK_THROW(r = CallRPC(std::string("setban 127.0.0.1 add")), std::runtime_error);

    CHECK_NO_THROW(CallRPC(std::string("setban 127.0.0.0/24 remove")));
    CHECK_NO_THROW(r = CallRPC(std::string("listbanned")));
    ar = r.get_array();
    CHECK_EQUAL(ar.size(), 0U);

    CHECK_NO_THROW(r = CallRPC(std::string("setban 127.0.0.0/255.255.0.0 add")));
    CHECK_THROW(r = CallRPC(std::string("setban 127.0.1.1 add")), std::runtime_error);

    CHECK_NO_THROW(CallRPC(std::string("clearbanned")));
    CHECK_NO_THROW(r = CallRPC(std::string("listbanned")));
    ar = r.get_array();
    CHECK_EQUAL(ar.size(), 0U);


    CHECK_THROW(r = CallRPC(std::string("setban test add")), std::runtime_error); //invalid IP

    //IPv6 tests
    CHECK_NO_THROW(r = CallRPC(std::string("setban FE80:0000:0000:0000:0202:B3FF:FE1E:8329 add")));
    CHECK_NO_THROW(r = CallRPC(std::string("listbanned")));
    ar = r.get_array();
    o1 = ar[0].get_obj();
    adr = o1.find_value("address");
    CHECK_EQUAL(adr.get_str(), std::string_view{"fe80::202:b3ff:fe1e:8329/128"});

    CHECK_NO_THROW(CallRPC(std::string("clearbanned")));
    CHECK_NO_THROW(r = CallRPC(std::string("setban 2001:db8::/ffff:fffc:0:0:0:0:0:0 add")));
    CHECK_NO_THROW(r = CallRPC(std::string("listbanned")));
    ar = r.get_array();
    o1 = ar[0].get_obj();
    adr = o1.find_value("address");
    CHECK_EQUAL(adr.get_str(), std::string_view{"2001:db8::/30"});

    CHECK_NO_THROW(CallRPC(std::string("clearbanned")));
    CHECK_NO_THROW(r = CallRPC(std::string("setban 2001:4d48:ac57:400:cacf:e9ff:fe1d:9c63/128 add")));
    CHECK_NO_THROW(r = CallRPC(std::string("listbanned")));
    ar = r.get_array();
    o1 = ar[0].get_obj();
    adr = o1.find_value("address");
    CHECK_EQUAL(adr.get_str(), std::string_view{"2001:4d48:ac57:400:cacf:e9ff:fe1d:9c63/128"});
}

BOOST_AUTO_TEST_CASE(rpc_convert_values_generatetoaddress)
{
    UniValue result;

    CHECK_NO_THROW(result = RPCConvertValues("generatetoaddress", {"101", "mkESjLZW66TmHhiFX8MCaBjrhZ543PPh9a"}));
    CHECK_EQUAL(result[0].getInt<int>(), std::remove_cvref_t<decltype(result[0].getInt<int>())>{101});
    CHECK_EQUAL(result[1].get_str(), std::string_view{"mkESjLZW66TmHhiFX8MCaBjrhZ543PPh9a"});

    CHECK_NO_THROW(result = RPCConvertValues("generatetoaddress", {"101", "mhMbmE2tE9xzJYCV9aNC8jKWN31vtGrguU"}));
    CHECK_EQUAL(result[0].getInt<int>(), std::remove_cvref_t<decltype(result[0].getInt<int>())>{101});
    CHECK_EQUAL(result[1].get_str(), std::string_view{"mhMbmE2tE9xzJYCV9aNC8jKWN31vtGrguU"});

    CHECK_NO_THROW(result = RPCConvertValues("generatetoaddress", {"1", "mkESjLZW66TmHhiFX8MCaBjrhZ543PPh9a", "9"}));
    CHECK_EQUAL(result[0].getInt<int>(), std::remove_cvref_t<decltype(result[0].getInt<int>())>{1});
    CHECK_EQUAL(result[1].get_str(), std::string_view{"mkESjLZW66TmHhiFX8MCaBjrhZ543PPh9a"});
    CHECK_EQUAL(result[2].getInt<int>(), std::remove_cvref_t<decltype(result[2].getInt<int>())>{9});

    CHECK_NO_THROW(result = RPCConvertValues("generatetoaddress", {"1", "mhMbmE2tE9xzJYCV9aNC8jKWN31vtGrguU", "9"}));
    CHECK_EQUAL(result[0].getInt<int>(), std::remove_cvref_t<decltype(result[0].getInt<int>())>{1});
    CHECK_EQUAL(result[1].get_str(), std::string_view{"mhMbmE2tE9xzJYCV9aNC8jKWN31vtGrguU"});
    CHECK_EQUAL(result[2].getInt<int>(), std::remove_cvref_t<decltype(result[2].getInt<int>())>{9});
}

BOOST_AUTO_TEST_CASE(rpc_getblockstats_calculate_percentiles_by_weight)
{
    int64_t total_weight = 200;
    std::vector<std::pair<CAmount, int64_t>> feerates;
    feerates.reserve(200);
    CAmount result[NUM_GETBLOCKSTATS_PERCENTILES] = { 0 };

    for (int64_t i = 0; i < 100; i++) {
        feerates.emplace_back(1 ,1);
    }

    for (int64_t i = 0; i < 100; i++) {
        feerates.emplace_back(2 ,1);
    }

    CalculatePercentilesByWeight(result, feerates, total_weight);
    CHECK_EQUAL(result[0], std::remove_cvref_t<decltype(result[0])>{1});
    CHECK_EQUAL(result[1], std::remove_cvref_t<decltype(result[1])>{1});
    CHECK_EQUAL(result[2], std::remove_cvref_t<decltype(result[2])>{1});
    CHECK_EQUAL(result[3], std::remove_cvref_t<decltype(result[3])>{2});
    CHECK_EQUAL(result[4], std::remove_cvref_t<decltype(result[4])>{2});

    // Test with more pairs, and two pairs overlapping 2 percentiles.
    total_weight = 100;
    CAmount result2[NUM_GETBLOCKSTATS_PERCENTILES] = { 0 };
    feerates.clear();

    feerates.emplace_back(1, 9);
    feerates.emplace_back(2 , 16); //10th + 25th percentile
    feerates.emplace_back(4 ,50); //50th + 75th percentile
    feerates.emplace_back(5 ,10);
    feerates.emplace_back(9 ,15);  // 90th percentile

    CalculatePercentilesByWeight(result2, feerates, total_weight);

    CHECK_EQUAL(result2[0], std::remove_cvref_t<decltype(result2[0])>{2});
    CHECK_EQUAL(result2[1], std::remove_cvref_t<decltype(result2[1])>{2});
    CHECK_EQUAL(result2[2], std::remove_cvref_t<decltype(result2[2])>{4});
    CHECK_EQUAL(result2[3], std::remove_cvref_t<decltype(result2[3])>{4});
    CHECK_EQUAL(result2[4], std::remove_cvref_t<decltype(result2[4])>{9});

    // Same test as above, but one of the percentile-overlapping pairs is split in 2.
    total_weight = 100;
    CAmount result3[NUM_GETBLOCKSTATS_PERCENTILES] = { 0 };
    feerates.clear();

    feerates.emplace_back(1, 9);
    feerates.emplace_back(2 , 11); // 10th percentile
    feerates.emplace_back(2 , 5); // 25th percentile
    feerates.emplace_back(4 ,50); //50th + 75th percentile
    feerates.emplace_back(5 ,10);
    feerates.emplace_back(9 ,15); // 90th percentile

    CalculatePercentilesByWeight(result3, feerates, total_weight);

    CHECK_EQUAL(result3[0], std::remove_cvref_t<decltype(result3[0])>{2});
    CHECK_EQUAL(result3[1], std::remove_cvref_t<decltype(result3[1])>{2});
    CHECK_EQUAL(result3[2], std::remove_cvref_t<decltype(result3[2])>{4});
    CHECK_EQUAL(result3[3], std::remove_cvref_t<decltype(result3[3])>{4});
    CHECK_EQUAL(result3[4], std::remove_cvref_t<decltype(result3[4])>{9});

    // Test with one transaction spanning all percentiles.
    total_weight = 104;
    CAmount result4[NUM_GETBLOCKSTATS_PERCENTILES] = { 0 };
    feerates.clear();

    feerates.emplace_back(1, 100);
    feerates.emplace_back(2, 1);
    feerates.emplace_back(3, 1);
    feerates.emplace_back(3, 1);
    feerates.emplace_back(999999, 1);

    CalculatePercentilesByWeight(result4, feerates, total_weight);

    for (int64_t i = 0; i < NUM_GETBLOCKSTATS_PERCENTILES; i++) {
        CHECK_EQUAL(result4[i], std::remove_cvref_t<decltype(result4[i])>{1});
    }
}

// Make sure errors are triggered appropriately if parameters have the same names.
BOOST_AUTO_TEST_CASE(check_dup_param_names)
{
    enum ParamType { POSITIONAL, NAMED, NAMED_ONLY };
    auto make_rpc = [](std::vector<std::tuple<std::string, ParamType>> param_names) {
        std::vector<RPCArg> params;
        std::vector<RPCArg> options;
        auto push_options = [&] { if (!options.empty()) params.emplace_back(strprintf("options%i", params.size()), RPCArg::Type::OBJ_NAMED_PARAMS, RPCArg::Optional::OMITTED, "", std::move(options)); };
        for (auto& [param_name, param_type] : param_names) {
            if (param_type == POSITIONAL) {
                push_options();
                params.emplace_back(std::move(param_name), RPCArg::Type::NUM, RPCArg::Optional::OMITTED, "description");
            } else {
                options.emplace_back(std::move(param_name), RPCArg::Type::NUM, RPCArg::Optional::OMITTED, "description", RPCArgOptions{.also_positional = param_type == NAMED});
            }
        }
        push_options();
        return RPCMethod{"method_name", "description", params, RPCResults{}, RPCExamples{""}};
    };

    // No errors if parameter names are unique.
    make_rpc({{"p1", POSITIONAL}, {"p2", POSITIONAL}});
    make_rpc({{"p1", POSITIONAL}, {"p2", NAMED}});
    make_rpc({{"p1", POSITIONAL}, {"p2", NAMED_ONLY}});
    make_rpc({{"p1", NAMED}, {"p2", POSITIONAL}});
    make_rpc({{"p1", NAMED}, {"p2", NAMED}});
    make_rpc({{"p1", NAMED}, {"p2", NAMED_ONLY}});
    make_rpc({{"p1", NAMED_ONLY}, {"p2", POSITIONAL}});
    make_rpc({{"p1", NAMED_ONLY}, {"p2", NAMED}});
    make_rpc({{"p1", NAMED_ONLY}, {"p2", NAMED_ONLY}});

    {
        test_only_CheckFailuresAreExceptionsNotAborts mock_checks{};
        // Error if parameter names are duplicates, unless one parameter is
        // positional and the other is named and .also_positional is true.
        CHECK_THROW(make_rpc({{"p1", POSITIONAL}, {"p1", POSITIONAL}}), NonFatalCheckError);
        make_rpc({{"p1", POSITIONAL}, {"p1", NAMED}});
        CHECK_THROW(make_rpc({{"p1", POSITIONAL}, {"p1", NAMED_ONLY}}), NonFatalCheckError);
        make_rpc({{"p1", NAMED}, {"p1", POSITIONAL}});
        CHECK_THROW(make_rpc({{"p1", NAMED}, {"p1", NAMED}}), NonFatalCheckError);
        CHECK_THROW(make_rpc({{"p1", NAMED}, {"p1", NAMED_ONLY}}), NonFatalCheckError);
        CHECK_THROW(make_rpc({{"p1", NAMED_ONLY}, {"p1", POSITIONAL}}), NonFatalCheckError);
        CHECK_THROW(make_rpc({{"p1", NAMED_ONLY}, {"p1", NAMED}}), NonFatalCheckError);
        CHECK_THROW(make_rpc({{"p1", NAMED_ONLY}, {"p1", NAMED_ONLY}}), NonFatalCheckError);

        // Make sure duplicate aliases are detected too.
        CHECK_THROW(make_rpc({{"p1", POSITIONAL}, {"p2|p1", NAMED_ONLY}}), NonFatalCheckError);
    }
}

BOOST_AUTO_TEST_CASE(help_example)
{
    // test different argument types
    const RPCArgList& args = {{"foo", "bar"}, {"b", true}, {"n", 1}};
    CHECK_EQUAL(HelpExampleCliNamed("test", args), std::string_view{"> bitcoin-cli -named test foo=bar b=true n=1\n"});
    CHECK_EQUAL(HelpExampleRpcNamed("test", args), std::string_view{"> curl --user myusername --data-binary '{\"jsonrpc\": \"2.0\", \"id\": \"curltest\", \"method\": \"test\", \"params\": {\"foo\":\"bar\",\"b\":true,\"n\":1}}' -H 'content-type: application/json' http://127.0.0.1:8332/\n"});

    // test shell escape
    CHECK_EQUAL(HelpExampleCliNamed("test", {{"foo", "b'ar"}}), std::string_view{"> bitcoin-cli -named test foo='b'''ar'\n"});
    CHECK_EQUAL(HelpExampleCliNamed("test", {{"foo", "b\"ar"}}), std::string_view{"> bitcoin-cli -named test foo='b\"ar'\n"});
    CHECK_EQUAL(HelpExampleCliNamed("test", {{"foo", "b ar"}}), std::string_view{"> bitcoin-cli -named test foo='b ar'\n"});

    // test object params
    UniValue obj_value(UniValue::VOBJ);
    obj_value.pushKV("foo", "bar");
    obj_value.pushKV("b", false);
    obj_value.pushKV("n", 1);
    CHECK_EQUAL(HelpExampleCliNamed("test", {{"name", obj_value}}), std::string_view{"> bitcoin-cli -named test name='{\"foo\":\"bar\",\"b\":false,\"n\":1}'\n"});
    CHECK_EQUAL(HelpExampleRpcNamed("test", {{"name", obj_value}}), std::string_view{"> curl --user myusername --data-binary '{\"jsonrpc\": \"2.0\", \"id\": \"curltest\", \"method\": \"test\", \"params\": {\"name\":{\"foo\":\"bar\",\"b\":false,\"n\":1}}}' -H 'content-type: application/json' http://127.0.0.1:8332/\n"});

    // test array params
    UniValue arr_value(UniValue::VARR);
    arr_value.push_back("bar");
    arr_value.push_back(false);
    arr_value.push_back(1);
    CHECK_EQUAL(HelpExampleCliNamed("test", {{"name", arr_value}}), std::string_view{"> bitcoin-cli -named test name='[\"bar\",false,1]'\n"});
    CHECK_EQUAL(HelpExampleRpcNamed("test", {{"name", arr_value}}), std::string_view{"> curl --user myusername --data-binary '{\"jsonrpc\": \"2.0\", \"id\": \"curltest\", \"method\": \"test\", \"params\": {\"name\":[\"bar\",false,1]}}' -H 'content-type: application/json' http://127.0.0.1:8332/\n"});

    // test types don't matter for shell
    CHECK_EQUAL(HelpExampleCliNamed("foo", {{"arg", true}}), HelpExampleCliNamed("foo", {{"arg", "true"}}));

    // test types matter for Rpc
    CHECK_NE(HelpExampleRpcNamed("foo", {{"arg", true}}), HelpExampleRpcNamed("foo", {{"arg", "true"}}));
}

static void CheckRpc(const std::vector<RPCArg>& params, const UniValue& args, RPCMethod::RPCMethodImpl test_impl)
{
    auto null_result{RPCResult{RPCResult::Type::NONE, "", "None"}};
    const RPCMethod rpc{"dummy", "dummy description", params, null_result, RPCExamples{""}, test_impl};
    JSONRPCRequest req;
    req.params = args;

    rpc.HandleRequest(req);
}

BOOST_AUTO_TEST_CASE(rpc_arg_helper)
{
    constexpr bool DEFAULT_BOOL = true;
    constexpr auto DEFAULT_STRING = "default";
    constexpr uint64_t DEFAULT_UINT64_T = 3;

    //! Parameters with which the RPCMethod is instantiated
    const std::vector<RPCArg> params{
        // Required arg
        {"req_int", RPCArg::Type::NUM, RPCArg::Optional::NO, ""},
        {"req_str", RPCArg::Type::STR, RPCArg::Optional::NO, ""},
        // Default arg
        {"def_uint64_t", RPCArg::Type::NUM, RPCArg::Default{DEFAULT_UINT64_T}, ""},
        {"def_string", RPCArg::Type::STR, RPCArg::Default{DEFAULT_STRING}, ""},
        {"def_bool", RPCArg::Type::BOOL, RPCArg::Default{DEFAULT_BOOL}, ""},
        // Optional arg without default
        {"opt_double", RPCArg::Type::NUM, RPCArg::Optional::OMITTED, ""},
        {"opt_string", RPCArg::Type::STR, RPCArg::Optional::OMITTED, ""}
    };

    //! Check that `self.Arg` returns the same value as the `request.params` accessors
    RPCMethod::RPCMethodImpl check_positional = [](const RPCMethod& self, const JSONRPCRequest& request) -> UniValue {
            CHECK_EQUAL(self.Arg<int>("req_int"), request.params[0].getInt<int>());
            CHECK_EQUAL(self.Arg<std::string_view>("req_str"), request.params[1].get_str());
            CHECK_EQUAL(self.Arg<uint64_t>("def_uint64_t"), request.params[2].isNull() ? DEFAULT_UINT64_T : request.params[2].getInt<uint64_t>());
            CHECK_EQUAL(self.Arg<std::string_view>("def_string"), request.params[3].isNull() ? DEFAULT_STRING : request.params[3].get_str());
            CHECK_EQUAL(self.Arg<bool>("def_bool"), request.params[4].isNull() ? DEFAULT_BOOL : request.params[4].get_bool());
            if (!request.params[5].isNull()) {
                CHECK_EQUAL(self.MaybeArg<double>("opt_double").value(), request.params[5].get_real());
            } else {
                CHECK(!self.MaybeArg<double>("opt_double"));
            }
            if (!request.params[6].isNull()) {
                CHECK_EQUAL(self.MaybeArg<std::string_view>("opt_string"), request.params[6].get_str());
            } else {
                CHECK(!self.MaybeArg<std::string_view>("opt_string"));
            }
            return UniValue{};
        };
    CheckRpc(params, UniValue{JSON(R"([5, "hello", null, null, null, null, null])")}, check_positional);
    CheckRpc(params, UniValue{JSON(R"([5, "hello", 4, "test", true, 1.23, "world"])")}, check_positional);
}

BOOST_AUTO_TEST_SUITE_END()
