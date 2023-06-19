// tcp_proxy.c
#include "tcp_proxy.h"

int create_listener(const char *ip_addr, const char *port, const int backlog){

    int status;
    struct addrinfo hints;
    struct addrinfo *servinfo;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    // Get socket info for net stuff
    status = getaddrinfo(ip_addr, port, &hints, &servinfo);
    if(status != 0){
        freeaddrinfo(servinfo);
        return -1;
    }

    int sockfd;
    struct addrinfo* rp;
    // need to iterate through  addrinfo linked list
    for (rp = servinfo; rp != NULL; rp = rp->ai_next){
        sockfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if(sockfd == -1) continue;
        if (bind(sockfd, rp->ai_addr, rp->ai_addrlen) == 0) break;
        close(sockfd); // try again
    }

    if(rp == NULL) {
        freeaddrinfo(servinfo);
        return -1;
    }

    // open socket for connections
    if(listen(sockfd, backlog) == -1){
        freeaddrinfo(servinfo);
        return -1;
    }

    freeaddrinfo(servinfo);
    return sockfd;
}


int create_target_conn(const char *ip_addr, const char *port) {
    int status;
    struct addrinfo hints;
    struct addrinfo *servinfo;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    // Get socket info for net stuff
    status = getaddrinfo(ip_addr, port, &hints, &servinfo);
    if(status != 0){
        return -1;
    }

    int sockfd;
    struct addrinfo* rp;
    // need to iterate through  addrinfo linked list
    for (rp = servinfo; rp != NULL; rp = rp->ai_next){
        sockfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if(sockfd == -1) continue;
        if (connect(sockfd, rp->ai_addr, rp->ai_addrlen) != -1) break;
        close(sockfd); // try again
    }

    if(rp == NULL) {
        freeaddrinfo(servinfo);
        return -1;
    }

    freeaddrinfo(servinfo);
    return sockfd;
}

void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int add_to_pfds(struct pollfd *pfds[], int newfd, int *fd_count, int *fd_size)
{
    // If we don't have room, add more space in the pfds array
    // todo: should add an pfds size max.
    if (*fd_count == *fd_size) {
        *fd_size *= 2; // Double it

        *pfds = realloc(*pfds, sizeof(**pfds) * (*fd_size));
        if(pfds == NULL){
            return -1;
        }
    }

    (*pfds)[*fd_count].fd = newfd;
    (*pfds)[*fd_count].events = POLLIN; // Check ready-to-read

    (*fd_count)++;

    return 0;
}

void del_from_pfds(struct pollfd pfds[], int i, int *fd_count)
{
    // Copy the one from the end over this one
    pfds[i] = pfds[*fd_count-1];

    (*fd_count)--;
}

