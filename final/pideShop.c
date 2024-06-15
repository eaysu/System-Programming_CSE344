#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <signal.h>
#include <time.h>

#define BUFFER_SIZE 1024
#define MAX_ORDERS 100
#define OVEN_CAPACITY 6
#define DELIVERY_CAPACITY 3
#define OVEN_APPARATUS 3
#define OVEN_DOORS 2

typedef struct {
    int client_socket;
    int order_id;
    int client_x;
    int client_y;
    int town_size_x;
    int town_size_y;
    int is_prepared;
    int is_cooked;
    int is_delivered;
    int canceled;
} order_t;

order_t orders[MAX_ORDERS];
int cooked_orders_list[MAX_ORDERS];
int cooked_orders_count = 0;

pthread_mutex_t order_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t oven_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t oven_queue_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t delivery_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t oven_apparatus_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t oven_door_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t oven_cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t delivery_cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t oven_apparatus_cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t oven_door_cond = PTHREAD_COND_INITIALIZER;

pthread_mutex_t delivery_count_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t all_orders_done_cond = PTHREAD_COND_INITIALIZER;

int total_orders = 0;
int oven_queue[OVEN_CAPACITY];
int oven_queue_size = 0;
int oven_apparatus = 0;
int oven_door = 0;
int cooked_orders = 0;
int num_delivery_personnel;
int delivery_speed;
int total_delivered_orders = 0;
int cook_order_count[MAX_ORDERS] = {0}; // Assuming MAX_ORDERS as the max number of cooks
int delivery_order_count[MAX_ORDERS] = {0}; // Assuming MAX_ORDERS as the max number of delivery personnel
FILE *log_file;

void log_event(const char *event) {
    time_t now = time(NULL);
    char *timestamp = ctime(&now);
    timestamp[strlen(timestamp) - 1] = '\0'; // Remove newline character
    fprintf(log_file, "[%s] %s\n", timestamp, event);
    fflush(log_file);
}

void* check_all_orders_done(void* arg) {
    pthread_mutex_lock(&delivery_count_mutex);
    while (total_delivered_orders < total_orders) {
        pthread_cond_wait(&all_orders_done_cond, &delivery_count_mutex);
    }
    pthread_mutex_unlock(&delivery_count_mutex);

    // Identify the most hardworking cook and delivery person
    int max_cook_orders = 0;
    int max_cook_id = -1;
    for (int i = 0; i < MAX_ORDERS; i++) {
        if (cook_order_count[i] > max_cook_orders) {
            max_cook_orders = cook_order_count[i];
            max_cook_id = i + 1;
        }
    }

    int max_delivery_orders = 0;
    int max_delivery_id = -1;
    for (int i = 0; i < MAX_ORDERS; i++) {
        if (delivery_order_count[i] > max_delivery_orders) {
            max_delivery_orders = delivery_order_count[i];
            max_delivery_id = i + 1;
        }
    }

    if (max_cook_id != -1 && max_delivery_id != -1) {
        printf("Thanks Cook %d and Delivery %d\n", max_cook_id, max_delivery_id);
    }

    return NULL;
}

void* cook_thread(void* arg) {
    int cook_id = *(int*)arg;
    free(arg);
    
    while (1) {
        pthread_mutex_lock(&order_mutex);
        int order_to_prepare = -1;
        for (int i = 0; i < total_orders; i++) {
            if (!orders[i].is_prepared && !orders[i].canceled) {
                orders[i].is_prepared = 1;
                order_to_prepare = i;
                break;
            }
        }
        pthread_mutex_unlock(&order_mutex);

        if (order_to_prepare == -1) {
            sleep(1);
            continue;
        }

        printf("Cook %d preparing order %d...\n", cook_id, order_to_prepare + 1);
        int preparation_time = (rand() % 3) + 2;
        sleep(preparation_time);

        pthread_mutex_lock(&order_mutex);
        if (orders[order_to_prepare].canceled) {
            printf("Order %d was canceled, discarding...\n", order_to_prepare);
            pthread_mutex_unlock(&order_mutex);
            continue;
        }
        pthread_mutex_unlock(&order_mutex);
        log_event("Order prepared");

        pthread_mutex_lock(&oven_door_mutex);
        while (oven_door >= OVEN_DOORS) {
            printf("Oven doors are used\n");
            pthread_cond_wait(&oven_door_cond, &oven_door_mutex);
        }
        oven_door++;
        pthread_mutex_unlock(&oven_door_mutex);

        pthread_mutex_lock(&oven_apparatus_mutex);
        while (oven_apparatus >= OVEN_APPARATUS) {
            printf("No available oven apparatus\n");
            pthread_cond_wait(&oven_apparatus_cond, &oven_apparatus_mutex);
        }
        oven_apparatus++;
        pthread_mutex_unlock(&oven_apparatus_mutex);

        pthread_mutex_lock(&oven_mutex);
        while (oven_queue_size >= OVEN_CAPACITY) {
            printf("There is no space for new order in oven\n");
            pthread_cond_wait(&oven_cond, &oven_mutex);
        }
        oven_queue[oven_queue_size++] = order_to_prepare;
        pthread_cond_signal(&oven_cond);
        pthread_mutex_unlock(&oven_mutex);

        pthread_mutex_lock(&oven_apparatus_mutex);
        oven_apparatus--;
        pthread_cond_signal(&oven_apparatus_cond);
        pthread_mutex_unlock(&oven_apparatus_mutex);

        pthread_mutex_lock(&oven_door_mutex);
        oven_door--;
        pthread_cond_signal(&oven_door_cond);
        pthread_mutex_unlock(&oven_door_mutex);

        printf("Cook %d placed order %d in oven...\n", cook_id, order_to_prepare + 1);
        log_event("Order placed in oven");

        // Track the number of orders each cook handles
        cook_order_count[cook_id - 1]++;
    }
    return NULL;
}

void* oven_thread(void* arg) {
    while (1) {
        pthread_mutex_lock(&oven_queue_mutex);
        while (oven_queue_size == 0) {
            pthread_cond_wait(&oven_cond, &oven_queue_mutex);
        }

        printf("Oven is cooking orders...\n");
        sleep(5); // Simulate oven cooking time

        pthread_mutex_lock(&order_mutex);
        for (int i = 0; i < oven_queue_size; i++) {
            int order_id = oven_queue[i];
            if (!orders[order_id].canceled) {
                orders[order_id].is_cooked = 1;
                cooked_orders_list[cooked_orders_count++] = order_id;
                cooked_orders++;
                printf("Order %d cooked\n", order_id + 1);
                log_event("Orders cooked");
            } else {
                printf("Order %d was canceled in the oven, discarding...\n", order_id);
            }
        }
        oven_queue_size = 0;
        pthread_cond_signal(&delivery_cond);
        pthread_mutex_unlock(&order_mutex);

        pthread_mutex_unlock(&oven_queue_mutex);
    }
    return NULL;
}

int calculate_manhattan_distance(int x1, int y1, int x2, int y2) {
    return abs(x1 - x2) + abs(y1 - y2);
}

void* delivery_thread(void* arg) {
    int id = *(int*)arg;
    free(arg);
    
    while (1) {
        pthread_mutex_lock(&delivery_mutex);
        while (cooked_orders == 0) {
            pthread_cond_wait(&delivery_cond, &delivery_mutex);
        }

        int orders_to_deliver = 0;
        int order_ids[DELIVERY_CAPACITY] = {0};
        pthread_mutex_lock(&order_mutex);
        for (int i = 0; i < cooked_orders_count && orders_to_deliver < DELIVERY_CAPACITY; i++) {
            int order_id = cooked_orders_list[i];
            if (orders[order_id].is_cooked && !orders[order_id].is_delivered) {
                order_ids[orders_to_deliver] = order_id;
                orders_to_deliver++;
            }
        }

        if (orders_to_deliver > 0) {
            cooked_orders -= orders_to_deliver;
        }
        pthread_mutex_unlock(&order_mutex);
        pthread_mutex_unlock(&delivery_mutex);

        if (orders_to_deliver > 0) {
            printf("Delivery person %d is delivering %d orders... order ids:", id, orders_to_deliver);
            for (int i = 0; i < orders_to_deliver; i++) {
                printf(" %d", order_ids[i] + 1);
            }
            printf("\n");

            pthread_mutex_lock(&order_mutex);
            int total_time = 0;
            for (int i = 0; i < orders_to_deliver; i++) {
                int order_id = order_ids[i];
                if (orders[order_id].is_cooked && !orders[order_id].is_delivered) {
                    int shop_x = orders[i].town_size_x / 2;
                    int shop_y = orders[i].town_size_y / 2;
                    int distance = calculate_manhattan_distance(shop_x, shop_y, orders[i].client_x, orders[i].client_y);
                    int new_delivery_time = distance / delivery_speed;
                    total_time += new_delivery_time;
                } 
            }       

            for (int i = 0; i < orders_to_deliver; i++) {
                int order_id = order_ids[i];
                if (orders[order_id].is_cooked && !orders[order_id].is_delivered) {
                    sleep(total_time);
                    orders[order_id].is_delivered = 1;
                    char delivery_message[BUFFER_SIZE];
                    snprintf(delivery_message, sizeof(delivery_message), "Order %d delivered in %d seconds", orders[order_id].order_id, total_time);
                    write(orders[order_id].client_socket, delivery_message, strlen(delivery_message));
                    close(orders[order_id].client_socket);
                    log_event("Order delivered");

                    // Track the number of deliveries by each delivery person
                    delivery_order_count[id - 1]++;

                    // Update total delivered orders count
                    pthread_mutex_lock(&delivery_count_mutex);
                    total_delivered_orders++;
                    if (total_delivered_orders == total_orders) {
                        printf("Done serving client\n");
                        printf("Waiting for another orders...\n");
                        pthread_cond_signal(&all_orders_done_cond);
                    }
                    pthread_mutex_unlock(&delivery_count_mutex);
                }
            }
            pthread_mutex_unlock(&order_mutex);
        }
    }
    return NULL;
}

void* handle_client_order(void* arg) {
    int client_socket = *(int*)arg;
    char buffer[BUFFER_SIZE];
    int bytes_read;

    bytes_read = read(client_socket, buffer, sizeof(buffer) - 1);
    if (bytes_read < 0) {
        perror("read");
        close(client_socket);
        free(arg);
        return NULL;
    }

    buffer[bytes_read] = '\0';
    printf("Received order from client: %s\n", buffer);
    log_event("Order received");

    pthread_mutex_lock(&order_mutex);
    int order_id = total_orders++;
    orders[order_id].client_socket = client_socket;
    orders[order_id].order_id = order_id;
    sscanf(buffer, "%d %d %d %d %d", &orders[order_id].order_id, &orders[order_id].client_x, &orders[order_id].client_y, &orders[order_id].town_size_x, &orders[order_id].town_size_y);
    orders[order_id].is_prepared = 0;
    orders[order_id].is_cooked = 0;
    orders[order_id].is_delivered = 0;
    orders[order_id].canceled = 0;
    pthread_mutex_unlock(&order_mutex);

    char *response = "Order received and being processed!";
    write(client_socket, response, strlen(response));

    free(arg);
    return NULL;
}

void* handle_client(void* arg) {
    int server_socket = *(int*)arg;
    while (1) {
        struct sockaddr_in client_addr;
        socklen_t addr_size = sizeof(client_addr);
        int client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &addr_size);
        if (client_socket < 0) {
            perror("accept");
            continue;
        }

        printf("Connection accepted from %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        pthread_t tid;
        int *client_socket_ptr = malloc(sizeof(int));
        *client_socket_ptr = client_socket;

        pthread_create(&tid, NULL, handle_client_order, client_socket_ptr);
        pthread_detach(tid);
    }
    return NULL;
}

void handle_signal(int sig) {
    printf("Cancellation signal received, shutting down server...\n");

    pthread_mutex_lock(&order_mutex);
    for (int i = 0; i < total_orders; i++) {
        orders[i].canceled = 1;
    }
    pthread_mutex_unlock(&order_mutex);

    // Notify all conditions to recheck the orders
    pthread_cond_broadcast(&oven_cond);
    pthread_cond_broadcast(&delivery_cond);
    pthread_cond_broadcast(&oven_apparatus_cond);
    pthread_cond_broadcast(&oven_door_cond);

    fclose(log_file);
    exit(0);
}

int main(int argc, char *argv[]) {
    if (argc != 5) {
        fprintf(stderr, "Usage: %s [portnumber] [CookthreadPoolSize] [DeliveryPoolSize] [DeliverySpeed]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int port = atoi(argv[1]);
    int cook_thread_pool_size = atoi(argv[2]);
    num_delivery_personnel = atoi(argv[3]);
    delivery_speed = atoi(argv[4]);

    int server_socket;
    struct sockaddr_in server_addr;

    log_file = fopen("pide_shop.log", "a");
    if (!log_file) {
        perror("fopen");
        exit(EXIT_FAILURE);
    }

    // Set up signal handling
    struct sigaction sa;
    sa.sa_handler = handle_signal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    if (listen(server_socket, 5) < 0) {
        perror("listen");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    printf("Server is listening on port %d...\n", port);

    pthread_t client_handler_thread;
    pthread_create(&client_handler_thread, NULL, handle_client, &server_socket);
    pthread_detach(client_handler_thread);

    pthread_t cook_threads[cook_thread_pool_size];
    for (int i = 0; i < cook_thread_pool_size; i++) {
        int *cook_id = malloc(sizeof(int));
        *cook_id = i + 1;
        pthread_create(&cook_threads[i], NULL, cook_thread, cook_id);
        pthread_detach(cook_threads[i]);
    }

    pthread_t oven_tid;
    pthread_create(&oven_tid, NULL, oven_thread, NULL);
    pthread_detach(oven_tid);

    pthread_t delivery_threads[num_delivery_personnel];
    for (int i = 0; i < num_delivery_personnel; i++) {
        int *delivery_id = malloc(sizeof(int));
        *delivery_id = i + 1;
        pthread_create(&delivery_threads[i], NULL, delivery_thread, delivery_id);
        pthread_detach(delivery_threads[i]);
    }

    // Thread to check for all orders completion
    pthread_t all_orders_done_tid;
    pthread_create(&all_orders_done_tid, NULL, check_all_orders_done, NULL);
    pthread_detach(all_orders_done_tid);

    while (1) {
        sleep(1);
    }

    return 0;
}
