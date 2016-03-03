#include "net.h"

#ifdef WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <mswsock.h>
//typedef WSABUF netbuf_t;
#else
#include <sys/socket.h>
#include <netdb.h>
#endif

//static int i;

int net_send(int sock, const addr_t *addr, const void *data, int len)
{
    //if (!(i++ & 3))
    //    return -1;

    return sendto(sock, data, len, 0, (struct sockaddr*) addr, sizeof(*addr));
}

int net_sendmsg(int sock, const addr_t *addr, netbuf_t *buf, int num_buf)
{
    //if (!(i++ & 3))
    //    return -1;

    #ifdef WIN32
    DWORD len;
    WSAMSG msg = {
        (struct sockaddr*) addr, sizeof(*addr),
        (WSABUF*) buf, num_buf,
        {0, 0}, //control
        0 //flags
    };
    if (WSASendMsg(sock, &msg, 0, &len, 0, 0))
        return -1;
    return len;
#else
    struct msghdr msg = {
        (struct sockaddr*) addr, sizeof(*addr),
        buf, num_buf,
        0, 0, //control
        0 //flags
    };
    return sendmsg(sock, &msg, 0);
#endif
}

netbuf_t net_buf(const void *data, size_t len)
{
    netbuf_t buf = {
#ifdef WIN32
        len, (void*) data
#else
        (void*) data, len
#endif
    };
    return buf;
}

int net_recv(int sock, addr_t *addr, void *data, int maxlen)
{
    socklen_t addrlen = sizeof(*addr);
    return recvfrom(sock, data, maxlen, 0, (struct sockaddr*) addr, &addrlen);
}

bool net_getaddr(addr_t *addr, const char *name, const char *port)
{
    struct addrinfo *root, *info;
    bool res;

    if (getaddrinfo(name, port, 0, &root))
        return 0;

    info = root;
    res = 0;
    do {
        if (info->ai_socktype && info->ai_socktype != SOCK_DGRAM)
            continue;

        if (info->ai_addr->sa_family == AF_INET) {
            memcpy(addr, info->ai_addr, sizeof(addr_t));
            res = 1;
            break;
        }

    } while ((info = info->ai_next));
    freeaddrinfo(root);

    return res;
}

void net_addr_ipv4(addr_t *addr, uint32_t ip, uint16_t port)
{
    addr->family = AF_INET;
    addr->ip = ip;
    addr->port = port;
}

int net_sock(void)
{
    return socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
}

int net_sock_nb(void)
{
    int sock;
#ifdef WIN32
    u_long mode;
    sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock < 0)
        return sock;

    mode = 1;
    if (ioctlsocket(sock, FIONBIO, &mode)) {
        closesocket(sock);
        return -1;
    }
#else
    sock = socket(AF_INET, SOCK_DGRAM | SOCK_NONBLOCK, IPPROTO_UDP);
    if (sock < 0)
        return sock;
#endif
    return sock;
}

void net_close(int sock)
{
#ifdef WIN32
    closesocket(sock);
#else
    close(sock);
#endif
}

bool net_bind(int sock, uint16_t port)
{
    addr_t addr;

    net_addr_ipv4(&addr, 0, port);
    return (bind(sock, (struct sockaddr*) &addr, sizeof(addr)) == 0);
}

bool net_attach(int sock, void *arg)
{
#ifdef WIN32
    return (WSAAsyncSelect(sock, (HWND) arg, WM_USER, FD_READ) == 0);
#else
    (void) sock;
    (void) arg;
    return 0;
#endif
}
