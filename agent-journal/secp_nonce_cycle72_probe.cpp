#include <secp256k1.h>
#include <secp256k1_extrakeys.h>
#include <secp256k1_musig.h>
#include <secp256k1_schnorrsig.h>

#include <array>
#include <cstdio>
#include <cstring>

namespace {

struct CallbackState {
    unsigned illegal_calls{0};
    const char* last_message{nullptr};
};

void IllegalCallback(const char* message, void* data)
{
    auto* state = static_cast<CallbackState*>(data);
    ++state->illegal_calls;
    state->last_message = message;
}

template <typename T>
bool IsByte(const T& object, unsigned char value)
{
    const auto* bytes = reinterpret_cast<const unsigned char*>(&object);
    for (size_t i = 0; i < sizeof(object); ++i) {
        if (bytes[i] != value) return false;
    }
    return true;
}

template <typename T>
bool IsZero(const T& object)
{
    return IsByte(object, 0);
}

template <typename T>
bool Equal(const T& left, const T& right)
{
    return std::memcmp(&left, &right, sizeof(left)) == 0;
}

void Fill(unsigned char* out, size_t size, unsigned char seed)
{
    for (size_t i = 0; i < size; ++i) out[i] = static_cast<unsigned char>(seed + 17 * i);
}

bool Check(bool condition, const char* name)
{
    std::printf("%s %s\n", condition ? "PASS" : "FAIL", name);
    return condition;
}

int FailingNonce(unsigned char* nonce32, const unsigned char*, size_t,
                 const unsigned char*, const unsigned char*, const unsigned char*,
                 size_t, void*)
{
    std::memset(nonce32, 0xa5, 32);
    return 0;
}

int ZeroNonce(unsigned char* nonce32, const unsigned char*, size_t,
              const unsigned char*, const unsigned char*, const unsigned char*,
              size_t, void*)
{
    std::memset(nonce32, 0, 32);
    return 1;
}

struct Flow {
    std::array<unsigned char, 2> unused{};
    std::array<std::array<unsigned char, 32>, 2> sk{};
    std::array<std::array<unsigned char, 32>, 2> session_rand{};
    std::array<unsigned char, 32> msg{};
    std::array<secp256k1_keypair, 2> keypair{};
    std::array<secp256k1_pubkey, 2> pk{};
    std::array<const secp256k1_pubkey*, 2> pk_ptr{};
    std::array<secp256k1_musig_secnonce, 2> secnonce{};
    std::array<secp256k1_musig_secnonce, 2> fresh_secnonce{};
    std::array<secp256k1_musig_pubnonce, 2> pubnonce{};
    std::array<const secp256k1_musig_pubnonce*, 2> pubnonce_ptr{};
    secp256k1_xonly_pubkey agg_pk{};
    secp256k1_musig_keyagg_cache cache{};
    secp256k1_musig_aggnonce aggnonce{};
    secp256k1_musig_session session{};
    std::array<secp256k1_musig_partial_sig, 2> partial_sig{};
    std::array<const secp256k1_musig_partial_sig*, 2> partial_sig_ptr{};
    std::array<unsigned char, 64> final_sig{};
};

bool Prepare(secp256k1_context* ctx, Flow& flow)
{
    Fill(flow.msg.data(), flow.msg.size(), 0x31);
    for (size_t i = 0; i < 2; ++i) {
        Fill(flow.sk[i].data(), flow.sk[i].size(), static_cast<unsigned char>(0x41 + i * 13));
        Fill(flow.session_rand[i].data(), flow.session_rand[i].size(), static_cast<unsigned char>(0x71 + i * 19));
        if (!secp256k1_keypair_create(ctx, &flow.keypair[i], flow.sk[i].data())) return false;
        if (!secp256k1_keypair_pub(ctx, &flow.pk[i], &flow.keypair[i])) return false;
        flow.pk_ptr[i] = &flow.pk[i];
        flow.pubnonce_ptr[i] = &flow.pubnonce[i];
        flow.partial_sig_ptr[i] = &flow.partial_sig[i];
        if (!secp256k1_musig_nonce_gen(ctx, &flow.secnonce[i], &flow.pubnonce[i],
                                       flow.session_rand[i].data(), flow.sk[i].data(),
                                       &flow.pk[i], flow.msg.data(), nullptr, nullptr)) {
            return false;
        }
        flow.fresh_secnonce[i] = flow.secnonce[i];
    }
    if (!secp256k1_musig_pubkey_agg(ctx, &flow.agg_pk, &flow.cache, flow.pk_ptr.data(), 2)) return false;
    if (!secp256k1_musig_nonce_agg(ctx, &flow.aggnonce, flow.pubnonce_ptr.data(), 2)) return false;
    if (!secp256k1_musig_nonce_process(ctx, &flow.session, &flow.aggnonce, flow.msg.data(), &flow.cache)) return false;
    for (size_t i = 0; i < 2; ++i) {
        if (!secp256k1_musig_partial_sign(ctx, &flow.partial_sig[i], &flow.secnonce[i],
                                          &flow.keypair[i], &flow.cache, &flow.session)) return false;
        if (!secp256k1_musig_partial_sig_verify(ctx, &flow.partial_sig[i], &flow.pubnonce[i],
                                                &flow.pk[i], &flow.cache, &flow.session)) return false;
    }
    if (!secp256k1_musig_partial_sig_agg(ctx, flow.final_sig.data(), &flow.session,
                                         flow.partial_sig_ptr.data(), 2)) return false;
    return secp256k1_schnorrsig_verify(ctx, flow.final_sig.data(), flow.msg.data(), 32, &flow.agg_pk) == 1;
}

bool TestFailureState(secp256k1_context* ctx, const Flow& flow)
{
    bool ok = true;
    secp256k1_musig_pubnonce stale_pubnonce = flow.pubnonce[0];
    secp256k1_musig_secnonce failed_secnonce;
    std::memset(&failed_secnonce, 0xa5, sizeof(failed_secnonce));
    std::array<unsigned char, 32> zero_rand{};
    const int zero_ret = secp256k1_musig_nonce_gen(ctx, &failed_secnonce, &stale_pubnonce,
                                                    zero_rand.data(), flow.sk[0].data(),
                                                    &flow.pk[0], flow.msg.data(), nullptr, nullptr);
    ok &= Check(zero_ret == 0, "nonce_gen rejects all-zero session randomness");
    ok &= Check(IsZero(failed_secnonce), "nonce_gen clears secret nonce on random failure");
    ok &= Check(Equal(stale_pubnonce, flow.pubnonce[0]), "nonce_gen leaves public output unchanged on early failure");

    secp256k1_musig_pubnonce invalid_key_pubnonce;
    secp256k1_musig_secnonce invalid_key_secnonce;
    std::memset(&invalid_key_pubnonce, 0x5a, sizeof(invalid_key_pubnonce));
    std::memset(&invalid_key_secnonce, 0x5a, sizeof(invalid_key_secnonce));
    std::array<unsigned char, 32> bad_key;
    std::array<unsigned char, 32> retry_rand;
    bad_key.fill(0xff);
    Fill(retry_rand.data(), retry_rand.size(), 0x19);
    const int bad_key_ret = secp256k1_musig_nonce_gen(ctx, &invalid_key_secnonce, &invalid_key_pubnonce,
                                                      retry_rand.data(), bad_key.data(), &flow.pk[0],
                                                      flow.msg.data(), nullptr, nullptr);
    ok &= Check(bad_key_ret == 0, "nonce_gen rejects an invalid secret key");
    ok &= Check(IsZero(invalid_key_secnonce), "nonce_gen invalidates secret nonce after key failure");
    ok &= Check(!IsByte(invalid_key_pubnonce, 0x5a), "nonce_gen does not preserve an invalid-key public nonce sentinel");

    secp256k1_musig_secnonce retry_secnonce = flow.fresh_secnonce[0];
    secp256k1_musig_partial_sig partial_sentinel;
    std::memset(&partial_sentinel, 0x5a, sizeof(partial_sentinel));
    using PartialSignFn = int (*)(const secp256k1_context*, secp256k1_musig_partial_sig*,
                                  secp256k1_musig_secnonce*, const secp256k1_keypair*,
                                  const secp256k1_musig_keyagg_cache*, const secp256k1_musig_session*);
    const PartialSignFn partial_sign = &secp256k1_musig_partial_sign;
    const int null_output_ret = partial_sign(ctx, nullptr, &retry_secnonce,
                                              &flow.keypair[0], &flow.cache, &flow.session);
    ok &= Check(null_output_ret == 0, "partial_sign rejects a null output");
    ok &= Check(IsZero(retry_secnonce), "partial_sign consumes nonce before output validation");
    (void)partial_sentinel;

    secp256k1_musig_secnonce wrong_key_secnonce = flow.fresh_secnonce[0];
    const int wrong_key_ret = secp256k1_musig_partial_sign(ctx, &partial_sentinel, &wrong_key_secnonce,
                                                            &flow.keypair[1], &flow.cache, &flow.session);
    ok &= Check(wrong_key_ret == 0, "partial_sign rejects a nonce bound to another key");
    ok &= Check(IsZero(wrong_key_secnonce), "partial_sign consumes nonce after key binding failure");
    ok &= Check(IsByte(partial_sentinel, 0x5a), "partial_sign preserves caller output on key binding failure");

    secp256k1_musig_secnonce bad_session_secnonce = flow.fresh_secnonce[0];
    secp256k1_musig_session invalid_session{};
    const int bad_session_ret = secp256k1_musig_partial_sign(ctx, &partial_sentinel, &bad_session_secnonce,
                                                              &flow.keypair[0], &flow.cache, &invalid_session);
    ok &= Check(bad_session_ret == 0, "partial_sign rejects an uninitialized session");
    ok &= Check(IsZero(bad_session_secnonce), "partial_sign consumes nonce after session failure");
    ok &= Check(IsByte(partial_sentinel, 0x5a), "partial_sign preserves caller output on session failure");

    secp256k1_musig_secnonce repeated_secnonce = flow.fresh_secnonce[0];
    secp256k1_musig_partial_sig repeated_sig;
    const int first_sign_ret = secp256k1_musig_partial_sign(ctx, &repeated_sig, &repeated_secnonce,
                                                             &flow.keypair[0], &flow.cache, &flow.session);
    const int second_sign_ret = secp256k1_musig_partial_sign(ctx, &repeated_sig, &repeated_secnonce,
                                                              &flow.keypair[0], &flow.cache, &flow.session);
    ok &= Check(first_sign_ret == 1, "partial_sign succeeds once with a valid nonce");
    ok &= Check(second_sign_ret == 0, "partial_sign rejects a consumed nonce");
    return ok;
}

bool TestSchnorrFailure(secp256k1_context* ctx, const Flow& flow)
{
    bool ok = true;
    secp256k1_schnorrsig_extraparams params = SECP256K1_SCHNORRSIG_EXTRAPARAMS_INIT;
    std::array<unsigned char, 64> sig;
    sig.fill(0x5a);
    params.noncefp = FailingNonce;
    const int failed_ret = secp256k1_schnorrsig_sign_custom(ctx, sig.data(), flow.msg.data(), 32,
                                                              &flow.keypair[0], &params);
    ok &= Check(failed_ret == 0, "schnorr custom nonce failure returns failure");
    ok &= Check(IsByte(sig, 0), "schnorr custom nonce failure clears signature output");

    sig.fill(0x5a);
    params.noncefp = ZeroNonce;
    const int zero_ret = secp256k1_schnorrsig_sign_custom(ctx, sig.data(), flow.msg.data(), 32,
                                                            &flow.keypair[0], &params);
    ok &= Check(zero_ret == 0, "schnorr rejects a zero callback nonce");
    ok &= Check(IsByte(sig, 0), "schnorr zero callback nonce clears signature output");
    return ok;
}

bool TestMalformedOutputBindings(secp256k1_context* ctx, const Flow& flow, CallbackState& callbacks)
{
    bool ok = true;
    secp256k1_musig_session session_sentinel;
    std::memset(&session_sentinel, 0x5a, sizeof(session_sentinel));
    secp256k1_musig_session session_before = session_sentinel;
    secp256k1_musig_keyagg_cache invalid_cache{};
    const unsigned before_process_callbacks = callbacks.illegal_calls;
    const int process_ret = secp256k1_musig_nonce_process(ctx, &session_sentinel, &flow.aggnonce,
                                                          flow.msg.data(), &invalid_cache);
    ok &= Check(process_ret == 0, "nonce_process rejects an invalid cache");
    ok &= Check(callbacks.illegal_calls == before_process_callbacks + 1, "nonce_process reports invalid cache");
    ok &= Check(Equal(session_sentinel, session_before), "nonce_process preserves session on cache failure");

    secp256k1_musig_aggnonce aggnonce_sentinel;
    std::memset(&aggnonce_sentinel, 0x5a, sizeof(aggnonce_sentinel));
    const secp256k1_musig_pubnonce* invalid_pubnonce_ptr[1] = {nullptr};
    const unsigned before_agg_callbacks = callbacks.illegal_calls;
    const int agg_ret = secp256k1_musig_nonce_agg(ctx, &aggnonce_sentinel, invalid_pubnonce_ptr, 1);
    ok &= Check(agg_ret == 0, "nonce_agg rejects a null nonce element");
    ok &= Check(callbacks.illegal_calls == before_agg_callbacks + 1, "nonce_agg reports a null nonce element");
    ok &= Check(IsByte(aggnonce_sentinel, 0x5a), "nonce_agg preserves output on pointer failure");

    std::array<unsigned char, 64> sig_sentinel;
    sig_sentinel.fill(0x5a);
    const secp256k1_musig_partial_sig* invalid_sig_ptr[1] = {nullptr};
    const unsigned before_sig_callbacks = callbacks.illegal_calls;
    const int sig_ret = secp256k1_musig_partial_sig_agg(ctx, sig_sentinel.data(), &flow.session,
                                                         invalid_sig_ptr, 1);
    ok &= Check(sig_ret == 0, "partial_sig_agg rejects a null signature element");
    ok &= Check(callbacks.illegal_calls == before_sig_callbacks + 1, "partial_sig_agg reports a null signature element");
    ok &= Check(sig_sentinel[0] == 0x5a && sig_sentinel[63] == 0x5a, "partial_sig_agg preserves output on pointer failure");
    return ok;
}

bool TestInfinityAndDuplicates(secp256k1_context* ctx, const Flow& flow)
{
    bool ok = true;
    std::array<unsigned char, 66> infinity_bytes{};
    secp256k1_musig_aggnonce infinity_nonce{};
    ok &= Check(secp256k1_musig_aggnonce_parse(ctx, &infinity_nonce, infinity_bytes.data()) == 1,
                "aggregate nonce parser accepts the encoded infinity point");
    secp256k1_musig_session infinity_session{};
    const int process_inf_ret = secp256k1_musig_nonce_process(ctx, &infinity_session, &infinity_nonce,
                                                              flow.msg.data(), &flow.cache);
    ok &= Check(process_inf_ret == 1, "nonce_process handles an infinity aggregate nonce");
    secp256k1_musig_secnonce inf_secnonce = flow.fresh_secnonce[0];
    secp256k1_musig_partial_sig inf_partial{};
    const int inf_sign_ret = secp256k1_musig_partial_sign(ctx, &inf_partial, &inf_secnonce,
                                                           &flow.keypair[0], &flow.cache, &infinity_session);
    ok &= Check(inf_sign_ret == 1, "partial_sign completes against the infinity-derived session");
    ok &= Check(secp256k1_musig_partial_sig_verify(ctx, &inf_partial, &flow.pubnonce[1], &flow.pk[0],
                                                   &flow.cache, &infinity_session) == 0,
                "infinity-derived session does not validate an unrelated signer nonce");

    const secp256k1_pubkey* duplicate_keys[2] = {&flow.pk[0], &flow.pk[0]};
    secp256k1_xonly_pubkey duplicate_agg_pk{};
    secp256k1_musig_keyagg_cache duplicate_cache{};
    ok &= Check(secp256k1_musig_pubkey_agg(ctx, &duplicate_agg_pk, &duplicate_cache, duplicate_keys, 2) == 1,
                "duplicate public keys are accepted as a multiset");

    std::array<unsigned char, 32> rand0;
    std::array<unsigned char, 32> rand1;
    Fill(rand0.data(), rand0.size(), 0x11);
    Fill(rand1.data(), rand1.size(), 0x22);
    secp256k1_musig_secnonce duplicate_secnonce[2];
    secp256k1_musig_pubnonce duplicate_pubnonce[2];
    const secp256k1_musig_pubnonce* duplicate_pubnonce_ptr[2] = {&duplicate_pubnonce[0], &duplicate_pubnonce[1]};
    ok &= Check(secp256k1_musig_nonce_gen(ctx, &duplicate_secnonce[0], &duplicate_pubnonce[0], rand0.data(),
                                          flow.sk[0].data(), &flow.pk[0], flow.msg.data(), nullptr, nullptr) == 1,
                "first duplicate-key nonce is generated");
    ok &= Check(secp256k1_musig_nonce_gen(ctx, &duplicate_secnonce[1], &duplicate_pubnonce[1], rand1.data(),
                                          flow.sk[0].data(), &flow.pk[0], flow.msg.data(), nullptr, nullptr) == 1,
                "second duplicate-key nonce is generated");
    secp256k1_musig_aggnonce duplicate_aggnonce{};
    secp256k1_musig_session duplicate_session{};
    ok &= Check(secp256k1_musig_nonce_agg(ctx, &duplicate_aggnonce, duplicate_pubnonce_ptr, 2) == 1,
                "duplicate-key nonces aggregate");
    ok &= Check(secp256k1_musig_nonce_process(ctx, &duplicate_session, &duplicate_aggnonce,
                                              flow.msg.data(), &duplicate_cache) == 1,
                "duplicate-key session processes");
    secp256k1_musig_partial_sig duplicate_partial[2];
    const secp256k1_musig_partial_sig* duplicate_partial_ptr[2] = {&duplicate_partial[0], &duplicate_partial[1]};
    ok &= Check(secp256k1_musig_partial_sign(ctx, &duplicate_partial[0], &duplicate_secnonce[0],
                                             &flow.keypair[0], &duplicate_cache, &duplicate_session) == 1,
                "first duplicate-key partial signature succeeds");
    ok &= Check(secp256k1_musig_partial_sign(ctx, &duplicate_partial[1], &duplicate_secnonce[1],
                                             &flow.keypair[0], &duplicate_cache, &duplicate_session) == 1,
                "second duplicate-key partial signature succeeds");
    std::array<unsigned char, 64> duplicate_sig{};
    ok &= Check(secp256k1_musig_partial_sig_agg(ctx, duplicate_sig.data(), &duplicate_session,
                                                duplicate_partial_ptr, 2) == 1,
                "duplicate-key partial signatures aggregate");
    ok &= Check(secp256k1_schnorrsig_verify(ctx, duplicate_sig.data(), flow.msg.data(), 32,
                                            &duplicate_agg_pk) == 1,
                "duplicate-key aggregate signature verifies");
    return ok;
}

} // namespace

int main()
{
    secp256k1_context* ctx = secp256k1_context_create(SECP256K1_CONTEXT_SIGN | SECP256K1_CONTEXT_VERIFY);
    if (ctx == nullptr) return 1;
    CallbackState callbacks;
    secp256k1_context_set_illegal_callback(ctx, IllegalCallback, &callbacks);

    Flow flow;
    bool ok = Check(Prepare(ctx, flow), "valid two-party MuSig flow signs and verifies");
    if (ok) ok &= TestFailureState(ctx, flow);
    if (ok) ok &= TestSchnorrFailure(ctx, flow);
    if (ok) ok &= TestMalformedOutputBindings(ctx, flow, callbacks);
    if (ok) ok &= TestInfinityAndDuplicates(ctx, flow);
    std::printf("illegal_callbacks=%u last=%s\n", callbacks.illegal_calls,
                callbacks.last_message == nullptr ? "none" : callbacks.last_message);
    secp256k1_context_destroy(ctx);
    return ok ? 0 : 1;
}
