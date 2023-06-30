// tcp_proxy.c
#include "tcp_proxy.h"

int create_listener(const char *ip_addr, const char *port, const int backlog){

    int status;
    struct addrinfo hints;
    struct addrinfo *results;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    // Get socket info for net stuff
    status = getaddrinfo(ip_addr, port, &hints, &results);
    if(status != 0){
        freeaddrinfo(results);
        return -1;
    }

    int sockfd;
    struct addrinfo* rp;
    // need to iterate through  addrinfo linked list
    for (rp = results; rp != NULL; rp = rp->ai_next){
        sockfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if(sockfd == -1) continue;
        if (bind(sockfd, rp->ai_addr, rp->ai_addrlen) == 0) break; //bound!
        close(sockfd); // try again
    }

    if(rp == NULL) {
        freeaddrinfo(results);
        return -1;
    }

    // open socket for connections
    if(listen(sockfd, backlog) == -1){
        freeaddrinfo(results);
        return -1;
    }

    freeaddrinfo(results);
    return sockfd;
}


int create_target_conn(const char *ip_addr, const char *port) {
    int status;
    struct addrinfo hints;
    struct addrinfo *results;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    // Get socket info for net stuff
    status = getaddrinfo(ip_addr, port, &hints, &results);
    if(status != 0){
        return -1;
    }

    int sockfd;
    struct addrinfo* rp;
    // need to iterate through  addrinfo linked list
    for (rp = results; rp != NULL; rp = rp->ai_next){
        sockfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if(sockfd == -1) continue;
        if (connect(sockfd, rp->ai_addr, rp->ai_addrlen) != -1) break; // connected!
        close(sockfd); // try again
    }

    if(rp == NULL) {
        freeaddrinfo(results);
        return -1;
    }

    freeaddrinfo(results);
    return sockfd;
}

void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}


int handle_read_event(int conn_fd, connection *conns, int *conn_count, int epollfd, const int MAX_BUF_SIZE) {

    int send_fd = -1;
    bool is_client = false; // need this to remove from my bad data structure
    // find connection pair. really should be hash map. or maybe use the void ptr on the event struct
    for(int i = 0; i < *conn_count; ++i){
        if(conns[i].client_fd == conn_fd){
            is_client = true;
            send_fd = conns[i].target_fd;
        } 
        else if (conns[i].target_fd == conn_fd) {
            send_fd = conns[i].client_fd;
        }
    }

    if(send_fd == -1){
        epoll_ctl(epollfd, EPOLL_CTL_DEL, conn_fd, NULL);
        close(conn_fd);
        fprintf(stderr, "could not find connection pair.\n");
        return -1;
    }
    

    char buffer[MAX_BUF_SIZE];
    int bytes_read = recv(conn_fd, buffer, sizeof(buffer), 0);
    if(bytes_read == -1){
        // should I close the sender???
        perror("recv");
        epoll_ctl(epollfd, EPOLL_CTL_DEL, conn_fd, NULL);
        epoll_ctl(epollfd, EPOLL_CTL_DEL, send_fd, NULL);
        close(conn_fd);
        close(send_fd);
        // remove from list
        remove_conn(is_client, conn_fd, conns, conn_count);
        return -1;
    }
    else if (bytes_read == 0){
        // conn closed
        // this also might be a bug. Do we send null character on to send_fd? 
        epoll_ctl(epollfd, EPOLL_CTL_DEL, conn_fd, NULL);
        epoll_ctl(epollfd, EPOLL_CTL_DEL, send_fd, NULL);
        close(conn_fd);
        close(send_fd);
        // remove from list
        remove_conn(is_client, conn_fd, conns, conn_count);
        puts("server: client closed connection");
        return -1;
    }

    printf("server: received %d / %ld bytes.\n", bytes_read, sizeof(buffer));

    int bytes_sent = sendall(send_fd, buffer, bytes_read);
    if(bytes_sent == -1) {
        perror("send");
        epoll_ctl(epollfd, EPOLL_CTL_DEL, conn_fd, NULL);
        epoll_ctl(epollfd, EPOLL_CTL_DEL, send_fd, NULL);
        close(conn_fd);
        close(send_fd);
        // remove from list
        remove_conn(is_client, conn_fd, conns, conn_count);
        return -1;
    }

    printf("server: sent %d / %d bytes.\n", bytes_sent, bytes_read);
    return 1;
}

// should remove the connection pair
int remove_conn(bool is_client, int conn_fd, connection *conn, int *conn_count) {

    int i_remove = -1;
    int i_last = (*conn_count) - 1;
    if(is_client) {
        for(int i = 0; i < *conn_count; ++i) {
            if(conn[i].client_fd == conn_fd){
                i_remove = i;
                break;
            }
        }
        if(i_remove == -1){
           return -1; 
        }

        printf("server: removed client_fd %d and target_fd %d from list\n", conn_fd, conn[i_remove].target_fd);
        conn[i_remove] = conn[i_last];
        (*conn_count)--;
        return 1;
    } 
    else { // would be target
        for(int i = 0; i < *conn_count; ++i) {
            if(conn[i].target_fd == conn_fd){
                i_remove = i;
                break;
            }
        }
        if(i_remove == -1){
           return -1; 
        }

        printf("server: removed client_fd %d and target_fd %d from list\n", conn[i_remove].client_fd, conn_fd);
        conn[i_remove] = conn[i_last];
        (*conn_count)--;
        return 1;
    }
}


int sendall(int sockfd, char *buf, int len) {
    int total = 0;        // how many bytes we've sent
    int bytesleft = len; // how many we have left to send
    int n = -1;

    while(total < len) {
        n = send(sockfd, buf+total, bytesleft, 0);
        if (n == -1) { break; }
        total += n;
        bytesleft -= n;
    }

    return n == -1 ? -1 : total;
}

