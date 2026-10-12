#include "kernel/types.h"
#include "user/user.h"

#define NTHREADS 3
#define ITERS 100000
#define PGSIZE 4096
#define MUTEX 0

volatile int counter = 0;
volatile int ready = 0;
int usesem;

void
worker(void *arg)
{
  __sync_fetch_and_add(&ready, 1);
  while (ready < NTHREADS)
    ;
  for (int i = 0; i < ITERS; i++) {
    if (usesem)
      sem_wait(MUTEX);
    counter = counter + 1;
    if (usesem)
      sem_signal(MUTEX);
  }
  exit(0);
}

void *
pagestack(void)
{
  char *m = malloc(2 * PGSIZE);
  return (void *)(((uint64)m + PGSIZE - 1) & ~(uint64)(PGSIZE - 1));
}

int
run(int sem)
{
  int t0;

  usesem = sem;
  counter = 0;
  ready = 0;
  t0 = uptime();
  for (int i = 0; i < NTHREADS; i++)
    clone(worker, 0, pagestack());
  for (int i = 0; i < NTHREADS; i++)
    join();
  printf("%s final counter = %d / %d  lost = %d  ticks = %d\n",
         sem ? "[with semaphore]   " : "[no synchronization]", counter,
         NTHREADS * ITERS, NTHREADS * ITERS - counter, uptime() - t0);
  return counter;
}

int
main(void)
{
  int before, after;

  sem_init(MUTEX, 1);
  printf("synctest: %d threads x %d increments\n", NTHREADS, ITERS);
  before = run(0);
  after = run(1);
  printf("before: %d   after: %d   %s\n", before, after,
         after == NTHREADS * ITERS ? "PASS: counter exact" : "FAIL");
  exit(0);
}
