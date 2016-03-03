#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "net.h"

bool on_init(int argc, const char *argv[], int sock);
void on_timer(void);
void on_recv(void);

#ifdef WIN32
#else

#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/timerfd.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>

static int timer_init(void)
{
    static const struct itimerspec itimer = {
        .it_interval = {.tv_nsec = 1000000000 / FPS},
        .it_value = {.tv_nsec = 1000000000 / FPS},
    };

    int fd;

    fd = timerfd_create(CLOCK_MONOTONIC, 0);
    if (fd < 0)
        return fd;

    if (timerfd_settime(fd, 0, &itimer, NULL)) {
        close(fd);
        return -1;
    }

    return fd;
}

static int epoll_init(int sock, int tfd)
{
    int fd;
    struct epoll_event ev;

    fd = epoll_create(1);
    if (fd < 0)
        return fd;

    ev.events = EPOLLIN;// | EPOLLET; TODO: nonblocking+EPOLLET
    ev.data.fd = 0;
    if (epoll_ctl(fd, EPOLL_CTL_ADD, sock, &ev) < 0)
        goto fail;

    ev.events = EPOLLIN;
    ev.data.fd = -1;
    if (epoll_ctl(fd, EPOLL_CTL_ADD, tfd, &ev) < 0)
        goto fail;

    return fd;
fail:
    close(fd);
    return -1;
}

int main(int argc, const char *argv[])
{
    int sock, efd, tfd, len;
    struct epoll_event ev;
    uint8_t buf[65536];

    sock = net_sock();
    if (sock < 0)
        return 0;

    if (!net_bind(sock, htons(PORT)))
        goto fail_close_sock;

    tfd = timer_init();
    if (tfd < 0)
        goto fail_close_sock;

    efd = epoll_init(sock, tfd);
    if (efd < 0)
        goto fail_close_tfd;

    if (!on_init(argc, argv, sock))
        goto fail_close_all;

    do {
        len = epoll_wait(efd, &ev, 1, -1);
        if (len < 0) {
            printf("epoll error %u\n", errno);
            continue;
        }

        if (ev.data.fd < 0) { /* timer event */
            len = read(tfd, buf, 8);
            on_timer();
            continue;
        }
        on_recv();
    } while(1);

fail_close_all:
    close(efd);
fail_close_tfd:
    close(tfd);
fail_close_sock:
    close(sock);
    return 0;
}
#endif
