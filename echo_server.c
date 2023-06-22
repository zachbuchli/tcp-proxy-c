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
#include <stdbool.h>

#include "tcp_proxy.h"

#define BACKLOG 10
#define MAX_BUF_SIZE 100
#define SERVER_IP "127.0.0.1"
#define SERVER_PORT "3333"

volatile sig_atomic_t shutdown_flag = 1;

static void sigterm_handler(int signo);
static void handle_connection(int client_fd, char *client_ip);

int main(void) {

    puts("echo server: staring tcp server....");

    // setup sig_term handler
    struct sigaction sa;
    sa.sa_handler = sigterm_handler;
    sigemptyset(&sa.sa_mask);
    //sa.sa_flags = SA_RESTART; // causes sig handler to not terminate.
    if(sigaction(SIGINT, &sa, NULL) == -1){
        perror("sigaction");
        return EXIT_FAILURE;
    }

    int listener_fd = create_listener(SERVER_IP, SERVER_PORT, BACKLOG);
    if(listener_fd == -1){
        fprintf(stderr, "echo server error: %s\n", strerror(errno));
        return EXIT_FAILURE;
    }

    printf("%s %s:%s...\n", "echo server: listening on port: ", SERVER_IP, SERVER_PORT);

    socklen_t addr_size;
    struct sockaddr_storage their_addr;
    addr_size = sizeof(their_addr);
    int new_fd;

    while(shutdown_flag) {

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

        handle_connection(new_fd, conn_addr_res);
    }

    puts("server: shutting down....\n");
    close(listener_fd);
    return EXIT_SUCCESS;
}


void handle_connection(int client_fd, char *client_ip){

    // wait for input from client
    // dont want to loop here because not implementing sessions
    char buffer[MAX_BUF_SIZE];
    int bytes_read = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
    if (bytes_read == -1){
        fprintf(stderr, "echo server error: %s\n", strerror(errno));
        close(client_fd);
        return;
    }
    if(bytes_read == 0){
        printf("echo server: client %s closed connection.\n", client_ip);
        return;
    }

    buffer[bytes_read] = '\0'; // ensure end of array character
    printf("echo server: received %d of %d bytes\n", bytes_read, MAX_BUF_SIZE);

    // send target response back to client
    int bytes_sent_client;
    bytes_sent_client = send(client_fd, buffer, bytes_read, 0);
    printf("echo server: sent %d / %d bytes to connection at %s\n", bytes_sent_client, bytes_read, client_ip);

    if(bytes_sent_client == -1){
        fprintf(stderr, "echo server error: %s\n", strerror(errno));
        close(client_fd);
        return;
    }

    close(client_fd);
}

void sigterm_handler(int signo) {

    printf("Signal Number: %d\n", signo);
    shutdown_flag = 0;
}
