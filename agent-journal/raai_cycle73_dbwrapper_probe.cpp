#include <dbwrapper.h>

#include <cstdio>

int main()
{
    try {
        CDBWrapper db{{.path = "/dev/null/bitcoin-raii-cycle73",
                       .cache_bytes = 1 << 20,
                       .memory_only = false,
                       .wipe_data = false,
                       .obfuscate = false}};
        std::puts("unexpected success");
        return 2;
    } catch (const std::exception& error) {
        std::printf("caught=%s\n", error.what());
        return 0;
    }
}
