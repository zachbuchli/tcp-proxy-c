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
#include <stdbool.h>
#include <sys/epoll.h>


typedef struct connection {
    int client_fd;
    int target_fd;
} connection;

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
extern void* get_in_addr(struct sockaddr *sa);


/**
 * handles EPOLLIN events from a client target connection pair found in the conns array. 
 * returns 1 if successfull, -1 if not. 
*/
extern int handle_read_event(int conn_fd, connection *conns, int *conn_count, int epollfd, const int MAX_BUF_SIZE);


/**
 * removes connection structs from an array of connections. Decrements conn_count.
 * returns 1 if successful, -1 if not.
*/
extern int remove_conn(bool isClient, int conn_fd, connection *conn, int *conn_count);

/**
 * sends all bytes to sender.  Sometimes send doesnt send all the byets requested...
 * Stole this from:
 * https://beej.us/guide/bgnet/html/split-wide/slightly-advanced-techniques.html#poll
 * returns bytes sent or -1 for error
*/
extern int sendall(int sockfd, char *buf, int len);

#endif //SERVER_CLIENT_TCP_PROXY_H
