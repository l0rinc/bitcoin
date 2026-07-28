#include <util/tokenpipe.h>

#include <csignal>
#include <utility>

int main()
{
    std::signal(SIGPIPE, SIG_IGN);

    auto pipe = TokenPipe::Make();
    if (!pipe) return 1;

    TokenPipeEnd read_end = pipe->TakeReadEnd();
    TokenPipeEnd write_end = pipe->TakeWriteEnd();
    if (!read_end.IsOpen() || !write_end.IsOpen()) return 2;
    if (pipe->TakeReadEnd().IsOpen() || pipe->TakeWriteEnd().IsOpen()) return 3;

    if (write_end.TokenWrite(0) != 0) return 4;
    if (read_end.TokenRead() != 0) return 5;
    if (write_end.TokenWrite(255) != 0) return 6;
    if (read_end.TokenRead() != 255) return 7;

    write_end.Close();
    if (read_end.TokenRead() != TokenPipeEnd::TS_EOS) return 8;
    read_end.Close();

    auto moved_pipe = TokenPipe::Make();
    if (!moved_pipe) return 9;
    TokenPipe moved_to(std::move(*moved_pipe));
    auto moved_again_opt = TokenPipe::Make();
    if (!moved_again_opt) return 10;
    TokenPipe moved_again(std::move(*moved_again_opt));
    moved_again = std::move(moved_to);
    TokenPipeEnd moved_read = moved_again.TakeReadEnd();
    TokenPipeEnd moved_write = moved_again.TakeWriteEnd();
    TokenPipeEnd replacement;
    replacement = std::move(moved_write);
    if (moved_write.IsOpen() || !replacement.IsOpen()) return 11;
    if (replacement.TokenWrite(42) != 0) return 12;
    if (moved_read.TokenRead() != 42) return 13;
    replacement.Close();
    if (moved_read.TokenRead() != TokenPipeEnd::TS_EOS) return 14;

    auto broken_pipe = TokenPipe::Make();
    if (!broken_pipe) return 15;
    TokenPipeEnd broken_read = broken_pipe->TakeReadEnd();
    TokenPipeEnd broken_write = broken_pipe->TakeWriteEnd();
    broken_read.Close();
    if (broken_write.TokenWrite(7) != TokenPipeEnd::TS_EOS) return 16;

    return 0;
}
