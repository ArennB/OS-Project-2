#include "BENSCHILLIBOWL.h"
#include <time.h>

/* ---------------- Helper Functions ---------------- */

bool IsEmpty(BENSCHILLIBOWL* bcb) {
    return bcb->current_size == 0;
}

bool IsFull(BENSCHILLIBOWL* bcb) {
    return bcb->current_size >= bcb->max_size;
}

/* ---------------- Menu ---------------- */

MenuItem PickRandomMenuItem() {
    char* menu[] = {
        "BensChilli", "BensHalfSmoke", "BensHotDog",
        "BensChilliCheeseFries", "BensShake", "BensHotCakes",
        "BensCake", "BensHamburger", "BensVeggieBurger",
        "BensOnionRings"
    };
    int menu_length = sizeof(menu) / sizeof(menu[0]);
    int index = rand() % menu_length;
    return menu[index];
}

/* ---------------- Open Restaurant ---------------- */

BENSCHILLIBOWL* OpenRestaurant(int max_size, int expected_num_orders) {
    printf("Restaurant is open!\n");

    BENSCHILLIBOWL* bcb = malloc(sizeof(BENSCHILLIBOWL));
    if (!bcb) {
        perror("Failed to allocate restaurant");
        exit(1);
    }

    bcb->orders = NULL;
    bcb->current_size = 0;
    bcb->max_size = max_size;
    bcb->next_order_number = 1;
    bcb->orders_handled = 0;
    bcb->expected_num_orders = expected_num_orders;

    pthread_mutex_init(&bcb->mutex, NULL);
    pthread_cond_init(&bcb->can_add_orders, NULL);
    pthread_cond_init(&bcb->can_get_orders, NULL);

    return bcb;
}

/* ---------------- Close Restaurant ---------------- */

void CloseRestaurant(BENSCHILLIBOWL* mcg) {
    pthread_mutex_lock(&mcg->mutex);

    if (mcg->orders_handled != mcg->expected_num_orders) {
        printf("Warning: Expected %d orders, but handled %d.\n",
               mcg->expected_num_orders, mcg->orders_handled);
    } else {
        printf("All %d orders were handled successfully!\n", mcg->orders_handled);
    }

    // free all remaining orders (shouldn't be any if logic correct)
    Order* curr = mcg->orders;
    while (curr) {
        Order* tmp = curr;
        curr = curr->next;
        free(tmp);
    }

    pthread_mutex_unlock(&mcg->mutex);
    pthread_mutex_destroy(&mcg->mutex);
    pthread_cond_destroy(&mcg->can_add_orders);
    pthread_cond_destroy(&mcg->can_get_orders);

    free(mcg);
    printf("Restaurant is closed!\n");
}

/* ---------------- Add Order ---------------- */

// AddOrder: Adds an order to the restaurant queue, blocking if full.
// Handles all locking, waiting, and signaling internally.
int AddOrder(BENSCHILLIBOWL* bcb, Order* order) {
    pthread_mutex_lock(&bcb->mutex);
    while (bcb->current_size >= bcb->max_size) {
        pthread_cond_wait(&bcb->can_add_orders, &bcb->mutex);
    }
    // Assign order number
    order->order_number = bcb->next_order_number++;
    // Add to end of linked list queue
    if (bcb->orders == NULL) {
        bcb->orders = order;
    } else {
        Order* temp = bcb->orders;
        while (temp->next != NULL) temp = temp->next;
        temp->next = order;
    }
    order->next = NULL;
    bcb->current_size++;
    printf("Customer #%d placed Order #%d: %s\n", order->customer_id, order->order_number, order->menu_item);
    pthread_cond_signal(&bcb->can_get_orders); // Wake up a cook
    pthread_mutex_unlock(&bcb->mutex);
    return order->order_number;
}

/* ---------------- Get Order ---------------- */

// GetOrder: Removes an order from the restaurant queue, blocking if empty.
// Handles all locking, waiting, and signaling internally.
Order* GetOrder(BENSCHILLIBOWL* bcb) {
    pthread_mutex_lock(&bcb->mutex);
    while (bcb->current_size == 0) {
        // If all orders handled, return NULL to stop cooks
        if (bcb->orders_handled >= bcb->expected_num_orders) {
            pthread_mutex_unlock(&bcb->mutex);
            return NULL;
        }
        pthread_cond_wait(&bcb->can_get_orders, &bcb->mutex);
    }
    // Remove from front of linked list queue
    Order* order = bcb->orders;
    bcb->orders = bcb->orders->next;
    bcb->current_size--;
    bcb->orders_handled++;
    pthread_cond_signal(&bcb->can_add_orders); // Wake up a customer
    pthread_mutex_unlock(&bcb->mutex);
    return order;
}