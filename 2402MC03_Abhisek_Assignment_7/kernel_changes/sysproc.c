#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"
#include "vm.h"

uint64
sys_exit(void)
{
  int n;
  argint(0, &n);
  kexit(n);
  return 0;
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return kfork();
}

uint64
sys_wait(void)
{
  uint64 p;
  argaddr(0, &p);
  return kwait(p);
}

uint64
sys_sbrk(void)
{
  uint64 addr;
  int t;
  int n;

  argint(0, &n);
  argint(1, &t);
  addr = myproc()->sz;

  if (t == SBRK_EAGER || n < 0) {
    if (growproc(n) < 0) {
      return -1;
    }
  } else {

    if (addr + n < addr)
      return -1;
    if (addr + n > userlimit(myproc()))
      return -1;
    setsz(myproc(), addr + n);
  }
  return addr;
}

uint64
sys_pause(void)
{
  int n;
  uint ticks0;

  argint(0, &n);
  if (n < 0)
    n = 0;
  acquire(&tickslock);
  ticks0 = ticks;
  while (ticks - ticks0 < n) {
    if (killed(myproc())) {
      release(&tickslock);
      return -1;
    }
    sleep_prepare(&ticks);
    release(&tickslock);
    sleep();
    acquire(&tickslock);
  }
  release(&tickslock);
  return 0;
}

uint64
sys_kill(void)
{
  int pid;

  argint(0, &pid);
  return kkill(pid);
}

uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

uint64
sys_clone(void)
{
  uint64 fcn, arg, stack;

  argaddr(0, &fcn);
  argaddr(1, &arg);
  argaddr(2, &stack);
  return kclone(fcn, arg, stack);
}

uint64
sys_join(void)
{
  return kjoin();
}

#define NSEM 10

struct semaphore {
  int value;
  int wakeups;
  int used;
  struct spinlock lk;
};

struct {
  struct spinlock lock;
  struct semaphore sem[NSEM];
} semtable;

void
seminit(void)
{
  initlock(&semtable.lock, "semtable");
  for (int i = 0; i < NSEM; i++)
    initlock(&semtable.sem[i].lk, "sem");
}

uint64
sys_sem_init(void)
{
  int id, value;
  struct semaphore *s;

  argint(0, &id);
  argint(1, &value);
  if (id < 0 || id >= NSEM || value < 0)
    return -1;
  s = &semtable.sem[id];
  acquire(&semtable.lock);
  acquire(&s->lk);
  s->value = value;
  s->wakeups = 0;
  s->used = 1;
  release(&s->lk);
  release(&semtable.lock);
  return 0;
}

uint64
sys_sem_wait(void)
{
  int id;
  struct semaphore *s;

  argint(0, &id);
  if (id < 0 || id >= NSEM)
    return -1;
  s = &semtable.sem[id];
  acquire(&s->lk);
  if (!s->used) {
    release(&s->lk);
    return -1;
  }
  s->value--;
  if (s->value < 0) {
    while (s->wakeups == 0) {
      if (killed(myproc())) {
        s->value++;
        release(&s->lk);
        return -1;
      }
      sleep_prepare(s);
      release(&s->lk);
      sleep();
      acquire(&s->lk);
    }
    s->wakeups--;
  }
  release(&s->lk);
  return 0;
}

uint64
sys_sem_signal(void)
{
  int id;
  struct semaphore *s;

  argint(0, &id);
  if (id < 0 || id >= NSEM)
    return -1;
  s = &semtable.sem[id];
  acquire(&s->lk);
  if (!s->used) {
    release(&s->lk);
    return -1;
  }
  s->value++;
  if (s->value <= 0) {
    s->wakeups++;
    wakeup(s);
  }
  release(&s->lk);
  return 0;
}

uint64
sys_set_priority(void)
{
  int pid, priority;

  argint(0, &pid);
  argint(1, &priority);
  return ksetpriority(pid, priority);
}

uint64
sys_get_priority(void)
{
  int pid;

  argint(0, &pid);
  return kgetpriority(pid);
}
