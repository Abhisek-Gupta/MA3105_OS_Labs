#include "kernel/types.h"
#include "user/user.h"

#define NTHREADS 3
#define ITERS 100000
#define PGSIZE 4096

volatile int counter = 0;
volatile int ready = 0;
volatile int seen[NTHREADS];

void
worker(void *arg)
{
  int id = (int)(uint64)arg;

  seen[id] = getpid();
  __sync_fetch_and_add(&ready, 1);
  while (ready < NTHREADS)
    ;
  for (int i = 0; i < ITERS; i++)
    counter = counter + 1;
  exit(0);
}

void *
pagestack(void)
{
  char *m = malloc(2 * PGSIZE);
  return (void *)(((uint64)m + PGSIZE - 1) & ~(uint64)(PGSIZE - 1));
}

int
main(void)
{
  int pid;

  printf("threadtest: %d threads x %d increments, no synchronization\n",
         NTHREADS, ITERS);
  printf("main pid %d, &counter = %p\n", getpid(), &counter);

  for (int i = 0; i < NTHREADS; i++) {
    pid = clone(worker, (void *)(uint64)i, pagestack());
    if (pid < 0) {
      printf("clone failed\n");
      exit(1);
    }
    printf("clone -> thread %d pid %d\n", i, pid);
  }

  for (int i = 0; i < NTHREADS; i++)
    printf("join  -> pid %d\n", join());

  for (int i = 0; i < NTHREADS; i++)
    printf("thread %d wrote seen[%d] = %d into shared memory\n", i, i, seen[i]);

  printf("final counter = %d (expected %d)\n", counter, NTHREADS * ITERS);
  if (counter == NTHREADS * ITERS)
    printf("no lost updates this run\n");
  else
    printf("race condition: %d increments lost\n", NTHREADS * ITERS - counter);
  exit(0);
}
