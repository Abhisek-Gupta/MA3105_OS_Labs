// kernel/sem.c  --  NEW FILE
//
// A small, lightweight counting-semaphore facility for xv6-riscv,
// implemented directly on top of this kernel's existing blocking
// primitives -- sleep_prepare(chan)/sleep()/wakeup(chan) -- the same
// mechanism this kernel uses internally for its own blocking (e.g.
// sleeplock.c). This gives user programs REAL blocking (the waiting
// process is taken off the run queue, not spinning), which is required
// for Question 2: the producer must actually block when the buffer is
// full and the consumer must actually block when the buffer is empty.
//
// The kernel keeps a small fixed-size table of semaphores, each
// identified by a small integer id chosen by the user program (e.g.
// 0 = "empty", 1 = "full", 2 = "mutex" in prodcons.c). Three syscalls
// are exposed:
//
//   int sem_init(int id, int value)   // set semaphore `id` to `value`
//   int sem_wait(int id)              // P() / down(): blocks if <= 0
//   int sem_signal(int id)            // V() / up(): wakes a waiter
//
// This is intentionally system-wide/global (like shm_get()'s single
// shared page from Question 1) which is sufficient for this assignment.
 
#include "types.h"
#include "riscv.h"
#include "param.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"
 
#define NSEM 16   // how many independent semaphores the kernel supports
 
struct semaphore {
  struct spinlock lock;  // protects `value`; also the sleep/wakeup lock
  int value;
  int inited;
};
 
static struct semaphore semtable[NSEM];
 
// Call once at boot, from main.c.
void
seminit(void)
{
  for(int i = 0; i < NSEM; i++){
    initlock(&semtable[i].lock, "sem");
    semtable[i].value = 0;
    semtable[i].inited = 0;
  }
}
 
// (Re-)initialize semaphore `id` to `value`. Must be called before the
// first sem_wait()/sem_signal() on that id (typically by the parent,
// before it forks).
int
sem_init(int id, int value)
{
  if(id < 0 || id >= NSEM)
    return -1;
  acquire(&semtable[id].lock);
  semtable[id].value = value;
  semtable[id].inited = 1;
  release(&semtable[id].lock);
  return 0;
}
 
// P(id) / down(id): decrement the semaphore. If it's already <= 0,
// block until sem_signal() wakes us, then re-check the condition (the
// while-loop guards against more than one waiter being woken for a
// single signal, and against spurious wakeups).
//
// This kernel's sleep/wakeup API is split into two calls instead of the
// classic single sleep(chan, lock):
//   - sleep_prepare(chan)  records our intent to sleep on `chan` (it
//     does this under the process's own p->lock internally), which is
//     safe to call while we still hold s->lock.
//   - sleep()              actually blocks -- but only if nothing has
//     woken us in the meantime (i.e. only if p->chan is still set).
// So the safe pattern is: sleep_prepare(chan); release(ourlock);
// sleep(); re-acquire(ourlock); recheck condition. This avoids the
// classic lost-wakeup race even though ourlock is released before the
// process actually goes to sleep.
int
sem_wait(int id)
{
  if(id < 0 || id >= NSEM)
    return -1;
  struct semaphore *s = &semtable[id];
 
  acquire(&s->lock);
  while(s->value <= 0){
    sleep_prepare(s);     // register intent to sleep on chan = s
    release(&s->lock);    // safe to release now; wakeup() can't be missed
    sleep();               // actually block (no-op if already woken)
    acquire(&s->lock);     // re-acquire before rechecking the condition
  }
  s->value--;
  release(&s->lock);
  return 0;
}
 
// V(id) / up(id): increment the semaphore and wake up anyone waiting
// on it. wakeup() is called after releasing s->lock, which is safe
// here (wakeup/sleep_prepare synchronize via each process's own
// p->lock) and avoids holding s->lock while scanning the whole process
// table.
int
sem_signal(int id)
{
  if(id < 0 || id >= NSEM)
    return -1;
  struct semaphore *s = &semtable[id];
 
  acquire(&s->lock);
  s->value++;
  release(&s->lock);
  wakeup(s);
  return 0;
}
 
