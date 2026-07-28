#include <secp256k1.h>
#include <secp256k1_ellswift.h>
#include <secp256k1_extrakeys.h>
#include <secp256k1_silentpayments.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <numeric>
#include <vector>

#include <sched.h>
#include <time.h>

namespace {

struct Inputs {
    std::array<unsigned char, 32> low{};
    std::array<unsigned char, 32> high{};
    std::array<unsigned char, 32> peer_secret{};
    std::array<unsigned char, 32> spend_secret{};
    std::array<unsigned char, 32> aux{};
    std::array<unsigned char, 36> outpoint{};
    secp256k1_pubkey peer{};
    secp256k1_pubkey scan_pubkey{};
    secp256k1_pubkey spend_pubkey{};
    std::array<unsigned char, 64> ell_a{};
    std::array<unsigned char, 64> ell_b{};
    secp256k1_silentpayments_prevouts_summary prevouts_summary{};
    secp256k1_silentpayments_recipient recipient{};
    secp256k1_xonly_pubkey tx_output{};
};

using Operation = bool (*)(secp256k1_context*, const Inputs&, bool, uint64_t, volatile unsigned char*);

struct TimedSample {
    uint64_t trial;
    bool high;
    double ns;
};

uint64_t NowNs()
{
    timespec ts{};
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    return static_cast<uint64_t>(ts.tv_sec) * 1000000000ULL + static_cast<uint64_t>(ts.tv_nsec);
}

double Mean(const std::vector<double>& values)
{
    return std::accumulate(values.begin(), values.end(), 0.0) / static_cast<double>(values.size());
}

double Variance(const std::vector<double>& values, double mean)
{
    double sum = 0.0;
    for (double value : values) sum += (value - mean) * (value - mean);
    return sum / static_cast<double>(values.size() - 1);
}

double Median(std::vector<double> values)
{
    const auto middle = values.begin() + values.size() / 2;
    std::nth_element(values.begin(), middle, values.end());
    return *middle;
}

double Percentile(std::vector<double> values, double fraction)
{
    const size_t index = static_cast<size_t>(fraction * static_cast<double>(values.size() - 1));
    auto middle = values.begin() + index;
    std::nth_element(values.begin(), middle, values.end());
    return *middle;
}

void PinToCpu(int cpu)
{
    cpu_set_t set;
    CPU_ZERO(&set);
    CPU_SET(cpu, &set);
    (void)sched_setaffinity(0, sizeof(set), &set);
}

bool EllswiftCreate(secp256k1_context* ctx, const Inputs& in, bool high, uint64_t, volatile unsigned char* sink)
{
    std::array<unsigned char, 64> ell{};
    const auto& key = high ? in.high : in.low;
    const int ret = secp256k1_ellswift_create(ctx, ell.data(), key.data(), in.aux.data());
    *sink ^= static_cast<unsigned char>(ell[0] ^ ell[63]);
    return ret == 1;
}

bool EllswiftXdh(secp256k1_context* ctx, const Inputs& in, bool high, uint64_t, volatile unsigned char* sink)
{
    std::array<unsigned char, 32> output{};
    const auto& key = high ? in.high : in.low;
    const int ret = secp256k1_ellswift_xdh(
        ctx, output.data(), in.ell_a.data(), in.ell_b.data(), key.data(), 0,
        secp256k1_ellswift_xdh_hash_function_bip324, nullptr);
    *sink ^= static_cast<unsigned char>(output[0] ^ output[31]);
    return ret == 1;
}

bool SilentSender(secp256k1_context* ctx, const Inputs& in, bool high, uint64_t, volatile unsigned char* sink)
{
    secp256k1_silentpayments_recipient recipient = in.recipient;
    secp256k1_xonly_pubkey generated_output{};
    secp256k1_xonly_pubkey* generated_outputs[1] = {&generated_output};
    const secp256k1_silentpayments_recipient* recipients[1] = {&recipient};
    const unsigned char* seckeys[1] = {(high ? in.high : in.low).data()};
    const int ret = secp256k1_silentpayments_sender_create_outputs(
        ctx, generated_outputs, recipients, 1, in.outpoint.data(), nullptr, 0, seckeys, 1);
    *sink ^= static_cast<unsigned char>(generated_output.data[0] ^ generated_output.data[31]);
    return ret == 1;
}

bool SilentLabel(secp256k1_context* ctx, const Inputs& in, bool high, uint64_t, volatile unsigned char* sink)
{
    secp256k1_silentpayments_label label{};
    std::array<unsigned char, 32> tweak{};
    const auto& key = high ? in.high : in.low;
    const int ret = secp256k1_silentpayments_recipient_label_create(ctx, &label, tweak.data(), key.data(), 0);
    *sink ^= static_cast<unsigned char>(label.data[0] ^ tweak[31]);
    return ret == 1;
}

bool SilentScan(secp256k1_context* ctx, const Inputs& in, bool high, uint64_t, volatile unsigned char* sink)
{
    secp256k1_silentpayments_found_output found{};
    secp256k1_silentpayments_found_output* found_outputs[1] = {&found};
    const secp256k1_xonly_pubkey* tx_outputs[1] = {&in.tx_output};
    const auto& key = high ? in.high : in.low;
    uint32_t n_found_outputs = 0;
    const int ret = secp256k1_silentpayments_recipient_scan_outputs(
        ctx, found_outputs, &n_found_outputs, tx_outputs, 1, key.data(), &in.prevouts_summary,
        &in.spend_pubkey, nullptr, nullptr);
    *sink ^= static_cast<unsigned char>(found.output.data[0] ^ found.tweak[31] ^ n_found_outputs);
    return ret == 1;
}

bool SameKey(Operation operation, secp256k1_context* ctx, const Inputs& in, bool, uint64_t trial, volatile unsigned char* sink)
{
    return operation(ctx, in, false, trial, sink);
}

void PrintRaw(const char* name, const std::vector<TimedSample>& raw)
{
    for (const TimedSample& sample : raw) {
        std::printf("raw operation=%s trial=%llu class=%s ns=%.2f\n", name,
                    static_cast<unsigned long long>(sample.trial), sample.high ? "high" : "low", sample.ns);
    }
}

bool Measure(const char* name, secp256k1_context* ctx, const Inputs& in, Operation operation,
             uint64_t trials, uint64_t rounds, uint64_t counter_base, volatile unsigned char* sink)
{
    std::vector<double> low_samples;
    std::vector<double> high_samples;
    std::vector<TimedSample> raw;
    low_samples.reserve(trials);
    high_samples.reserve(trials);
    raw.reserve(trials * 2);

    for (uint64_t i = 0; i < 256; ++i) {
        if (!operation(ctx, in, (i & 1) != 0, counter_base + i, sink)) return false;
    }

    uint64_t state = 0x6c8e9cf570932bd1ULL;
    for (uint64_t trial = 0; trial < trials; ++trial) {
        state ^= state << 7;
        state ^= state >> 9;
        state ^= state << 8;
        const bool high_first = (state & 1) != 0;
        for (int position = 0; position < 2; ++position) {
            const bool high = high_first ^ (position != 0);
            const uint64_t start = NowNs();
            for (uint64_t round = 0; round < rounds; ++round) {
                if (!operation(ctx, in, high, counter_base + trial, sink)) return false;
            }
            const double elapsed = static_cast<double>(NowNs() - start) / static_cast<double>(rounds);
            raw.push_back({trial, high, elapsed});
            (high ? high_samples : low_samples).push_back(elapsed);
        }
    }

    const double low_mean = Mean(low_samples);
    const double high_mean = Mean(high_samples);
    const double standard_error = std::sqrt(Variance(low_samples, low_mean) / low_samples.size() +
                                            Variance(high_samples, high_mean) / high_samples.size());
    const double welch_t = (low_mean - high_mean) / standard_error;
    std::printf("summary operation=%s low_mean_ns=%.2f high_mean_ns=%.2f low_median_ns=%.2f high_median_ns=%.2f "
                "low_p95_ns=%.2f high_p95_ns=%.2f t=%.3f samples_per_class=%llu rounds=%llu\n",
                name, low_mean, high_mean, Median(low_samples), Median(high_samples),
                Percentile(low_samples, 0.95), Percentile(high_samples, 0.95), welch_t,
                static_cast<unsigned long long>(trials), static_cast<unsigned long long>(rounds));
    PrintRaw(name, raw);
    return true;
}

bool Randomize(secp256k1_context* ctx, uint64_t tag)
{
    std::array<unsigned char, 32> seed{};
    for (size_t i = 0; i < seed.size(); ++i) seed[i] = static_cast<unsigned char>((tag + i * 29) & 0xff);
    return secp256k1_context_randomize(ctx, seed.data()) != 0;
}

bool Prepare(secp256k1_context* ctx, Inputs& in)
{
    in.low[31] = 1;
    in.high.fill(0x7f);
    in.peer_secret.fill(0x33);
    in.peer_secret[31] = 7;
    in.spend_secret.fill(0x44);
    in.spend_secret[31] = 9;
    in.aux.fill(0xa5);
    for (size_t i = 0; i < in.outpoint.size(); ++i) in.outpoint[i] = static_cast<unsigned char>(i * 7 + 3);

    if (!secp256k1_ec_seckey_verify(ctx, in.low.data()) ||
        !secp256k1_ec_seckey_verify(ctx, in.high.data()) ||
        !secp256k1_ec_seckey_verify(ctx, in.peer_secret.data()) ||
        !secp256k1_ec_seckey_verify(ctx, in.spend_secret.data())) {
        return false;
    }
    if (!secp256k1_ec_pubkey_create(ctx, &in.peer, in.peer_secret.data()) ||
        !secp256k1_ec_pubkey_create(ctx, &in.scan_pubkey, in.peer_secret.data()) ||
        !secp256k1_ec_pubkey_create(ctx, &in.spend_pubkey, in.spend_secret.data())) {
        return false;
    }
    if (!secp256k1_ellswift_create(ctx, in.ell_a.data(), in.peer_secret.data(), in.aux.data()) ||
        !secp256k1_ellswift_create(ctx, in.ell_b.data(), in.spend_secret.data(), in.aux.data())) {
        return false;
    }

    in.recipient.scan_pubkey = in.scan_pubkey;
    in.recipient.spend_pubkey = in.spend_pubkey;
    in.recipient.index = 0;
    const secp256k1_pubkey* pubkeys[1] = {&in.peer};
    if (!secp256k1_silentpayments_recipient_prevouts_summary_create(
            ctx, &in.prevouts_summary, in.outpoint.data(), nullptr, 0, pubkeys, 1)) {
        return false;
    }
    int parity = 0;
    if (!secp256k1_xonly_pubkey_from_pubkey(ctx, &in.tx_output, &parity, &in.spend_pubkey)) return false;
    return true;
}

} // namespace

int main(int argc, char** argv)
{
    const int cpu = argc > 1 ? std::atoi(argv[1]) : 2;
    const uint64_t trials = argc > 2 ? std::strtoull(argv[2], nullptr, 10) : 1500;
    const uint64_t rounds = argc > 3 ? std::strtoull(argv[3], nullptr, 10) : 4;
    PinToCpu(cpu);

    volatile unsigned char sink = 0;
    for (uint64_t repetition = 0; repetition < 3; ++repetition) {
        secp256k1_context* ctx = secp256k1_context_create(SECP256K1_CONTEXT_SIGN | SECP256K1_CONTEXT_VERIFY);
        if (ctx == nullptr) return 1;
        Inputs in;
        if (!Randomize(ctx, 0x7100 + repetition) || !Prepare(ctx, in)) {
            secp256k1_context_destroy(ctx);
            return 1;
        }
        std::printf("repetition=%llu\n", static_cast<unsigned long long>(repetition));
        const uint64_t base = 0x100000ULL * (repetition + 1);
        if (!Randomize(ctx, base + 1) ||
            !Measure("ellswift_create", ctx, in, EllswiftCreate, trials, rounds, base + 0x1000, &sink) ||
            !Randomize(ctx, base + 2) ||
            !Measure("ellswift_xdh", ctx, in, EllswiftXdh, trials, rounds, base + 0x2000, &sink) ||
            !Randomize(ctx, base + 3) ||
            !Measure("silent_sender_create_outputs", ctx, in, SilentSender, trials, rounds, base + 0x3000, &sink) ||
            !Randomize(ctx, base + 4) ||
            !Measure("silent_recipient_label_create", ctx, in, SilentLabel, trials, rounds, base + 0x4000, &sink) ||
            !Randomize(ctx, base + 5) ||
            !Measure("silent_recipient_scan_outputs", ctx, in, SilentScan, trials, rounds, base + 0x5000, &sink) ||
            !Measure("silent_recipient_scan_outputs_same_key", ctx, in,
                     [](secp256k1_context* c, const Inputs& i, bool h, uint64_t t, volatile unsigned char* s) {
                         return SameKey(SilentScan, c, i, h, t, s);
                     }, trials, rounds, base + 0x6000, &sink)) {
            secp256k1_context_destroy(ctx);
            return 1;
        }
        secp256k1_context_destroy(ctx);
    }
    std::printf("sink=%u\n", static_cast<unsigned>(sink));
    return 0;
}
