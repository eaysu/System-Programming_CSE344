#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

#define MAX_AUTOMOBILE_SPOTS 8
#define MAX_PICKUP_SPOTS 4
#define TEMPORARY_LOT 0

// Shared counters for parking spots
int mFree_automobile = MAX_AUTOMOBILE_SPOTS;
int mFree_pickup = MAX_PICKUP_SPOTS;
int temporary_lot = TEMPORARY_LOT;

// Semaphores for synchronization
sem_t newAutomobile;
sem_t newPickup;
sem_t inChargeforAutomobile;
sem_t inChargeforPickup;

// Mutexes to protect the shared counters
pthread_mutex_t automobile_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t pickup_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t temporary_lot_mutex = PTHREAD_MUTEX_INITIALIZER;

void* carOwner(void* arg) {
    int vehicle_type = *((int*)arg); 
    free(arg);

    if (vehicle_type == 1) {
        sem_post(&newAutomobile);
    } else {
        sem_post(&newPickup);
    }

    pthread_mutex_lock(&temporary_lot_mutex);
    if (temporary_lot == 0) {
        temporary_lot = 1;
        pthread_mutex_unlock(&temporary_lot_mutex);

        if (vehicle_type == 1) {
            printf("** Automobile owner brings its vehicle in temporary lot.\n");
            sem_wait(&inChargeforAutomobile);
        } else {
            printf("** Pickup owner brings its vehicle in temporary lot.\n");
            sem_wait(&inChargeforPickup);
        }
        pthread_detach(pthread_self());
    } else {
        pthread_mutex_unlock(&temporary_lot_mutex);
        if (vehicle_type == 1) {
            printf("!! Automobile owner left due to no available spots.\n");
        } else {
            printf("!! Pickup owner left due to no available spots.\n");
        }
        pthread_detach(pthread_self());
    }
    return(NULL);
}


void* carStayHandler(void* arg) {
    int vehicle_type = *((int*)arg);
    free(arg);

    // every vehicle has same park duration
    sleep(10);

    if (vehicle_type == 1) {
        pthread_mutex_lock(&automobile_mutex);
        mFree_automobile++;
        printf("-- Automobile stay over. Attendant retreives. Available spots: %d\n", mFree_automobile);
        pthread_mutex_unlock(&automobile_mutex);
    } else {
        pthread_mutex_lock(&pickup_mutex);
        mFree_pickup++;
        printf("-- Pickup stay over. Attendant retreives. Available spots: %d\n", mFree_pickup);
        pthread_mutex_unlock(&pickup_mutex);
    }

    return NULL;
}

void* carAttendant(void* arg) {
    int vehicle_type = *((int*)arg); // 1 for automobile, 2 for pickup
    free(arg);

    while (1) {

        if (vehicle_type == 1) {
            sem_wait(&newAutomobile);
            pthread_mutex_lock(&automobile_mutex);
            pthread_mutex_lock(&temporary_lot_mutex);
            if (mFree_automobile > 0 && temporary_lot == 1) {
                temporary_lot = 0;
                mFree_automobile--;
                printf("++ Automobile attendant parked the car. Available spots: %d\n", mFree_automobile);
                sem_post(&inChargeforAutomobile);

                // Create a new thread to handle the vehicle's stay duration
                int* type = malloc(sizeof(int));
                *type = 1;
                pthread_t stay_thread;
                pthread_create(&stay_thread, NULL, carStayHandler, type);
                pthread_detach(stay_thread);
            }
            pthread_mutex_unlock(&temporary_lot_mutex);
            pthread_mutex_unlock(&automobile_mutex);

        } else {
            sem_wait(&newPickup);
            pthread_mutex_lock(&pickup_mutex);
            pthread_mutex_lock(&temporary_lot_mutex);  
            if (mFree_pickup > 0 && temporary_lot == 1) {
                temporary_lot = 0;
                mFree_pickup--;
                printf("++ Pickup attendant parked the car. Available spots: %d\n", mFree_pickup);
                sem_post(&inChargeforPickup);

                // Create a new thread to handle the vehicle's stay
                int* type = malloc(sizeof(int));
                *type = 2;
                pthread_t stay_thread;
                pthread_create(&stay_thread, NULL, carStayHandler, type);
                pthread_detach(stay_thread);
            }
            pthread_mutex_unlock(&temporary_lot_mutex);
            pthread_mutex_unlock(&pickup_mutex);
        }

    }

    return NULL;
}

int main() {
    srand(time(NULL));
    pthread_t owners[12];
    pthread_t attendants[2];

    // Initialize semaphores
    sem_init(&newAutomobile, 0, 0);
    sem_init(&newPickup, 0, 0);
    sem_init(&inChargeforAutomobile, 0, 0);
    sem_init(&inChargeforPickup, 0, 0);

    // Create car attendant threads
    for (int i = 0; i < 2; i++) {
        int* type = malloc(sizeof(int));
        *type = (i == 0) ? 1 : 2;
        pthread_create(&attendants[i], NULL, carAttendant, type);
    }

    // Simulate car owners arriving
    for (int i = 0; i < 12; i++) {
        int* type = malloc(sizeof(int));
        *type = (rand() % 2) + 1; // Randomly assign vehicle type (1 or 2)
        pthread_create(&owners[i], NULL, carOwner, type);
        sleep(1); // Simulate time between arrivals
    }

    int i = 0;
    while (1) {
        if (mFree_automobile == MAX_AUTOMOBILE_SPOTS && mFree_pickup == MAX_PICKUP_SPOTS) {
            if (i == 2) {
                break;
            }
            pthread_detach(attendants[i]);
            i++; 
        }
    }

    // Clean up semaphores
    sem_destroy(&newAutomobile);
    sem_destroy(&newPickup);
    sem_destroy(&inChargeforAutomobile);
    sem_destroy(&inChargeforPickup);

    // Clean up mutexes and condition variable
    pthread_mutex_destroy(&automobile_mutex);
    pthread_mutex_destroy(&pickup_mutex);
    pthread_mutex_destroy(&temporary_lot_mutex);

    return 0;
}
