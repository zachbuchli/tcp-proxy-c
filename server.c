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
#include <poll.h>

#include "tcp_proxy.h"
#include <threads.h>

volatile sig_atomic_t shutdown_signal = 1;

int main(void) {

    const int BACKLOG = 10;
    const int MAX_BUF_SIZE = 100;
    const char TARGET_ADDR[] = "127.0.0.1";
    const char TARGET_PORT[] = "3333";
    const char HOST_ADDR[] = "127.0.0.1";
    const char PORT[] = "4444";
    const size_t MAX_CONNECTIONS = 10;

    puts("server: staring tcp server....");

    int fd_count = 0;
    int fd_size = MAX_CONNECTIONS;
    struct pollfd *pfds = malloc(sizeof(struct pollfd*) * fd_size);

    int target_conn_fd = create_target_conn(TARGET_ADDR, TARGET_PORT);
    if(target_conn_fd == -1){
        fprintf(stderr, "server: %s\n", strerror(errno));
        free(pfds);
        pfds = NULL;
        return EXIT_FAILURE;
    }
    printf("server: connected to target %s:%s\n", TARGET_ADDR, TARGET_PORT);

    int listener_fd = create_listener(HOST_ADDR, PORT, BACKLOG);
    if(listener_fd == -1){
        fprintf(stderr, "server: %s\n", strerror(errno));
        free(pfds);
        pfds = NULL;
        return EXIT_FAILURE;
    }

    printf("%s %s:%s...\n", "server: listening on port: ", HOST_ADDR, PORT);

    pfds[0].fd = listener_fd;
    pfds[0].events = POLLIN;
    ++fd_count;

    pfds[1].fd = target_conn_fd;
    pfds[1].events = POLLIN;
    ++fd_count;

    for(;;) {
        int poll_count = poll(pfds, fd_count, -1);
        if(poll_count == -1){
            perror("poll");
        }

        for(int i = 0; i < fd_count; i++){

            if(pfds[i].revents & POLLIN) { // ready to read
                if(pfds[i].fd == listener_fd) { // new connection

                    socklen_t addr_size;
                    struct sockaddr_storage their_addr;
                    addr_size = sizeof(their_addr);
                    int new_fd;

                    new_fd = accept(listener_fd, (struct sockaddr *)&their_addr, &addr_size);
                    if( new_fd == -1) {
                        fprintf(stderr, "server: %s\n", strerror(errno));

                    }

                    add_to_pfds(&pfds, new_fd, &fd_count, &fd_size);

                    // Get name of connected client
                    char conn_addr_res[INET_ADDRSTRLEN];
                    void *conn_addr = &((((struct sockaddr_in *)&their_addr)->sin_addr)); // gross
                    inet_ntop(their_addr.ss_family, conn_addr, conn_addr_res, sizeof(conn_addr_res));
                    printf("server: connection from: %s on socket: %d\n", conn_addr_res, new_fd);

                } else { // client already connected

                    char buffer[MAX_BUF_SIZE];
                    int bytes_read = recv(pfds[i].fd, buffer, sizeof(buffer), 0);
                    if (bytes_read == -1){
                        fprintf(stderr, "server: %s\n", strerror(errno));
                    }

                    if(bytes_read <= 0){
                        if(bytes_read == 0){
                            printf("server: client closed connection.\n");
                        } else {
                            perror("recv");
                        }
                        close(pfds[i].fd);
                        del_from_pfds(pfds, i, &fd_count);
                        continue; // do I want to do this?
                    }

                    printf("server: received %d of %d bytes\n", bytes_read, MAX_BUF_SIZE);







                }
            }
        }
    }

    socklen_t addr_size;
    struct sockaddr_storage their_addr;
    addr_size = sizeof(their_addr);
    int new_fd;

    new_fd = accept(listener_fd, (struct sockaddr *)&their_addr, &addr_size);
    if( new_fd == -1) {
        fprintf(stderr, "server: %s\n", strerror(errno));
        return EXIT_FAILURE;
    }

    // Get name of connected client
    char conn_addr_res[INET_ADDRSTRLEN];
    void *conn_addr = &((((struct sockaddr_in *)&their_addr)->sin_addr)); // gross
    inet_ntop(their_addr.ss_family, conn_addr, conn_addr_res, sizeof(conn_addr_res));
    printf("server: connection from: %s...\n", conn_addr_res);


    // place main server loop here.
    while(shutdown_signal == 1){

        // wait for input from client
        char buffer[MAX_BUF_SIZE];
        int bytes_read = recv(new_fd, buffer, sizeof(buffer) - 1, 0);
        if (bytes_read == -1){
            fprintf(stderr, "server: %s\n", strerror(errno));
            close(listener_fd);
            close(new_fd);
            return EXIT_FAILURE;
        }

        if(bytes_read == 0){
            printf("server: client closed connection.\n");
            close(listener_fd);
            close(new_fd);
            return EXIT_SUCCESS;
        }

        buffer[bytes_read] = '\0'; // ensure end of array character
        printf("server: received %d of %d bytes\n", bytes_read, MAX_BUF_SIZE);

        // forward message to target
        int bytes_sent_target;
        bytes_sent_target = send(target_conn_fd, buffer, bytes_read, 0);
        printf("server: sent %d / %d bytes to connection at %s\n", bytes_sent_target, bytes_read, conn_addr_res);

        if(bytes_sent_target == -1){
            fprintf(stderr, "%s\n", strerror(errno));
            close(listener_fd);
            close(new_fd);
            close(target_conn_fd);
            return EXIT_FAILURE;
        }

        // wait for response from target
        char buffer_target[MAX_BUF_SIZE];
        int bytes_read_target = recv(target_conn_fd, buffer_target, sizeof(buffer) - 1, 0);
        if (bytes_read_target == -1){
            fprintf(stderr, "server: %s\n", strerror(errno));
            close(listener_fd);
            close(new_fd);
            close(target_conn_fd);
            return EXIT_FAILURE;
        }

        if(bytes_read_target == 0){
            printf("server: client closed connection.\n");
            close(listener_fd);
            close(new_fd);
            return EXIT_SUCCESS;
        }

        // send target response back to client
        int bytes_sent_client;
        bytes_sent_client = send(new_fd, buffer_target, bytes_read_target, 0);
        printf("server: sent %d / %d bytes to connection at %s\n", bytes_sent_client, bytes_read_target, conn_addr_res);

        if(bytes_sent_client == -1){
            fprintf(stderr, "%s\n", strerror(errno));
            close(listener_fd);
            close(new_fd);
            close(target_conn_fd);
            return EXIT_FAILURE;
        }
    }


    puts("server: shutting down....");
    close(listener_fd);
    close(new_fd);
    close(target_conn_fd);
    return EXIT_SUCCESS;
}
