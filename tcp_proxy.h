#ifndef SERVER_CLIENT_TCP_PROXY_H
#define SERVER_CLIENT_TCP_PROXY_H

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <poll.h>

/**
 * returns fd that listens for connections at ip_addr and port. returns -1 if error.
 * @param ip_addr
 * @param port
 * @param backlog
 * @return sfd
 */
extern int create_listener(const char *ip_addr, const char *port, const int backlog);


/**
 * returns fd for connection to ip_addr and port. Returns -1 if error.
 * @param ip_addr
 * @param port
 * @return sfd
 */
extern int create_target_conn(const char *ip_addr, const char *port);


/**
 * Get sockaddr, IPv4 or IPv6
 * @param sa
 * @return
 * snagged this one from https://beej.us/guide/bgnet/html/split/slightly-advanced-techniques.html
 */
extern void *get_in_addr(struct sockaddr *sa);


/**
 * adds a new file descriptor to the set.
 * @param pfds
 * @param newfd
 * @param fd_count
 * @param fd_size
 * snagged this from https://beej.us/guide/bgnet/html/split/slightly-advanced-techniques.html
 */
extern int add_to_pfds(struct pollfd *pfds[], int newfd, int *fd_count, int *fd_size);

/**
 * Remove a file descriptor from the set
 * @param pfds
 * @param i
 * @param fd_count
 * snagged this from https://beej.us/guide/bgnet/html/split/slightly-advanced-techniques.html
 */
extern void del_from_pfds(struct pollfd pfds[], int i, int *fd_count);

#endif //SERVER_CLIENT_TCP_PROXY_H
