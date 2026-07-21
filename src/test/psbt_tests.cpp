// Copyright (c) 2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://www.opensource.org/licenses/mit-license.php.

#include <psbt.h>

#include <boost/test/unit_test.hpp>
#include <test/util/setup_common.h>
#include <util/strencodings.h>

BOOST_FIXTURE_TEST_SUITE(psbt_tests, BasicTestingSetup)

static PSBTProprietary MakeProprietary(uint64_t subtype, uint8_t key_data, uint8_t value)
{
    return PSBTProprietary{
        .subtype = subtype,
        .identifier = {'p', 's', 'b', 't'},
        .key = {key_data},
        .value = {value},
    };
}

void CheckTimeLock(const std::string& base64_psbt, std::optional<uint32_t> timelock)
{
    util::Result<PartiallySignedTransaction> psbt = DecodeBase64PSBT(base64_psbt);
    BOOST_CHECK(psbt);

    std::optional<uint32_t> computed_timelock = psbt->ComputeTimeLock();
    std::optional<CMutableTransaction> tx = psbt->GetUnsignedTx();
    if (timelock) {
        BOOST_CHECK(computed_timelock);
        BOOST_CHECK_EQUAL(*computed_timelock, *timelock);
        BOOST_CHECK(tx);
        BOOST_CHECK_EQUAL(tx->nLockTime, *timelock);
    } else {
        BOOST_CHECK(!computed_timelock);
        BOOST_CHECK(!tx);
    }
}

BOOST_AUTO_TEST_CASE(psbt2_timelock_test)
{
    CheckTimeLock("cHNidP8BAgQCAAAAAQQBAQEFAQIB+wQCAAAAAAEOIAsK2SFBnByHGXNdctxzn56p4GONH+TB7vD5lECEgV/IAQ8EAAAAAAABAwgACK8vAAAAAAEEFgAUxDD2TEdW2jENvRoIVXLvKZkmJywAAQMIi73rCwAAAAABBBYAFE3Rk6yWSlasG54cyoRU/i9HT4UTAA==", 0);
    CheckTimeLock("cHNidP8BAgQCAAAAAQMEAAAAAAEEAQIBBQEBAfsEAgAAAAABDiAPdY2/vU2nwWyKMwnDyB4RAPVh6mRttbAXUsSF4b3enwEPBAEAAAAAAQ4gOhs7PIN9ZInqejHY5sfdUDwAG+8+BpWOdXSAjWjKeKUBDwQAAAAAAAEDCE+TNXcAAAAAAQQWABQLE1LKzQPPaqG388jWOIZxs0peEQA=", 0);
    CheckTimeLock("cHNidP8BAgQCAAAAAQMEAAAAAAEEAQIBBQEBAfsEAgAAAAABDiAPdY2/vU2nwWyKMwnDyB4RAPVh6mRttbAXUsSF4b3enwEPBAEAAAABEgQQJwAAAAEOIDobOzyDfWSJ6nox2ObH3VA8ABvvPgaVjnV0gI1oynilAQ8EAAAAAAABAwhPkzV3AAAAAAEEFgAUCxNSys0Dz2qht/PI1jiGcbNKXhEA", 10000);
    CheckTimeLock("cHNidP8BAgQCAAAAAQMEAAAAAAEEAQIBBQEBAfsEAgAAAAABDiAPdY2/vU2nwWyKMwnDyB4RAPVh6mRttbAXUsSF4b3enwEPBAEAAAABEgQQJwAAAAEOIDobOzyDfWSJ6nox2ObH3VA8ABvvPgaVjnV0gI1oynilAQ8EAAAAAAESBCgjAAAAAQMIT5M1dwAAAAABBBYAFAsTUsrNA89qobfzyNY4hnGzSl4RAA==", 10000);
    CheckTimeLock("cHNidP8BAgQCAAAAAQMEAAAAAAEEAQIBBQEBAfsEAgAAAAABDiAPdY2/vU2nwWyKMwnDyB4RAPVh6mRttbAXUsSF4b3enwEPBAEAAAABEgQQJwAAAAEOIDobOzyDfWSJ6nox2ObH3VA8ABvvPgaVjnV0gI1oynilAQ8EAAAAAAERBIyNxGIBEgQoIwAAAAEDCE+TNXcAAAAAAQQWABQLE1LKzQPPaqG388jWOIZxs0peEQA=", 10000);
    CheckTimeLock("cHNidP8BAgQCAAAAAQMEAAAAAAEEAQIBBQEBAfsEAgAAAAABDiAPdY2/vU2nwWyKMwnDyB4RAPVh6mRttbAXUsSF4b3enwEPBAEAAAABEQSLjcRiARIEECcAAAABDiA6Gzs8g31kiep6Mdjmx91QPAAb7z4GlY51dICNaMp4pQEPBAAAAAABEQSMjcRiARIEKCMAAAABAwhPkzV3AAAAAAEEFgAUCxNSys0Dz2qht/PI1jiGcbNKXhEA", 10000);
    CheckTimeLock("cHNidP8BAgQCAAAAAQMEAAAAAAEEAQIBBQEBAfsEAgAAAAABDiAPdY2/vU2nwWyKMwnDyB4RAPVh6mRttbAXUsSF4b3enwEPBAEAAAABEQSLjcRiAAEOIDobOzyDfWSJ6nox2ObH3VA8ABvvPgaVjnV0gI1oynilAQ8EAAAAAAERBIyNxGIBEgQoIwAAAAEDCE+TNXcAAAAAAQQWABQLE1LKzQPPaqG388jWOIZxs0peEQA=", 1657048460);
    CheckTimeLock("cHNidP8BAgQCAAAAAQMEAAAAAAEEAQIBBQEBAfsEAgAAAAABDiAPdY2/vU2nwWyKMwnDyB4RAPVh6mRttbAXUsSF4b3enwEPBAEAAAABEQSLjcRiARIEECcAAAABDiA6Gzs8g31kiep6Mdjmx91QPAAb7z4GlY51dICNaMp4pQEPBAAAAAABEQSMjcRiAAEDCE+TNXcAAAAAAQQWABQLE1LKzQPPaqG388jWOIZxs0peEQA=", 1657048460);
    CheckTimeLock("cHNidP8BAgQCAAAAAQMEAAAAAAEEAQIBBQEBAfsEAgAAAAABDiAPdY2/vU2nwWyKMwnDyB4RAPVh6mRttbAXUsSF4b3enwEPBAEAAAAAAQ4gOhs7PIN9ZInqejHY5sfdUDwAG+8+BpWOdXSAjWjKeKUBDwQAAAAAAREEjI3EYgABAwhPkzV3AAAAAAEEFgAUCxNSys0Dz2qht/PI1jiGcbNKXhEA", 1657048460);
    CheckTimeLock("cHNidP8BAgQCAAAAAQMEAAAAAAEEAQIBBQEBAfsEAgAAAAABDiAPdY2/vU2nwWyKMwnDyB4RAPVh6mRttbAXUsSF4b3enwEPBAEAAAABEgQQJwAAAAEOIDobOzyDfWSJ6nox2ObH3VA8ABvvPgaVjnV0gI1oynilAQ8EAAAAAAERBIyNxGIAAQMIT5M1dwAAAAABBBYAFAsTUsrNA89qobfzyNY4hnGzSl4RAA==", std::nullopt);
}

BOOST_AUTO_TEST_CASE(psbt2_addinput)
{
    FastRandomContext rng(/*fDeterministic=*/true);

    CMutableTransaction mtx;
    PartiallySignedTransaction psbt(mtx, /*version=*/2);
    psbt.m_tx_modifiable.emplace();
    psbt.m_tx_modifiable->set(0, true);
    BOOST_CHECK_EQUAL(psbt.inputs.size(), 0);

    // Same PSBT version is required
    uint256 txid;
    rng.fillrand(MakeWritableByteSpan(txid));
    PSBTInput psbtin_v0(/*psbt_version=*/0, Txid::FromUint256(txid), /*prev_out=*/0);
    BOOST_CHECK(!psbt.AddInput(psbtin_v0));
    BOOST_CHECK_EQUAL(psbt.inputs.size(), 0);
    rng.fillrand(MakeWritableByteSpan(txid));
    PSBTInput psbtin(/*psbt_version=*/2, Txid::FromUint256(txid), /*prev_out=*/0);
    BOOST_CHECK(psbt.AddInput(psbtin));
    BOOST_CHECK_EQUAL(psbt.inputs.size(), 1);

    // Duplicates are not allowed
    BOOST_CHECK(!psbt.AddInput(psbtin));
    BOOST_CHECK_EQUAL(psbt.inputs.size(), 1);

    // Input with a unique txid is allowed
    rng.fillrand(MakeWritableByteSpan(txid));
    PSBTInput psbtin2(/*psbt_version=*/2, Txid::FromUint256(txid), /*prev_out=*/0);
    BOOST_CHECK(psbt.AddInput(psbtin2));
    BOOST_CHECK_EQUAL(psbt.inputs.size(), 2);

    // Disabling inputs modifiable flag prevents adding new inputs
    psbt.m_tx_modifiable->set(0, false);
    rng.fillrand(MakeWritableByteSpan(txid));
    PSBTInput psbtin3(/*psbt_version=*/2, Txid::FromUint256(txid), /*prev_out=*/0);
    BOOST_CHECK(!psbt.AddInput(psbtin3));
    BOOST_CHECK_EQUAL(psbt.inputs.size(), 2);
    psbt.m_tx_modifiable->set(0, true);

    // Make sure that timelock compatibility checks are working
    // No previous required timelocks, new input with both height and time timelocks is allowed
    rng.fillrand(MakeWritableByteSpan(txid));
    PSBTInput psbtin4(/*psbt_version=*/2, Txid::FromUint256(txid), /*prev_out=*/0);
    psbtin4.time_locktime = LOCKTIME_THRESHOLD;
    psbtin4.height_locktime = 100;
    BOOST_CHECK(psbt.AddInput(psbtin4));
    BOOST_CHECK_EQUAL(psbt.inputs.size(), 3);

    // Input with only a time timelock is allowed
    rng.fillrand(MakeWritableByteSpan(txid));
    PSBTInput psbtin5(/*psbt_version=*/2, Txid::FromUint256(txid), /*prev_out=*/0);
    psbtin5.time_locktime = LOCKTIME_THRESHOLD + 1;
    BOOST_CHECK(psbt.AddInput(psbtin5));
    BOOST_CHECK_EQUAL(psbt.inputs.size(), 4);

    // Input with only a height timelock is not allowed because of previous
    rng.fillrand(MakeWritableByteSpan(txid));
    PSBTInput psbtin6(/*psbt_version=*/2, Txid::FromUint256(txid), /*prev_out=*/0);
    psbtin6.height_locktime = 100;
    BOOST_CHECK(!psbt.AddInput(psbtin6));
    BOOST_CHECK_EQUAL(psbt.inputs.size(), 4);

    // Adding an input that already has a signature is allowed
    rng.fillrand(MakeWritableByteSpan(txid));
    PSBTInput psbtin7(/*psbt_version=*/2, Txid::FromUint256(txid), /*prev_out=*/0);
    psbtin7.final_script_sig << OP_1;
    BOOST_CHECK(psbt.AddInput(psbtin7));
    BOOST_CHECK_EQUAL(psbt.inputs.size(), 5);

    // Same thing, but with other things that have signatures
    psbtin7.final_script_sig.clear();
    psbtin7.final_script_witness.stack.emplace_back();
    BOOST_CHECK(!psbt.AddInput(psbtin7));
    BOOST_CHECK_EQUAL(psbt.inputs.size(), 5);
    psbtin7.final_script_witness.SetNull();
    psbtin7.partial_sigs.emplace();
    BOOST_CHECK(!psbt.AddInput(psbtin7));
    BOOST_CHECK_EQUAL(psbt.inputs.size(), 5);
    psbtin7.partial_sigs.clear();
    psbtin7.m_tap_key_sig.push_back(0);
    BOOST_CHECK(!psbt.AddInput(psbtin7));
    BOOST_CHECK_EQUAL(psbt.inputs.size(), 5);
    psbtin7.m_tap_key_sig.clear();
    psbtin7.m_tap_script_sigs.emplace();
    BOOST_CHECK(!psbt.AddInput(psbtin7));
    BOOST_CHECK_EQUAL(psbt.inputs.size(), 5);
    psbtin7.m_tap_script_sigs.clear();
    psbtin7.m_musig2_partial_sigs.emplace();
    BOOST_CHECK(!psbt.AddInput(psbtin7));
    BOOST_CHECK_EQUAL(psbt.inputs.size(), 5);

    // Adding an input that changes the timelock is no longer allowed
    rng.fillrand(MakeWritableByteSpan(txid));
    PSBTInput psbtin8(/*psbt_version=*/2, Txid::FromUint256(txid), /*prev_out=*/0);
    psbtin8.time_locktime = LOCKTIME_THRESHOLD + 2;
    BOOST_CHECK(!psbt.AddInput(psbtin8));
    BOOST_CHECK_EQUAL(psbt.inputs.size(), 5);
}

BOOST_AUTO_TEST_CASE(psbt2_addoutput)
{
    CMutableTransaction mtx;
    PartiallySignedTransaction psbt(mtx, /*version=*/2);
    psbt.m_tx_modifiable.emplace();
    psbt.m_tx_modifiable->set(1, true);
    BOOST_CHECK_EQUAL(psbt.outputs.size(), 0);

    // Same PSBT version is required
    PSBTOutput psbtout_v0(/*psbt_version=*/0, /*amount=*/1, CScript());
    BOOST_CHECK(!psbt.AddOutput(psbtout_v0));
    BOOST_CHECK_EQUAL(psbt.outputs.size(), 0);
    PSBTOutput psbtout(/*psbt_version=*/2, /*amount=*/1, CScript());
    BOOST_CHECK(psbt.AddOutput(psbtout));
    BOOST_CHECK_EQUAL(psbt.outputs.size(), 1);

    // Disabling outputs modifiable flag prevents adding new outputs
    psbt.m_tx_modifiable->set(1, false);
    PSBTOutput psbtout2(/*psbt_version=*/2, /*amount=*/1, CScript());
    BOOST_CHECK(!psbt.AddOutput(psbtout2));
    BOOST_CHECK_EQUAL(psbt.outputs.size(), 1);
    psbt.m_tx_modifiable->set(1, true);
    PSBTOutput psbtout3(/*psbt_version=*/2, /*amount=*/1, CScript());
    BOOST_CHECK(psbt.AddOutput(psbtout3));
    BOOST_CHECK_EQUAL(psbt.outputs.size(), 2);
}

BOOST_AUTO_TEST_CASE(merge_proprietary_fields)
{
    CMutableTransaction tx;
    tx.vin.emplace_back(COutPoint{});
    tx.vout.emplace_back(0, CScript{});

    PartiallySignedTransaction left(tx);
    PartiallySignedTransaction right(tx);

    const auto left_prop = MakeProprietary(/*subtype=*/1, /*key_data=*/0x01, /*value=*/0xaa);
    const auto right_prop = MakeProprietary(/*subtype=*/2, /*key_data=*/0x02, /*value=*/0xbb);

    left.m_proprietary.insert(left_prop);
    left.inputs[0].m_proprietary.insert(left_prop);
    left.outputs[0].m_proprietary.insert(left_prop);

    right.m_proprietary.insert(right_prop);
    right.inputs[0].m_proprietary.insert(right_prop);
    right.outputs[0].m_proprietary.insert(right_prop);

    BOOST_REQUIRE(left.Merge(right));

    BOOST_REQUIRE_EQUAL(left.m_proprietary.size(), 2U);
    BOOST_REQUIRE_EQUAL(left.inputs[0].m_proprietary.size(), 2U);
    BOOST_REQUIRE_EQUAL(left.outputs[0].m_proprietary.size(), 2U);

    const auto global_it = left.m_proprietary.find(right_prop);
    BOOST_REQUIRE(global_it != left.m_proprietary.end());
    BOOST_CHECK(global_it->value == right_prop.value);

    const auto input_it = left.inputs[0].m_proprietary.find(right_prop);
    BOOST_REQUIRE(input_it != left.inputs[0].m_proprietary.end());
    BOOST_CHECK(input_it->value == right_prop.value);

    const auto output_it = left.outputs[0].m_proprietary.find(right_prop);
    BOOST_REQUIRE(output_it != left.outputs[0].m_proprietary.end());
    BOOST_CHECK(output_it->value == right_prop.value);
}

BOOST_AUTO_TEST_CASE(finalize_rejects_invalid_final_script)
{
    const auto psbt_res{DecodeRawPSBT(MakeByteSpan(ParseHex("70736274ff01009a020000000258e87a21b56daf0c23be8e7070456c336f7cbaa5c8757924f545887bb2abdd750000000000ffffffff7a8d0427d0ec650a68aa46bb0b0920ea4422c071b2ca78302a077a59d07cea1d0100000000ffffffff0270aa00f00800000016001cd85c6b71d0060b09c9886a00000000000000034df500e1000005000016001e00aea9a2e53af8767788df5546e8742d1d87008f00000000000100bb0200000001aad73931018bd25f84ae400b68848be09db706eac2ac18298babee71ab656f8b0000000048473044022058f6fc7c6a33e1b31548d481c826c015bd30135aad42cd67790dab66d2ad243b02204a1ced2604c6735b6393e5b41691dd78b00f0c5942fb9f751856faa938157dba01feffffff0280f0fa020000000017a9140fb9463421696b82c833af241c78c17ddbde493487d0f20a270100000017a91429ca74f8a08f81999428185c97b5d852e4063f61876500000001071101210002000000000000000000000215d632fa6108559d6c5cd39a4c2183f1ab96e07f2102da6108a5756c5cd39b4c2183f1ab96e07f50300c6a4f000000816300000a00000001012067c2eb0b00000100171614b7f5faf40e3d40a5a459b1dbb7c756de178701042200208c2353173743b595dfb4a07b72ba8e42e3797da74e6151926860221f4a7352ae00002202027f6399757d2eff55a136ad02c684b1838b6556e5f1b6b34282a94b6b5005109610d90c6a4f00000080000000800500008000")))};
    BOOST_REQUIRE(psbt_res);

    PartiallySignedTransaction psbt{*psbt_res};
    CMutableTransaction result;
    BOOST_CHECK(!FinalizeAndExtractPSBT(psbt, result));
}

BOOST_AUTO_TEST_SUITE_END()
