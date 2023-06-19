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

#include "tcp_proxy.h"

volatile sig_atomic_t shutdown_flag = 1;

static void sigterm_handler(int signo){
    printf("Got the signal!");
    shutdown_flag = 0;
}

int main(void) {


    const int BACKLOG = 10;
    const int MAX_BUF_SIZE = 100;
    const char HOST_ADDR[] = "127.0.0.1";
    const char PORT[] = "3333";

    puts("echo server: staring tcp server....");

    int listener_fd = create_listener(HOST_ADDR, PORT, BACKLOG);
    if(listener_fd == -1){
        fprintf(stderr, "echo server error: %s\n", strerror(errno));
        return EXIT_FAILURE;
    }

    printf("%s %s:%s...\n", "echo server: listening on port: ", HOST_ADDR, PORT);

    struct sigaction sa;
    sa.sa_handler = sigterm_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART; // causes sig handler to not terminate.
    if(sigaction(SIGINT, &sa, NULL) == -1){
        fprintf(stderr, "sigaction\n");
        close(listener_fd);
        return EXIT_FAILURE;
    }

    socklen_t addr_size;
    struct sockaddr_storage their_addr;
    addr_size = sizeof(their_addr);
    int new_fd;

    new_fd = accept(listener_fd, (struct sockaddr *)&their_addr, &addr_size);
    if( new_fd == -1) {
        fprintf(stderr, "echo server error: %s\n", strerror(errno));
        return EXIT_FAILURE;
    }

    // Get name of connected client
    char conn_addr_res[INET_ADDRSTRLEN];
    void *conn_addr = &((((struct sockaddr_in *)&their_addr)->sin_addr)); // gross
    inet_ntop(their_addr.ss_family, conn_addr, conn_addr_res, sizeof(conn_addr_res));
    printf("echo server: connection from: %s...\n", conn_addr_res);

    // place main server loop here.

    // todo: refactor to handle multiple clients
    while(shutdown_flag == 1){ // echo until shutdown
        // wait for input from client
        char buffer[MAX_BUF_SIZE];
        int bytes_read = recv(new_fd, buffer, sizeof(buffer) - 1, 0);
        if (bytes_read == -1){
            fprintf(stderr, "echo server error: %s\n", strerror(errno));
            close(listener_fd);
            close(new_fd);
            return EXIT_FAILURE;
        }
        if(bytes_read == 0){
            printf("echo server: client %s closed connection.\n", conn_addr_res);
            break;
        }

        buffer[bytes_read] = '\0'; // ensure end of array character
        printf("echo server: received %d of %d bytes\n", bytes_read, MAX_BUF_SIZE);

        // send target response back to client
        int bytes_sent_client;
        bytes_sent_client = send(new_fd, buffer, bytes_read, 0);
        printf("echo server: sent %d / %d bytes to connection at %s\n", bytes_sent_client, bytes_read, conn_addr_res);

        if(bytes_sent_client == -1){
            fprintf(stderr, "echo server error: %s\n", strerror(errno));
            close(listener_fd);
            close(new_fd);
            return EXIT_FAILURE;
        }
    }


    puts("server: shutting down....\n");
    close(listener_fd);
    close(new_fd);
    return EXIT_SUCCESS;
}
