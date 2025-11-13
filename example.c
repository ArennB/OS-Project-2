#include <semaphore.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <time.h>

int main(int argc, char **argv)
{
  int fd, zero = 0, *BankAccount;
  sem_t *mutex;

  srand(time(NULL));

  // open a file and map it into memory to hold the shared BankAccount variable
  fd = open("bank.txt", O_RDWR | O_CREAT, S_IRWXU);
  write(fd, &zero, sizeof(int));
  BankAccount = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  close(fd);

  /* create, initialize semaphore */
  if ((mutex = sem_open("bank_semaphore", O_CREAT, 0644, 1)) == SEM_FAILED) {
    perror("semaphore initialization");
    exit(1);
  }

  printf("Parent initializes BankAccount = %d\n", *BankAccount);
  printf("Parent is about to fork child processes...\n");

  pid_t student_pid, mom_pid;

  // --- First fork: Poor Student ---
  if ((student_pid = fork()) == 0) {
    int localBalance, need, randomNum;
    while (1) {
      sleep(rand() % 6);
      printf("Poor Student: Attempting to Check Balance\n");

      sem_wait(mutex);
      localBalance = *BankAccount;
      randomNum = rand();

      if (randomNum % 2 == 0) {
        need = rand() % 50 + 1;
        printf("Poor Student needs $%d\n", need);

        if (need <= localBalance) {
          localBalance -= need;
          printf("Poor Student: Withdraws $%d / Balance = $%d\n", need, localBalance);
        } else {
          printf("Poor Student: Not Enough Cash ($%d)\n", localBalance);
        }
        *BankAccount = localBalance;
      } else {
        printf("Poor Student: Last Checking Balance = $%d\n", localBalance);
      }

      sem_post(mutex);
    }
    exit(0);
  }

  // --- Second fork: Lovable Mom ---
  if ((mom_pid = fork()) == 0) {
    int localBalance, amount;
    while (1) {
      sleep(rand() % 10);
      printf("Lovable Mom: Attempting to Check Balance\n");

      sem_wait(mutex);
      localBalance = *BankAccount;

      if (localBalance <= 100) {
        amount = rand() % 126; // random 0–125
        localBalance += amount;
        printf("Lovable Mom: Deposits $%d / Balance = $%d\n", amount, localBalance);
        *BankAccount = localBalance;
      } else {
        printf("Lovable Mom: Thinks Student has enough Cash ($%d)\n", localBalance);
      }

      sem_post(mutex);
    }
    exit(0);
  }

  // --- Back to parent: Dear Old Dad ---
  int localBalance, amount, randomNum;
  while (1) {
    sleep(rand() % 6);
    printf("Dear Old Dad: Attempting to Check Balance\n");

    sem_wait(mutex);
    localBalance = *BankAccount;
    randomNum = rand();

    if (randomNum % 2 == 0) {
      if (localBalance < 100) {
        amount = rand() % 100 + 1;
        if (amount % 2 == 0) {
          localBalance += amount;
          printf("Dear Old Dad: Deposits $%d / Balance = $%d\n", amount, localBalance);
          *BankAccount = localBalance;
        } else {
          printf("Dear Old Dad: Doesn't have any money to give\n");
        }
      } else {
        printf("Dear Old Dad: Thinks Student has enough Cash ($%d)\n", localBalance);
      }
    } else {
      printf("Dear Old Dad: Last Checking Balance = $%d\n", localBalance);
    }

    sem_post(mutex);
  }

  exit(0);
}
