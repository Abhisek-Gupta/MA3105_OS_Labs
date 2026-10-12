#include "kernel/types.h"
#include "user/user.h"

#define BUFSZ 5
#define NITEMS 20
#define PGSIZE 4096
#define EMPTY 1
#define FULL 2
#define MUTEX 3

int buf[BUFSZ];
int in = 0, out = 0, count = 0, maxcount = 0, errors = 0;

void
producer(void *arg)
{
  for (int item = 1; item <= NITEMS; item++) {
    if (item > NITEMS / 2)
      pause(3);
    sem_wait(EMPTY);
    sem_wait(MUTEX);
    buf[in] = item;
    count++;
    if (count > BUFSZ)
      errors++;
    if (count > maxcount)
      maxcount = count;
    printf("produced %d  slot %d  count %d\n", item, in, count);
    in = (in + 1) % BUFSZ;
    sem_signal(MUTEX);
    sem_signal(FULL);
  }
  exit(0);
}

void
consumer(void *arg)
{
  int item;

  for (int i = 1; i <= NITEMS; i++) {
    if (i <= NITEMS / 2)
      pause(3);
    sem_wait(FULL);
    sem_wait(MUTEX);
    item = buf[out];
    count--;
    if (count < 0)
      errors++;
    printf("               consumed %d  slot %d  count %d\n", item, out, count);
    out = (out + 1) % BUFSZ;
    sem_signal(MUTEX);
    sem_signal(EMPTY);
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
main(void)
{
  sem_init(EMPTY, BUFSZ);
  sem_init(FULL, 0);
  sem_init(MUTEX, 1);
  printf("prodcons: buffer %d, %d items, consumer slow first half, producer slow second half\n",
         BUFSZ, NITEMS);
  clone(producer, 0, pagestack());
  clone(consumer, 0, pagestack());
  join();
  join();
  printf("max occupancy %d / %d, final count %d, overflow/underflow errors %d\n",
         maxcount, BUFSZ, count, errors);
  printf("%s\n", errors == 0 && count == 0 && maxcount <= BUFSZ ? "PASS" : "FAIL");
  exit(0);
}
