#ifndef NET_H
#define NET_H

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#ifdef WIN32
//dont polute with windows headers
typedef struct {unsigned long len; char *buf;} netbuf_t; //WSABUF
#else
#include <sys/uio.h>
typedef struct iovec netbuf_t;
#endif

typedef struct {
    union {
        struct {
            uint16_t family, port;
            uint32_t ip;
        };
        uint64_t cmp;
    };
    uint8_t padding[8];
} addr_t;

/* send */
int net_send(int sock, const addr_t *addr, const void *data, int len);
int net_sendmsg(int sock, const addr_t *addr, netbuf_t *buf, int num_buf);

netbuf_t net_buf(const void *data, size_t len);

/* recv */
int net_recv(int sock, addr_t *addr, void *data, int maxlen);
/* easy getaddrinfo() */
bool net_getaddr(addr_t *addr, const char *name, const char *port);
/* addr struct from ip + port*/
void net_addr_ipv4(addr_t *addr, uint32_t ip, uint16_t port);
/* udp socket */
int net_sock(void);
/* nonblocking udp socket */
int net_sock_nb(void);
/* close */
void net_close(int sock);
/* bind() */
bool net_bind(int sock, uint16_t port);
/* */
bool net_attach(int sock, void *arg);

#endif
