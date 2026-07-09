#include "mini_muduo/base/UniqueFd.h"


#include <cassert>
#include <cerrno>
#include <fcntl.h>
#include <unistd.h>

static void test_default_state()
{
    mini_muduo::UniqueFd unique_fd;

    assert(!unique_fd.valid());

    assert(unique_fd.get() == -1);
}

void test_destructor_closes_fd()
{
    int pipe_fds[2];
    int result = ::pipe(pipe_fds);
    assert(result == 0);
    int raw_fd = pipe_fds[0];

    {
        mini_muduo::UniqueFd owner(raw_fd);

        assert(owner.valid());
        assert(owner.get() == raw_fd);
    }

    errno = 0;
    result = ::fcntl(raw_fd, F_GETFD);

    assert(result == -1);
    assert(errno == EBADF);

    const int close_result = ::close(pipe_fds[1]);
    assert(close_result == 0);
}

int main()
{
    test_default_state();
    test_destructor_closes_fd();
    return 0;
}