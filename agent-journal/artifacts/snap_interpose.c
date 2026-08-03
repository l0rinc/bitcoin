// LD_PRELOAD fault injector: short-write the snapshot base_blockhash file.
// Tracks fopen* streams whose path ends in "base_blockhash"; the first
// fwrite on such a stream accepts 1 byte and reports a short write,
// making AutoFile::write throw ios_base::failure (write path of
// WriteSnapshotBaseBlockhash, goal38 / 1fe00d5a05 verification).
#define _GNU_SOURCE
#include <dlfcn.h>
#include <stdio.h>
#include <string.h>

static FILE* (*real_fopen)(const char*, const char*) = NULL;
static FILE* (*real_fopen64)(const char*, const char*) = NULL;
static size_t (*real_fwrite)(const void*, size_t, size_t, FILE*) = NULL;
static FILE* target = NULL;
static int armed = 1;

static FILE* track(FILE* f, const char* path) {
    if (f && armed && path) {
        const char* base = strrchr(path, '/');
        base = base ? base + 1 : path;
        if (strcmp(base, "base_blockhash") == 0) target = f;
    }
    return f;
}

FILE* fopen(const char* path, const char* mode) {
    if (!real_fopen) real_fopen = dlsym(RTLD_NEXT, "fopen");
    return track(real_fopen(path, mode), path);
}
FILE* fopen64(const char* path, const char* mode) {
    if (!real_fopen64) real_fopen64 = dlsym(RTLD_NEXT, "fopen64");
    return track(real_fopen64(path, mode), path);
}

size_t fwrite(const void* ptr, size_t size, size_t nmemb, FILE* stream) {
    if (!real_fwrite) real_fwrite = dlsym(RTLD_NEXT, "fwrite");
    if (armed && stream == target && target) {
        armed = 0;
        real_fwrite(ptr, 1, 1, stream);      // 1 byte lands on disk
        return size == 1 ? 1 : 0;            // short write -> AutoFile throws
    }
    return real_fwrite(ptr, size, nmemb, stream);
}
