#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include "BENSCHILLIBOWL.h"

#define BENSCHILLIBOWL_SIZE 100
#define NUM_CUSTOMERS 90
#define NUM_COOKS 10
#define ORDERS_PER_CUSTOMER 3
#define EXPECTED_NUM_ORDERS (NUM_CUSTOMERS * ORDERS_PER_CUSTOMER)

// Global restaurant
BENSCHILLIBOWL *bcb;

// Helper function declarations
bool IsEmpty(BENSCHILLIBOWL* bcb);
bool IsFull(BENSCHILLIBOWL* bcb);

/* Customer thread */
void* BENSCHILLIBOWLCustomer(void* tid) {
    int customer_id = (int)(long) tid;

    for (int i = 0; i < ORDERS_PER_CUSTOMER; i++) {
        Order *order = malloc(sizeof(Order));
        if (!order) {
            perror("Failed to allocate order");
            pthread_exit(NULL);
        }

        // Pick random menu item
        order->menu_item = PickRandomMenuItem();
        order->customer_id = customer_id;
        order->next = NULL;


        AddOrder(bcb, order);

        usleep(1000); // simulate delay
    }

    return NULL;
}

/* Cook thread */
void* BENSCHILLIBOWLCook(void* tid) {
    int cook_id = (int)(long) tid;
    int orders_fulfilled = 0;

    while (1) {

        Order *order = GetOrder(bcb);
        if (!order) {
            printf("Cook #%d fulfilled %d orders\n", cook_id, orders_fulfilled);
            return NULL;
        }

         printf("Cook #%d is fulfilling order for Customer #%d: %s\n",
             cook_id, order->customer_id, order->menu_item);
         orders_fulfilled++;
         usleep(1000); // simulate cooking time
         free(order);
    }

    return NULL;
}

/* Main */
int main() {
    srand(time(NULL));

    bcb = OpenRestaurant(BENSCHILLIBOWL_SIZE, EXPECTED_NUM_ORDERS);

    pthread_t customers[NUM_CUSTOMERS];
    pthread_t cooks[NUM_COOKS];

    // Create threads
    for (long i = 0; i < NUM_CUSTOMERS; i++)
        pthread_create(&customers[i], NULL, BENSCHILLIBOWLCustomer, (void*)i);
    for (long i = 0; i < NUM_COOKS; i++)
        pthread_create(&cooks[i], NULL, BENSCHILLIBOWLCook, (void*)i);

    // Join threads
    for (int i = 0; i < NUM_CUSTOMERS; i++)
        pthread_join(customers[i], NULL);
    for (int i = 0; i < NUM_COOKS; i++)
        pthread_join(cooks[i], NULL);

    CloseRestaurant(bcb);

    return 0;
}
