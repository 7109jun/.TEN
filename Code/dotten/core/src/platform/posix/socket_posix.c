#if !defined(_POSIX_C_SOURCE)
#define _POSIX_C_SOURCE 200809L
#endif

#include "ten/socket.h"
#include "ten/memory.h"
#include "internal/error_internal.h"

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

struct ten_socket {
    int fd;
};

static ten_socket_t *ten__socket_wrap(int fd)
{
    ten_socket_t *sock = (ten_socket_t *)ten_malloc(sizeof(ten_socket_t));
    if (sock == NULL)
    {
        close(fd);
        return NULL;
    }
    sock->fd = fd;
    return sock;
}

static int ten__resolve(const char *host, uint16_t port, int socktype, struct addrinfo **out)
{
    struct addrinfo hints;
    char port_str[8];

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = socktype;
    snprintf(port_str, sizeof(port_str), "%u", (unsigned int)port);

    return getaddrinfo(host, port_str, &hints, out);
}

ten_socket_t *ten_tcp_connect(const char *host, uint16_t port)
{
    struct addrinfo *res;
    struct addrinfo *rp;
    int fd = -1;

    if (host == NULL)
    {
        ten__set_error(TEN_ERROR_NULL_ARGUMENT);
        return NULL;
    }

    if (ten__resolve(host, port, SOCK_STREAM, &res) != 0)
    {
        ten__set_error(TEN_ERROR_NETWORK);
        return NULL;
    }

    for (rp = res; rp != NULL; rp = rp->ai_next)
    {
        fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (fd < 0)
        {
            continue;
        }
        if (connect(fd, rp->ai_addr, rp->ai_addrlen) == 0)
        {
            break;
        }
        close(fd);
        fd = -1;
    }
    freeaddrinfo(res);

    if (fd < 0)
    {
        ten__set_error(TEN_ERROR_NETWORK);
        return NULL;
    }

    ten__set_error(TEN_OK);
    return ten__socket_wrap(fd);
}

ten_socket_t *ten_tcp_listen(const char *host, uint16_t port, int backlog)
{
    struct addrinfo *res;
    struct addrinfo *rp;
    int fd = -1;
    int yes = 1;

    if (host == NULL)
    {
        ten__set_error(TEN_ERROR_NULL_ARGUMENT);
        return NULL;
    }

    if (ten__resolve(host, port, SOCK_STREAM, &res) != 0)
    {
        ten__set_error(TEN_ERROR_NETWORK);
        return NULL;
    }

    for (rp = res; rp != NULL; rp = rp->ai_next)
    {
        fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (fd < 0)
        {
            continue;
        }
        setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
        if (bind(fd, rp->ai_addr, rp->ai_addrlen) == 0)
        {
            break;
        }
        close(fd);
        fd = -1;
    }
    freeaddrinfo(res);

    if (fd < 0)
    {
        ten__set_error(TEN_ERROR_NETWORK);
        return NULL;
    }

    if (listen(fd, backlog) != 0)
    {
        close(fd);
        ten__set_error(TEN_ERROR_NETWORK);
        return NULL;
    }

    ten__set_error(TEN_OK);
    return ten__socket_wrap(fd);
}

ten_socket_t *ten_tcp_accept(ten_socket_t *server)
{
    int fd;

    if (server == NULL)
    {
        ten__set_error(TEN_ERROR_NULL_ARGUMENT);
        return NULL;
    }

    fd = accept(server->fd, NULL, NULL);
    if (fd < 0)
    {
        ten__set_error(TEN_ERROR_NETWORK);
        return NULL;
    }

    ten__set_error(TEN_OK);
    return ten__socket_wrap(fd);
}

ten_socket_t *ten_udp_socket(void)
{
    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0)
    {
        ten__set_error(TEN_ERROR_NETWORK);
        return NULL;
    }
    ten__set_error(TEN_OK);
    return ten__socket_wrap(fd);
}

ten_socket_t *ten_udp_bind(const char *host, uint16_t port)
{
    struct addrinfo *res;
    struct addrinfo *rp;
    int fd = -1;

    if (host == NULL)
    {
        ten__set_error(TEN_ERROR_NULL_ARGUMENT);
        return NULL;
    }

    if (ten__resolve(host, port, SOCK_DGRAM, &res) != 0)
    {
        ten__set_error(TEN_ERROR_NETWORK);
        return NULL;
    }

    for (rp = res; rp != NULL; rp = rp->ai_next)
    {
        fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (fd < 0)
        {
            continue;
        }
        if (bind(fd, rp->ai_addr, rp->ai_addrlen) == 0)
        {
            break;
        }
        close(fd);
        fd = -1;
    }
    freeaddrinfo(res);

    if (fd < 0)
    {
        ten__set_error(TEN_ERROR_NETWORK);
        return NULL;
    }

    ten__set_error(TEN_OK);
    return ten__socket_wrap(fd);
}

int64_t ten_udp_send_to(ten_socket_t *socket, const char *host, uint16_t port,
                         const void *data, size_t size)
{
    struct addrinfo *res;
    ssize_t n;

    if (socket == NULL || host == NULL || (data == NULL && size > 0))
    {
        ten__set_error(TEN_ERROR_NULL_ARGUMENT);
        return -1;
    }

    if (ten__resolve(host, port, SOCK_DGRAM, &res) != 0)
    {
        ten__set_error(TEN_ERROR_NETWORK);
        return -1;
    }

    n = sendto(socket->fd, data, size, 0, res->ai_addr, res->ai_addrlen);
    freeaddrinfo(res);

    if (n < 0)
    {
        ten__set_error(TEN_ERROR_NETWORK);
        return -1;
    }

    ten__set_error(TEN_OK);
    return (int64_t)n;
}

int64_t ten_udp_receive_from(ten_socket_t *socket, void *buffer, size_t size,
                              char *out_host, size_t out_host_size, uint16_t *out_port)
{
    struct sockaddr_storage addr;
    socklen_t addr_len = sizeof(addr);
    ssize_t n;

    if (socket == NULL || (buffer == NULL && size > 0))
    {
        ten__set_error(TEN_ERROR_NULL_ARGUMENT);
        return -1;
    }

    n = recvfrom(socket->fd, buffer, size, 0, (struct sockaddr *)&addr, &addr_len);
    if (n < 0)
    {
        ten__set_error(TEN_ERROR_NETWORK);
        return -1;
    }

    if (out_host != NULL || out_port != NULL)
    {
        /* NI_MAXHOST/NI_MAXSERV는 표준 POSIX 매크로가 아니라서(글ibc에서도
           노출 여부가 feature test macro에 따라 달라짐) 직접 넉넉한
           크기를 사용한다. */
        char host_buf[256];
        char port_buf[32];

        getnameinfo((struct sockaddr *)&addr, addr_len, host_buf, sizeof(host_buf),
                    port_buf, sizeof(port_buf), NI_NUMERICHOST | NI_NUMERICSERV);

        if (out_host != NULL && out_host_size > 0)
        {
            size_t copy_len = strlen(host_buf);
            if (copy_len >= out_host_size)
            {
                copy_len = out_host_size - 1;
            }
            memcpy(out_host, host_buf, copy_len);
            out_host[copy_len] = '\0';
        }

        if (out_port != NULL)
        {
            *out_port = (uint16_t)atoi(port_buf);
        }
    }

    ten__set_error(TEN_OK);
    return (int64_t)n;
}

int64_t ten_socket_send(ten_socket_t *socket, const void *data, size_t size)
{
    ssize_t n;

    if (socket == NULL || (data == NULL && size > 0))
    {
        ten__set_error(TEN_ERROR_NULL_ARGUMENT);
        return -1;
    }

    n = send(socket->fd, data, size, 0);
    if (n < 0)
    {
        ten__set_error(TEN_ERROR_NETWORK);
        return -1;
    }

    ten__set_error(TEN_OK);
    return (int64_t)n;
}

int64_t ten_socket_receive(ten_socket_t *socket, void *buffer, size_t size)
{
    ssize_t n;

    if (socket == NULL || (buffer == NULL && size > 0))
    {
        ten__set_error(TEN_ERROR_NULL_ARGUMENT);
        return -1;
    }

    n = recv(socket->fd, buffer, size, 0);
    if (n < 0)
    {
        ten__set_error(TEN_ERROR_NETWORK);
        return -1;
    }

    ten__set_error(TEN_OK);
    return (int64_t)n;
}

void ten_socket_close(ten_socket_t *socket)
{
    if (socket == NULL)
    {
        return;
    }
    close(socket->fd);
    ten_free(socket);
}

intptr_t ten_socket_native_handle(const ten_socket_t *socket)
{
    if (socket == NULL)
    {
        ten__set_error(TEN_ERROR_NULL_ARGUMENT);
        return -1;
    }
    ten__set_error(TEN_OK);
    return (intptr_t)socket->fd;
}

int ten_socket_poll_readable(const ten_socket_t *socket, int timeout_ms)
{
    fd_set readfds;
    struct timeval tv;
    int rc;

    if (socket == NULL)
    {
        ten__set_error(TEN_ERROR_NULL_ARGUMENT);
        return -1;
    }

    FD_ZERO(&readfds);
    FD_SET(socket->fd, &readfds);

    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;

    rc = select(socket->fd + 1, &readfds, NULL, NULL, &tv);
    if (rc < 0)
    {
        ten__set_error(TEN_ERROR_NETWORK);
        return -1;
    }

    ten__set_error(TEN_OK);
    return (rc > 0) ? 1 : 0;
}
