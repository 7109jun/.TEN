/*
 * Windows 백엔드(미검증 — thread_windows.c 상단 주석 참고).
 */

#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>

#include "ten/socket.h"
#include "ten/memory.h"
#include "internal/error_internal.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct ten_socket {
    SOCKET fd;
};

static int g_wsa_initialized = 0;

static void ten__wsa_ensure_init(void)
{
    if (!g_wsa_initialized)
    {
        WSADATA wsa_data;
        WSAStartup(MAKEWORD(2, 2), &wsa_data);
        g_wsa_initialized = 1;
    }
}

static ten_socket_t *ten__socket_wrap(SOCKET fd)
{
    ten_socket_t *sock = (ten_socket_t *)ten_malloc(sizeof(ten_socket_t));
    if (sock == NULL)
    {
        closesocket(fd);
        return NULL;
    }
    sock->fd = fd;
    return sock;
}

static int ten__resolve(const char *host, uint16_t port, int socktype, struct addrinfo **out)
{
    struct addrinfo hints;
    char port_str[8];

    ten__wsa_ensure_init();

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = socktype;
    _snprintf_s(port_str, sizeof(port_str), _TRUNCATE, "%u", (unsigned int)port);

    return getaddrinfo(host, port_str, &hints, out);
}

ten_socket_t *ten_tcp_connect(const char *host, uint16_t port)
{
    struct addrinfo *res;
    struct addrinfo *rp;
    SOCKET fd = INVALID_SOCKET;

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
        if (fd == INVALID_SOCKET)
        {
            continue;
        }
        if (connect(fd, rp->ai_addr, (int)rp->ai_addrlen) == 0)
        {
            break;
        }
        closesocket(fd);
        fd = INVALID_SOCKET;
    }
    freeaddrinfo(res);

    if (fd == INVALID_SOCKET)
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
    SOCKET fd = INVALID_SOCKET;
    BOOL yes = TRUE;

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
        if (fd == INVALID_SOCKET)
        {
            continue;
        }
        setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (const char *)&yes, sizeof(yes));
        if (bind(fd, rp->ai_addr, (int)rp->ai_addrlen) == 0)
        {
            break;
        }
        closesocket(fd);
        fd = INVALID_SOCKET;
    }
    freeaddrinfo(res);

    if (fd == INVALID_SOCKET)
    {
        ten__set_error(TEN_ERROR_NETWORK);
        return NULL;
    }

    if (listen(fd, backlog) != 0)
    {
        closesocket(fd);
        ten__set_error(TEN_ERROR_NETWORK);
        return NULL;
    }

    ten__set_error(TEN_OK);
    return ten__socket_wrap(fd);
}

ten_socket_t *ten_tcp_accept(ten_socket_t *server)
{
    SOCKET fd;

    if (server == NULL)
    {
        ten__set_error(TEN_ERROR_NULL_ARGUMENT);
        return NULL;
    }

    fd = accept(server->fd, NULL, NULL);
    if (fd == INVALID_SOCKET)
    {
        ten__set_error(TEN_ERROR_NETWORK);
        return NULL;
    }

    ten__set_error(TEN_OK);
    return ten__socket_wrap(fd);
}

ten_socket_t *ten_udp_socket(void)
{
    SOCKET fd;
    ten__wsa_ensure_init();

    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd == INVALID_SOCKET)
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
    SOCKET fd = INVALID_SOCKET;

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
        if (fd == INVALID_SOCKET)
        {
            continue;
        }
        if (bind(fd, rp->ai_addr, (int)rp->ai_addrlen) == 0)
        {
            break;
        }
        closesocket(fd);
        fd = INVALID_SOCKET;
    }
    freeaddrinfo(res);

    if (fd == INVALID_SOCKET)
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
    int n;

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

    n = sendto(socket->fd, (const char *)data, (int)size, 0, res->ai_addr, (int)res->ai_addrlen);
    freeaddrinfo(res);

    if (n == SOCKET_ERROR)
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
    int addr_len = sizeof(addr);
    int n;

    if (socket == NULL || (buffer == NULL && size > 0))
    {
        ten__set_error(TEN_ERROR_NULL_ARGUMENT);
        return -1;
    }

    n = recvfrom(socket->fd, (char *)buffer, (int)size, 0, (struct sockaddr *)&addr, &addr_len);
    if (n == SOCKET_ERROR)
    {
        ten__set_error(TEN_ERROR_NETWORK);
        return -1;
    }

    if (out_host != NULL || out_port != NULL)
    {
        char host_buf[NI_MAXHOST];
        char port_buf[NI_MAXSERV];

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
    int n;

    if (socket == NULL || (data == NULL && size > 0))
    {
        ten__set_error(TEN_ERROR_NULL_ARGUMENT);
        return -1;
    }

    n = send(socket->fd, (const char *)data, (int)size, 0);
    if (n == SOCKET_ERROR)
    {
        ten__set_error(TEN_ERROR_NETWORK);
        return -1;
    }

    ten__set_error(TEN_OK);
    return (int64_t)n;
}

int64_t ten_socket_receive(ten_socket_t *socket, void *buffer, size_t size)
{
    int n;

    if (socket == NULL || (buffer == NULL && size > 0))
    {
        ten__set_error(TEN_ERROR_NULL_ARGUMENT);
        return -1;
    }

    n = recv(socket->fd, (char *)buffer, (int)size, 0);
    if (n == SOCKET_ERROR)
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
    closesocket(socket->fd);
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

    tv.tv_sec = (long)(timeout_ms / 1000);
    tv.tv_usec = (long)((timeout_ms % 1000) * 1000);

    /* Windows의 select()는 첫 인자를 무시한다(소켓 핸들이 fd 번호가
       아니므로). */
    rc = select(0, &readfds, NULL, NULL, &tv);
    if (rc == SOCKET_ERROR)
    {
        ten__set_error(TEN_ERROR_NETWORK);
        return -1;
    }

    ten__set_error(TEN_OK);
    return (rc > 0) ? 1 : 0;
}
