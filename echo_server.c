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
#include <pthread.h>
#include <sys/epoll.h>

#include "tcp_proxy.h"

#define BACKLOG 10
#define MAX_BUF_SIZE 256
#define SERVER_IP "127.0.0.1"
#define SERVER_PORT "8080"
#define MAX_CONN 10
#define MAX_EVENTS 10


void handle_read_event_echo(int conn_fd);

static pthread_t thread_id;

int main(void) {

    puts("echo server: staring tcp server....");

    int listener_fd = create_listener(SERVER_IP, SERVER_PORT, BACKLOG);
    if(listener_fd == -1){
        fprintf(stderr, "server: %s\n", strerror(errno));
        return EXIT_FAILURE;
    }

    printf("echo server: listening on port: %s:%s\n", SERVER_IP, SERVER_PORT);


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
    struct sockaddr_storage client_addr;
    socklen_t addr_len = sizeof(client_addr);

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

                puts("echo server: new connection established");
            } else {
                // fd has something to read!
                handle_read_event_echo(events[n].data.fd);
            }
        }
    }
}

void handle_read_event_echo(int conn_fd) {

    
    char buffer[MAX_BUF_SIZE];
    int bytes_read = recv(conn_fd, buffer, sizeof(buffer), 0);
    if(bytes_read == -1){
        perror("recv");
        close(conn_fd);
        return;
    }
    else if (bytes_read == 0){
        // conn closed
        // this also might be a bug. Do we send null character on to send_fd? 
        puts("echo server: client closed connection");
        close(conn_fd);
        return;
    }

    printf("echo server: received %d / %ld bytes\n", bytes_read, sizeof(buffer));

    int bytes_sent = sendall(conn_fd, buffer, bytes_read);
    if(bytes_sent == -1){
        perror("sendall");
        close(conn_fd);
        return;
    }
    
    
    printf("echo server: sent %d / %d bytes\n", bytes_sent, bytes_read);
}

 