#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <signal.h>

#define BUFFER_SIZE 1024

int port;
char server_ip[16];
int town_size_x;
int town_size_y;

void* client_thread(void* arg) {
    int client_id = *(int*)arg;
    int client_socket;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];
    int bytes_read;

    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket < 0) {
        perror("socket");
        return NULL;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
        perror("inet_pton");
        close(client_socket);
        return NULL;
    }

    if (connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect");
        close(client_socket);
        return NULL;
    }

    printf("Client %d connected to PideShop at %s:%d\n", client_id, server_ip, port);

    int client_x = rand() % town_size_x;
    int client_y = rand() % town_size_y;
    char message[BUFFER_SIZE];
    snprintf(message, sizeof(message), "%d %d %d", client_id, client_x, client_y);
    write(client_socket, message, strlen(message));

    bytes_read = read(client_socket, buffer, sizeof(buffer) - 1);
    if (bytes_read < 0) {
        perror("read");
        close(client_socket);
        return NULL;
    }

    buffer[bytes_read] = '\0';
    printf("Client %d received message from server: %s\n", client_id, buffer);

    bytes_read = read(client_socket, buffer, sizeof(buffer) - 1);
    if (bytes_read < 0) {
        perror("read");
        close(client_socket);
        return NULL;
    }

    buffer[bytes_read] = '\0';
    printf("Client %d received delivery message from server: %s\n", client_id, buffer);

    close(client_socket);
    return NULL;
}

void sigint_handler(int signum) {
    printf("\nSome orders are cancelled.\n");
    printf("\nCtrl+C pressed. Notifying server and exiting.\n");
    // Here you can add code to notify the server if needed.
    // For simplicity, let's exit directly after notifying.

    exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[]) {
    if (argc != 5) {
        fprintf(stderr, "Usage: %s [portnumber] [numberOfClients] [p] [q]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    port = atoi(argv[1]);
    int number_of_clients = atoi(argv[2]);
    town_size_x = atoi(argv[3]);
    town_size_y = atoi(argv[4]);

    strcpy(server_ip, "127.0.0.1");

    // Register signal handler for SIGINT (Ctrl+C)
    // Set up signal handling
    struct sigaction sa;
    sa.sa_handler = sigint_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }

    pthread_t threads[number_of_clients];
    int client_ids[number_of_clients];

    for (int i = 0; i < number_of_clients; i++) {
        client_ids[i] = i + 1;
        if (pthread_create(&threads[i], NULL, client_thread, &client_ids[i]) != 0) {
            perror("pthread_create");
            exit(EXIT_FAILURE);
        }
    }

    for (int i = 0; i < number_of_clients; i++) {
        pthread_join(threads[i], NULL);
    }

    printf("All clients served.\n");
    return 0;
}
