#define _XOPEN_SOURCE 700
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <time.h>
#include <string.h>
#include <sys/types.h>

#define MAX_PARENTS 2
#define MAX_CHILDREN 20

#ifndef MAP_ANON
#define MAP_ANON 0x1000
#endif
#ifndef MAP_ANONYMOUS
#define MAP_ANONYMOUS MAP_ANON
#endif

typedef struct {
    int BankAccount;
    sem_t mutex;
} SharedData;

void dear_old_dad(SharedData *shared) {
    while (1) {
        sleep(rand() % 6);
        printf("Dear Old Dad: Attempting to Check Balance\n");
        int r = rand();
        sem_wait(&shared->mutex);
        int localBalance = shared->BankAccount;
        if (r % 2 == 0) {
            if (localBalance < 100) {
                int amount = rand() % 101;
                if (amount % 2 == 0) {
                    localBalance += amount;
                    printf("Dear old Dad: Deposits $%d / Balance = $%d\n", amount, localBalance);
                    shared->BankAccount = localBalance;
                } else {
                    printf("Dear old Dad: Doesn't have any money to give\n");
                }
            } else {
                printf("Dear old Dad: Thinks Student has enough Cash ($%d)\n", localBalance);
            }
        } else {
            printf("Dear Old Dad: Last Checking Balance = $%d\n", localBalance);
        }
        sem_post(&shared->mutex);
    }
}

void lovable_mom(SharedData *shared) {
    while (1) {
        sleep(rand() % 11);
        printf("Lovable Mom: Attempting to Check Balance\n");
        sem_wait(&shared->mutex);
        int localBalance = shared->BankAccount;
        if (localBalance <= 100) {
            int amount = (rand() % 126); // 0-125
            localBalance += amount;
            printf("Lovable Mom: Deposits $%d / Balance = $%d\n", amount, localBalance);
            shared->BankAccount = localBalance;
        } else {
            printf("Lovable Mom: Thinks Student has enough Cash ($%d)\n", localBalance);
        }
        sem_post(&shared->mutex);
    }
}

void poor_student(SharedData *shared, int id) {
    char buf[128];
    while (1) {
        sleep(rand() % 6);
        snprintf(buf, sizeof(buf), "Poor Student(%d): Attempting to Check Balance\n", id);
        write(1, buf, strlen(buf));
        int r = rand();
        sem_wait(&shared->mutex);
        int localBalance = shared->BankAccount;
        if (r % 2 == 0) {
            int need = rand() % 51; // 0-50
            snprintf(buf, sizeof(buf), "Poor Student(%d) needs $%d\n", id, need);
            write(1, buf, strlen(buf));
            if (need <= localBalance) {
                localBalance -= need;
                snprintf(buf, sizeof(buf), "Poor Student(%d): Withdraws $%d / Balance = $%d\n", id, need, localBalance);
                shared->BankAccount = localBalance;
            } else {
                snprintf(buf, sizeof(buf), "Poor Student(%d): Not Enough Cash ($%d)\n", id, localBalance);
            }
            write(1, buf, strlen(buf));
        } else {
            snprintf(buf, sizeof(buf), "Poor Student(%d): Last Checking Balance = $%d\n", id, localBalance);
            write(1, buf, strlen(buf));
        }
        sem_post(&shared->mutex);
    }
}

int main(int argc, char *argv[]) {
    int num_parents = 1, num_children = 1;
    if (argc >= 3) {
        num_parents = atoi(argv[1]);
        num_children = atoi(argv[2]);
        if (num_parents < 1) num_parents = 1;
        if (num_parents > 2) num_parents = 2;
        if (num_children < 1) num_children = 1;
        if (num_children > MAX_CHILDREN) num_children = MAX_CHILDREN;
    }
    srand(time(NULL));
    SharedData *shared = mmap(NULL, sizeof(SharedData), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (shared == MAP_FAILED) { perror("mmap"); exit(1); }
    shared->BankAccount = 0;
    sem_init(&shared->mutex, 1, 1);

    pid_t pids[MAX_PARENTS + MAX_CHILDREN];
    int idx = 0;
    // Parents
    for (int i = 0; i < num_parents; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            srand(time(NULL) ^ getpid());
            if (i == 0) dear_old_dad(shared);
            else lovable_mom(shared);
            exit(0);
        } else if (pid > 0) {
            pids[idx++] = pid;
        } else {
            perror("fork");
        }
    }
    // Children
    for (int i = 0; i < num_children; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            srand(time(NULL) ^ getpid());
            poor_student(shared, i+1);
            exit(0);
        } else if (pid > 0) {
            pids[idx++] = pid;
        } else {
            perror("fork");
        }
    }
    // Wait for all children (never reached, infinite loop)
    for (int i = 0; i < idx; i++) {
        waitpid(pids[i], NULL, 0);
    }
    sem_destroy(&shared->mutex);
    munmap(shared, sizeof(SharedData));
    return 0;
}
