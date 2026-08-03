// LD_PRELOAD fault injector: force a short write on the 8-byte xor.dat key.
// Mirrors the parallel campaign's reproducer for d7d3559a30:
// accept only 1 of the 8 bytes on the FIRST 8-byte fwrite (the xor key).
#define _GNU_SOURCE
#include <dlfcn.h>
#include <stdio.h>

static size_t (*real_fwrite)(const void*, size_t, size_t, FILE*) = NULL;
static int armed = 1;

size_t fwrite(const void* ptr, size_t size, size_t nmemb, FILE* stream) {
    if (!real_fwrite) real_fwrite = dlsym(RTLD_NEXT, "fwrite");
    if (armed && size * nmemb == 8) {
        armed = 0;
        real_fwrite(ptr, 1, 1, stream);
        return size == 1 ? 1 : 0; // short: fewer items than requested
    }
    return real_fwrite(ptr, size, nmemb, stream);
}
