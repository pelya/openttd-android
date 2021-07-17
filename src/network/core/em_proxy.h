/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @file em_proxy.h Emscripten socket proxy
 */

#ifndef NETWORK_CORE_EM_PROXY_H
#define NETWORK_CORE_EM_PROXY_H

int em_proxy_initialize_network(void);

int em_proxy_socket(int domain, int type, int protocol);
int em_proxy_closesocket(int socket);
int em_proxy_shutdown(int socket, int how);
int em_proxy_bind(int socket, const struct sockaddr *address, socklen_t address_len);
int em_proxy_connect(int socket, const struct sockaddr *address, socklen_t address_len);
int em_proxy_listen(int socket, int backlog);
int em_proxy_accept(int socket, struct sockaddr *address, socklen_t *address_len);
ssize_t em_proxy_send(int socket, const void *message, size_t length, int flags);
ssize_t em_proxy_recv(int socket, void *buffer, size_t length, int flags);
ssize_t em_proxy_sendto(int socket, const void *message, size_t length, int flags, const struct sockaddr *dest_addr, socklen_t dest_len);
ssize_t em_proxy_recvfrom(int socket, void *buffer, size_t length, int flags, struct sockaddr *address, socklen_t *address_len);
int em_proxy_getsockopt(int socket, int level, int option_name, void *option_value, socklen_t *option_len);
int em_proxy_setsockopt(int socket, int level, int option_name, const void *option_value, socklen_t option_len);
int em_proxy_getaddrinfo(const char *node, const char *service, const struct addrinfo *hints, struct addrinfo **res);
void em_proxy_freeaddrinfo(struct addrinfo *res);
int em_proxy_getnameinfo(const struct sockaddr *addr, socklen_t addrlen, char *host, socklen_t hostlen, char *serv, socklen_t servlen, int flags);
int em_proxy_select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout);

#ifndef EM_PROXY_NO_MACROS
#	define socket       em_proxy_socket
#	undef  closesocket
#	define closesocket  em_proxy_closesocket
#	define shutdown     em_proxy_shutdown
#	define bind         em_proxy_bind
#	define connect      em_proxy_connect
#	define listen       em_proxy_listen
#	define accept       em_proxy_accept
#	define send         em_proxy_send
#	define recv         em_proxy_recv
#	define sendto       em_proxy_sendto
#	define recvfrom     em_proxy_recvfrom
#	define getsockopt   em_proxy_getsockopt
#	define setsockopt   em_proxy_setsockopt
#	define getaddrinfo  em_proxy_getaddrinfo
#	define freeaddrinfo em_proxy_freeaddrinfo
#	define getnameinfo  em_proxy_getnameinfo
#	define select       em_proxy_select
#endif /* EM_PROXY_NO_MACROS */

#endif /* NETWORK_CORE_EM_PROXY_H */
