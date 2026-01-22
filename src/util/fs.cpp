// Copyright (c) 2017-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <util/check.h>
#include <util/fs.h>

#include <util/check.h>
#include <util/syserror.h>

#include <cerrno>
#include <string>

#ifndef WIN32
#include <fcntl.h>
#include <unistd.h>
#else
#include <codecvt>
#include <cstring>
#include <fcntl.h>
#include <io.h>
#include <limits>
#include <locale>
#include <share.h>
#include <sys/stat.h>
#include <windows.h>
#endif

namespace fsbridge {

FILE *fopen(const fs::path& p, const char *mode)
{
    const bool exclusive{strchr(mode, 'x') != nullptr};
#ifndef WIN32
    Assume((!exclusive) || !strcmp(mode, "wbx"));
    return ::fopen(p.c_str(), mode);
#else
    if (exclusive) {
        Assert(!strcmp(mode, "wbx"));
        int fd;
        if (::_wsopen_s(&fd, p.wstring().c_str(), _O_WRONLY | _O_CREAT | _O_EXCL | _O_BINARY, _SH_DENYNO, _S_IREAD | _S_IWRITE)) {
            return nullptr;
        }
        FILE* fp = ::_fdopen(fd, "wb");
        if (!fp) ::_close(fd);
        return fp;
    }

    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>,wchar_t> utf8_cvt;
    return ::_wfopen(p.wstring().c_str(), utf8_cvt.from_bytes(mode).c_str());
#endif
}

fs::path AbsPathJoin(const fs::path& base, const fs::path& path)
{
    assert(base.is_absolute());
    return path.empty() ? base : fs::path(base / path);
}

#ifndef WIN32

static std::string GetErrorReason()
{
    return SysErrorString(errno);
}

FileLock::FileLock(const fs::path& file)
{
    fd = open(file.c_str(), O_RDWR);
    if (fd == -1) {
        reason = GetErrorReason();
    }
}

FileLock::~FileLock()
{
    if (fd != -1) {
        close(fd);
    }
}

bool FileLock::TryLock()
{
    if (fd == -1) {
        return false;
    }

    struct flock lock;
    lock.l_type = F_WRLCK;
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;
    if (fcntl(fd, F_SETLK, &lock) == -1) {
        reason = GetErrorReason();
        return false;
    }

    return true;
}
#else

static std::string GetErrorReason() {
    return Win32ErrorString(GetLastError());
}

FileLock::FileLock(const fs::path& file)
{
    hFile = CreateFileW(file.wstring().c_str(),  GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile == INVALID_HANDLE_VALUE) {
        reason = GetErrorReason();
    }
}

FileLock::~FileLock()
{
    if (hFile != INVALID_HANDLE_VALUE) {
        CloseHandle(hFile);
    }
}

bool FileLock::TryLock()
{
    if (hFile == INVALID_HANDLE_VALUE) {
        return false;
    }
    _OVERLAPPED overlapped = {};
    if (!LockFileEx(hFile, LOCKFILE_EXCLUSIVE_LOCK | LOCKFILE_FAIL_IMMEDIATELY, 0, std::numeric_limits<DWORD>::max(), std::numeric_limits<DWORD>::max(), &overlapped)) {
        reason = GetErrorReason();
        return false;
    }
    return true;
}
#endif

std::string get_filesystem_error_message(const fs::filesystem_error& e)
{
    return e.code().message();
}

} // namespace fsbridge
