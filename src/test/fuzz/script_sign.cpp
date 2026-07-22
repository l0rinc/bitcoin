// Copyright (c) 2020-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparams.h>
#include <key.h>
#include <musig.h>
#include <psbt.h>
#include <pubkey.h>
#include <script/interpreter.h>
#include <script/keyorigin.h>
#include <script/sign.h>
#include <script/signingprovider.h>
#include <streams.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/util/transaction_utils.h>
#include <util/chaintype.h>
#include <util/translation.h>

#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <map>
#include <optional>
#include <string>
#include <vector>

void initialize_script_sign()
{
    static ECC_Context ecc_context{};
    SelectParams(ChainType::REGTEST);
}

FUZZ_TARGET(script_sign, .init = initialize_script_sign)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    const std::vector<uint8_t> key = ConsumeRandomLengthByteVector(fuzzed_data_provider, 128);

    {
        DataStream random_data_stream{ConsumeDataStream(fuzzed_data_provider)};
        std::map<CPubKey, KeyOriginInfo> hd_keypaths;
        try {
            DeserializeHDKeypaths(random_data_stream, key, hd_keypaths);
        } catch (const std::ios_base::failure&) {
        }
        DataStream serialized{};
        SerializeHDKeypaths(serialized, hd_keypaths, CompactSizeWriter(fuzzed_data_provider.ConsumeIntegral<uint8_t>()));
    }

    {
        std::map<CPubKey, KeyOriginInfo> hd_keypaths;
        LIMITED_WHILE (fuzzed_data_provider.ConsumeBool(), 10000) {
            const std::optional<CPubKey> pub_key = ConsumeDeserializable<CPubKey>(fuzzed_data_provider);
            if (!pub_key) {
                break;
            }
            const std::optional<KeyOriginInfo> key_origin_info = ConsumeDeserializable<KeyOriginInfo>(fuzzed_data_provider);
            if (!key_origin_info) {
                break;
            }
            hd_keypaths[*pub_key] = *key_origin_info;
        }
        DataStream serialized{};
        try {
            SerializeHDKeypaths(serialized, hd_keypaths, CompactSizeWriter(fuzzed_data_provider.ConsumeIntegral<uint8_t>()));
        } catch (const std::ios_base::failure&) {
        }
        std::map<CPubKey, KeyOriginInfo> deserialized_hd_keypaths;
        try {
            DeserializeHDKeypaths(serialized, key, deserialized_hd_keypaths);
        } catch (const std::ios_base::failure&) {
        }
        assert(hd_keypaths.size() >= deserialized_hd_keypaths.size());
    }

    {
        SignatureData signature_data_1{ConsumeScript(fuzzed_data_provider)};
        SignatureData signature_data_2{ConsumeScript(fuzzed_data_provider)};
        signature_data_1.MergeSignatureData(signature_data_2);
    }

    FillableSigningProvider provider;
    CKey k = ConsumePrivateKey(fuzzed_data_provider);
    if (k.IsValid()) {
        provider.AddKey(k);
    }

    {
        const std::optional<CMutableTransaction> mutable_transaction = ConsumeDeserializable<CMutableTransaction>(fuzzed_data_provider, TX_WITH_WITNESS);
        const std::optional<CTxOut> tx_out = ConsumeDeserializable<CTxOut>(fuzzed_data_provider);
        const unsigned int n_in = fuzzed_data_provider.ConsumeIntegral<unsigned int>();
        if (mutable_transaction && tx_out && mutable_transaction->vin.size() > n_in) {
            SignatureData signature_data_1 = DataFromTransaction(*mutable_transaction, n_in, *tx_out);
            CTxIn input;
            UpdateInput(input, signature_data_1);
            const CScript script = ConsumeScript(fuzzed_data_provider);
            SignatureData signature_data_2{script};
            signature_data_1.MergeSignatureData(signature_data_2);
        }
        if (mutable_transaction) {
            CTransaction tx_from{*mutable_transaction};
            CMutableTransaction tx_to;
            const std::optional<CMutableTransaction> opt_tx_to = ConsumeDeserializable<CMutableTransaction>(fuzzed_data_provider, TX_WITH_WITNESS);
            if (opt_tx_to) {
                tx_to = *opt_tx_to;
            }
            CMutableTransaction script_tx_to = tx_to;
            CMutableTransaction sign_transaction_tx_to = tx_to;
            if (n_in < tx_to.vin.size() && tx_to.vin[n_in].prevout.n < tx_from.vout.size()) {
                SignatureData empty;
                (void)SignSignature(provider, tx_from, tx_to, n_in, fuzzed_data_provider.ConsumeIntegral<int>(), empty);
            }
            if (n_in < script_tx_to.vin.size()) {
                SignatureData empty;
                auto from_pub_key = ConsumeScript(fuzzed_data_provider);
                auto amount = ConsumeMoney(fuzzed_data_provider);
                auto n_hash_type = fuzzed_data_provider.ConsumeIntegral<int>();
                (void)SignSignature(provider, from_pub_key, script_tx_to, n_in, amount, n_hash_type, empty);
                MutableTransactionSignatureCreator signature_creator{tx_to, n_in, ConsumeMoney(fuzzed_data_provider), {.sighash_type = fuzzed_data_provider.ConsumeIntegral<int>()}};
                std::vector<unsigned char> vch_sig;
                CKeyID address;
                if (fuzzed_data_provider.ConsumeBool()) {
                    if (k.IsValid()) {
                        address = k.GetPubKey().GetID();
                    }
                } else {
                    address = CKeyID{ConsumeUInt160(fuzzed_data_provider)};
                }
                auto script_code = ConsumeScript(fuzzed_data_provider);
                auto sigversion = fuzzed_data_provider.PickValueInArray({SigVersion::BASE, SigVersion::WITNESS_V0});
                (void)signature_creator.CreateSig(provider, vch_sig, address, script_code, sigversion);
            }
            std::map<COutPoint, Coin> coins{ConsumeCoins(fuzzed_data_provider)};
            std::map<int, bilingual_str> input_errors;
            (void)SignTransaction(sign_transaction_tx_to, &provider, coins, {.sighash_type = fuzzed_data_provider.ConsumeIntegral<int>()}, input_errors);
        }
    }

    {
        SignatureData signature_data_1;
        (void)ProduceSignature(provider, DUMMY_SIGNATURE_CREATOR, ConsumeScript(fuzzed_data_provider), signature_data_1);
        SignatureData signature_data_2;
        (void)ProduceSignature(provider, DUMMY_MAXIMUM_SIGNATURE_CREATOR, ConsumeScript(fuzzed_data_provider), signature_data_2);
    }

    // Exercise the cryptographic-secret lifecycle that random SignatureData
    // rarely reaches: create, consume, and remove both MuSig2 secret nonces.
    {
        std::array<unsigned char, 32> fallback_secret{};
        fallback_secret.back() = 1;
        CKey participant_one = k;
        if (!participant_one.IsValid()) {
            participant_one.Set(fallback_secret.begin(), fallback_secret.end(), /*fCompressedIn=*/true);
        }
        fallback_secret.back() = 2;
        CKey participant_two;
        participant_two.Set(fallback_secret.begin(), fallback_secret.end(), /*fCompressedIn=*/true);
        if (participant_one.GetPubKey() == participant_two.GetPubKey()) {
            fallback_secret.back() = 3;
            participant_two.Set(fallback_secret.begin(), fallback_secret.end(), /*fCompressedIn=*/true);
        }

        const std::vector<CPubKey> participants{participant_one.GetPubKey(), participant_two.GetPubKey()};
        const std::optional<CPubKey> aggregate_pubkey{MuSig2AggregatePubkeys(participants)};
        assert(aggregate_pubkey.has_value());

        CMutableTransaction musig_transaction;
        musig_transaction.vin.emplace_back(COutPoint{Txid::FromUint256(uint256::ONE), 0});
        musig_transaction.vout.emplace_back(1, CScript{});
        const CTransaction musig_tx_const{musig_transaction};
        std::vector<CTxOut> spent_outputs;
        spent_outputs.emplace_back(1, CScript{});
        PrecomputedTransactionData musig_txdata;
        musig_txdata.Init(musig_tx_const, std::move(spent_outputs), true);
        MutableTransactionSignatureCreator musig_creator{musig_transaction, 0, 1, &musig_txdata, {.sighash_type = SIGHASH_DEFAULT}};

        FlatSigningProvider musig_provider;
        musig_provider.keys.emplace(participants[0].GetID(), participant_one);
        musig_provider.keys.emplace(participants[1].GetID(), participant_two);
        musig_provider.aggregate_pubkeys.emplace(*aggregate_pubkey, participants);
        std::map<uint256, MuSig2SecNonce> secnonces;
        musig_provider.musig2_secnonces = &secnonces;

        SignatureData musig_data;
        musig_data.musig2_pubkeys.emplace(*aggregate_pubkey, participants);
        const auto pubnonce_one{musig_creator.CreateMuSig2Nonce(musig_provider, *aggregate_pubkey, *aggregate_pubkey, participants[0], nullptr, nullptr, SigVersion::TAPROOT, musig_data)};
        const auto pubnonce_two{musig_creator.CreateMuSig2Nonce(musig_provider, *aggregate_pubkey, *aggregate_pubkey, participants[1], nullptr, nullptr, SigVersion::TAPROOT, musig_data)};
        assert(pubnonce_one.size() == MUSIG2_PUBNONCE_SIZE);
        assert(pubnonce_two.size() == MUSIG2_PUBNONCE_SIZE);
        assert(secnonces.size() == participants.size());

        const auto leaf_aggregate_key{std::make_pair(*aggregate_pubkey, uint256{})};
        auto& pubnonces{musig_data.musig2_pubnonces[leaf_aggregate_key]};
        pubnonces.emplace(participants[0], pubnonce_one);
        pubnonces.emplace(participants[1], pubnonce_two);

        const std::vector<std::pair<uint256, bool>> no_tweaks;
        uint256 partial_sig_one;
        assert(musig_creator.CreateMuSig2PartialSig(musig_provider, partial_sig_one, *aggregate_pubkey, *aggregate_pubkey, participants[0], nullptr, no_tweaks, SigVersion::TAPROOT, musig_data));
        assert(!partial_sig_one.IsNull());
        assert(secnonces.size() == 1);
        uint256 partial_sig_two;
        assert(musig_creator.CreateMuSig2PartialSig(musig_provider, partial_sig_two, *aggregate_pubkey, *aggregate_pubkey, participants[1], nullptr, no_tweaks, SigVersion::TAPROOT, musig_data));
        assert(!partial_sig_two.IsNull());
        assert(secnonces.empty());

        auto& partial_sigs{musig_data.musig2_partial_sigs[leaf_aggregate_key]};
        partial_sigs.emplace(participants[0], partial_sig_one);
        partial_sigs.emplace(participants[1], partial_sig_two);
        std::vector<uint8_t> aggregate_sig;
        assert(musig_creator.CreateMuSig2AggregateSig(participants, aggregate_sig, *aggregate_pubkey, *aggregate_pubkey, nullptr, no_tweaks, SigVersion::TAPROOT, musig_data));
        assert(aggregate_sig.size() == 64);

        ScriptExecutionData execdata;
        execdata.m_annex_init = true;
        execdata.m_annex_present = false;
        uint256 sighash;
        assert(SignatureHashSchnorr(sighash, execdata, musig_tx_const, 0, SIGHASH_DEFAULT, SigVersion::TAPROOT, musig_txdata, MissingDataBehavior::ASSERT_FAIL));
        assert(XOnlyPubKey(*aggregate_pubkey).VerifySchnorr(sighash, aggregate_sig));

        uint256 retry_partial_sig{uint256::ONE};
        assert(!musig_creator.CreateMuSig2PartialSig(musig_provider, retry_partial_sig, *aggregate_pubkey, *aggregate_pubkey, participants[0], nullptr, no_tweaks, SigVersion::TAPROOT, musig_data));
        assert(retry_partial_sig == uint256::ONE);
        assert(secnonces.empty());
    }
}
