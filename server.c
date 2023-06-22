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
#include <signal.h>
#include <threads.h>
#include <sys/epoll.h>

#include "tcp_proxy.h"

#define BACKLOG 10
#define MAX_BUF_SIZE 256
#define TARGET_IP "127.0.0.1"
#define TARGET_PORT "8080"
#define SERVER_IP "127.0.0.1"
#define SERVER_PORT "9000"
#define MAX_CONN 10
#define MAX_EVENTS 10



typedef struct connection {
    int client_fd;
    int target_fd;
} connection;

void handle_read_event(int conn_fd, connection *conns, int conn_count);

int main(void) {

    puts("server: staring tcp server....");

    int listener_fd = create_listener(SERVER_IP, SERVER_PORT, BACKLOG);
    if(listener_fd == -1){
        fprintf(stderr, "server: %s\n", strerror(errno));
        return EXIT_FAILURE;
    }

    printf("server: listening on port: %s:%s\n", SERVER_IP, SERVER_PORT);


    // get the epoll party started
    struct epoll_event ev;
    struct epoll_event events[MAX_EVENTS];

    int epollfd = epoll_create1(0);
    if(epollfd == -1) {
        perror("epoll_create1");
        return EXIT_FAILURE;
    }

    //ev.events = EPOLLIN | EPOLLEXCLUSIVE;
    ev.events = EPOLLIN;
    ev.data.fd = listener_fd;
    
    if(epoll_ctl(epollfd, EPOLL_CTL_ADD, listener_fd, &ev) == -1) {
        perror("epoll_ctl");
        return EXIT_FAILURE;
    }

    int client_fd;
    int target_fd;
    struct sockaddr_storage client_addr;
    socklen_t addr_len = sizeof(client_addr);

    connection conns[MAX_CONN];
    int conn_count = 0;

    for(;;) { // main event loop

        int nfds = epoll_wait(epollfd, events, MAX_EVENTS, -1);
        if(nfds == -1) {
            perror("epoll_wait");
            return EXIT_FAILURE;
        }
        // loop throw events
        for(int n = 0; n < nfds; ++n) {
            if(events[n].data.fd == listener_fd){

                // handle new connection
                client_fd = accept(listener_fd, (struct sockaddr *)&client_addr, &addr_len);
                if (client_fd == -1) {
                    perror("accept");
                    return EXIT_FAILURE;
                }

               // add client to epoll 
                ev.events = EPOLLIN;
                ev.data.fd = client_fd;
                if (epoll_ctl(epollfd, EPOLL_CTL_ADD, client_fd, &ev) == -1) {
                    perror("epoll_ctl");
                    return EXIT_FAILURE;
                }

                // add target to epoll
                target_fd = create_target_conn(TARGET_IP, TARGET_PORT);
                if(target_fd == -1){
                    fprintf(stderr, "server: %s\n", strerror(errno));
                    return EXIT_FAILURE;
                }
                // add target to epoll 
                ev.events = EPOLLIN;
                ev.data.fd = target_fd;
                if (epoll_ctl(epollfd, EPOLL_CTL_ADD, target_fd, &ev) == -1) {
                    perror("epoll_ctl");
                    return EXIT_FAILURE;
                }

                // save target and client pair to data structure
                connection conn;
                conn.client_fd = client_fd;
                conn.target_fd = target_fd;
                conns[conn_count] = conn;
                conn_count++;

                puts("server: new connection established....");
            } else {
                // need to read and write 
                // potential solution:
                // if client_fd, forward to target
                // if target_fd, lookup client_fd
                
                handle_read_event(events[n].data.fd, conns, conn_count);
            }
        }
    }
}

void handle_read_event(int conn_fd, connection *conns, int conn_count) {

    int send_fd = -1;
    // find connection pair. really should be hash map. or maybe use the void ptr on the event struct
    for(int i = 0; i < conn_count; ++i){
        if(conns[i].client_fd == conn_fd){
            send_fd = conns[i].target_fd;
        } 
        else if (conns[i].target_fd == conn_fd) {
            send_fd = conns[i].client_fd;
        }
    }

    if(send_fd == -1){
        close(conn_fd);
        fprintf(stderr, "could not find connection pair.\n");
        return; // exit for now
    }
    

    char buffer[MAX_BUF_SIZE];
    int bytes_read = recv(conn_fd, buffer, sizeof(buffer), 0);
    if(bytes_read == -1){
        perror("recv");
        close(conn_fd);
        close(send_fd);
        // remove from list
        return;
    }
    else if (bytes_read == 0){
        // conn closed
        // this also might be a bug. Do we send null character on to send_fd? 
        close(conn_fd);
        close(send_fd);
        // remove from list
        puts("server: client closed connection");
        return;
    }

    printf("server: received %d / %ld bytes.\n", bytes_read, sizeof(buffer));

    int bytes_sent = send(send_fd, buffer, bytes_read, 0);
    if(bytes_sent == -1) {
        perror("send");
        close(conn_fd);
        close(send_fd);
        // remove from list
    }

    printf("server: sent %d / %d bytes.\n", bytes_sent, bytes_read);
}
