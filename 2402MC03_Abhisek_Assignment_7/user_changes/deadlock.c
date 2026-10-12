#include "kernel/types.h"
#include "user/user.h"

#define MAXN 5
#define MAXM 3

int n, m;
int avail[MAXM];
int alloc[MAXN][MAXM];
int req[MAXN][MAXM];
int maxm[MAXN][MAXM];
int need[MAXN][MAXM];

void
load(int nn, int mm, int a[], int al[][MAXM], int r[][MAXM])
{
  n = nn;
  m = mm;
  for (int j = 0; j < m; j++)
    avail[j] = a[j];
  for (int i = 0; i < n; i++)
    for (int j = 0; j < m; j++) {
      alloc[i][j] = al[i][j];
      req[i][j] = r[i][j];
    }
}

void
printvec(int *v)
{
  for (int j = 0; j < m; j++)
    printf("%d ", v[j]);
}

void
printstate(char *rname, int r[][MAXM])
{
  printf("  Available: ");
  printvec(avail);
  printf("\n  Proc  Allocation  %s\n", rname);
  for (int i = 0; i < n; i++) {
    printf("  P%d    ", i);
    printvec(alloc[i]);
    printf("      ");
    printvec(r[i]);
    printf("\n");
  }
}

int
fits(int *r, int *w)
{
  for (int j = 0; j < m; j++)
    if (r[j] > w[j])
      return 0;
  return 1;
}

int
detect_deadlock(int *dead)
{
  int work[MAXM], finish[MAXN], progress, nd = 0;

  for (int j = 0; j < m; j++)
    work[j] = avail[j];
  for (int i = 0; i < n; i++) {
    finish[i] = 1;
    for (int j = 0; j < m; j++)
      if (alloc[i][j])
        finish[i] = 0;
  }

  do {
    progress = 0;
    for (int i = 0; i < n; i++) {
      if (!finish[i] && fits(req[i], work)) {
        for (int j = 0; j < m; j++)
          work[j] += alloc[i][j];
        finish[i] = 1;
        progress = 1;
        printf("  P%d request satisfiable -> finishes, Work = ", i);
        printvec(work);
        printf("\n");
      }
    }
  } while (progress);

  for (int i = 0; i < n; i++)
    if (!finish[i])
      dead[nd++] = i;
  return nd;
}

void
report(int nd, int *dead)
{
  if (nd == 0) {
    printf("  RESULT: no deadlock\n\n");
    return;
  }
  printf("  RESULT: DEADLOCK among { ");
  for (int i = 0; i < nd; i++)
    printf("P%d ", dead[i]);
  printf("}\n\n");
}

int
is_safe_state(int *seq)
{
  int work[MAXM], finish[MAXN], progress, k = 0;

  for (int j = 0; j < m; j++)
    work[j] = avail[j];
  for (int i = 0; i < n; i++)
    finish[i] = 0;

  do {
    progress = 0;
    for (int i = 0; i < n; i++) {
      if (!finish[i] && fits(need[i], work)) {
        for (int j = 0; j < m; j++)
          work[j] += alloc[i][j];
        finish[i] = 1;
        seq[k++] = i;
        progress = 1;
        printf("    P%d Need <= Work -> runs, Work = ", i);
        printvec(work);
        printf("\n");
      }
    }
  } while (progress);

  if (k < n) {
    printf("    no remaining process has Need <= Work = ");
    printvec(work);
    printf("-> stuck\n");
  }
  return k == n;
}

void
computeneed(void)
{
  for (int i = 0; i < n; i++)
    for (int j = 0; j < m; j++)
      need[i][j] = maxm[i][j] - alloc[i][j];
}

void
safecheck(void)
{
  int seq[MAXN];

  printstate("Need", need);
  if (is_safe_state(seq)) {
    printf("  SAFE, sequence: ");
    for (int i = 0; i < n; i++)
      printf("P%d ", seq[i]);
    printf("\n\n");
  } else {
    printf("  UNSAFE\n\n");
  }
}

int
request(int p, int *r)
{
  int seq[MAXN], ok;

  printf("  Request P%d ( ", p);
  printvec(r);
  printf(")\n");
  if (!fits(r, need[p])) {
    printf("  DENIED: exceeds Need\n\n");
    return 0;
  }
  if (!fits(r, avail)) {
    printf("  WAIT: exceeds Available\n\n");
    return 0;
  }
  for (int j = 0; j < m; j++) {
    avail[j] -= r[j];
    alloc[p][j] += r[j];
    need[p][j] -= r[j];
  }
  printstate("Need", need);
  ok = is_safe_state(seq);
  if (ok) {
    printf("  GRANTED: state stays safe, sequence: ");
    for (int i = 0; i < n; i++)
      printf("P%d ", seq[i]);
    printf("\n\n");
    return 1;
  }
  for (int j = 0; j < m; j++) {
    avail[j] += r[j];
    alloc[p][j] -= r[j];
    need[p][j] += r[j];
  }
  printf("  DENIED: granting would make the state UNSAFE, rolled back\n\n");
  return 0;
}

void
pipedeadlock(void)
{
  int p1[2], p2[2], ready[2], a, b, st, dead[MAXN], nd;
  char c;
  int av[MAXM] = {0, 0};
  int al[MAXN][MAXM] = {{1, 0}, {0, 1}};
  int rq[MAXN][MAXM] = {{0, 1}, {1, 0}};

  printf("=== Test 4: real deadlock with xv6 processes and pipes ===\n");
  pipe(p1);
  pipe(p2);
  pipe(ready);

  if ((a = fork()) == 0) {
    close(p1[0]);
    close(p2[1]);
    close(ready[0]);
    write(ready[1], "A", 1);
    read(p2[0], &c, 1);
    exit(0);
  }
  if ((b = fork()) == 0) {
    close(p2[0]);
    close(p1[1]);
    close(ready[0]);
    write(ready[1], "B", 1);
    read(p1[0], &c, 1);
    exit(0);
  }
  close(p1[0]);
  close(p1[1]);
  close(p2[0]);
  close(p2[1]);
  close(ready[1]);
  read(ready[0], &c, 1);
  read(ready[0], &c, 1);
  pause(10);

  printf("  A (pid %d) holds pipe1 write end, blocked reading pipe2\n", a);
  printf("  B (pid %d) holds pipe2 write end, blocked reading pipe1\n", b);
  printf("  resources: R0 = pipe1, R1 = pipe2; P0 = A, P1 = B\n");
  load(2, 2, av, al, rq);
  printstate("Request", req);
  nd = detect_deadlock(dead);
  report(nd, dead);
  if (nd == 2)
    printf("  circular wait: A -> pipe2 (held by B) -> B -> pipe1 (held by A) -> A\n");

  kill(a);
  kill(b);
  for (int i = 0; i < 2; i++) {
    int pid = wait(&st);
    printf("  reaped pid %d, exit status %d (killed while still blocked)\n", pid, st);
  }
  close(ready[0]);
}

int
main(void)
{
  int dead[MAXN], nd;
  int a1[MAXM] = {0, 0, 0};
  int al1[MAXN][MAXM] = {{0, 1, 0}, {2, 0, 0}, {3, 0, 3}, {2, 1, 1}, {0, 0, 2}};
  int rq1[MAXN][MAXM] = {{0, 0, 0}, {2, 0, 2}, {0, 0, 1}, {1, 0, 0}, {0, 0, 2}};
  int rq2[MAXN][MAXM] = {{0, 0, 0}, {2, 0, 2}, {0, 0, 0}, {1, 0, 0}, {0, 0, 2}};
  int a3[MAXM] = {3, 3, 2};
  int al3[MAXN][MAXM] = {{0, 1, 0}, {2, 0, 0}, {3, 0, 2}, {2, 1, 1}, {0, 0, 2}};
  int mx3[MAXN][MAXM] = {{7, 5, 3}, {3, 2, 2}, {9, 0, 2}, {2, 2, 2}, {4, 3, 3}};
  int r1[MAXM] = {1, 0, 2};
  int r2[MAXM] = {0, 2, 0};

  printf("=== Test 1: detection, deadlocked matrix ===\n");
  load(5, 3, a1, al1, rq1);
  printstate("Request", req);
  nd = detect_deadlock(dead);
  report(nd, dead);

  printf("=== Test 2: detection, safe matrix ===\n");
  load(5, 3, a1, al1, rq2);
  printstate("Request", req);
  nd = detect_deadlock(dead);
  report(nd, dead);

  printf("=== Test 3: Banker's avoidance, safe -> unsafe after request ===\n");
  load(5, 3, a3, al3, rq2);
  for (int i = 0; i < n; i++)
    for (int j = 0; j < m; j++)
      maxm[i][j] = mx3[i][j];
  computeneed();
  safecheck();
  request(1, r1);
  request(0, r2);

  pipedeadlock();
  exit(0);
}
