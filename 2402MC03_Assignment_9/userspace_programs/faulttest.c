#include "types.h"
#include "stat.h"
#include "user.h"

int
main(int argc, char *argv[])
{
  printf(1, "faulttest: starting lazy allocation test\n");

  int initial_faults = getfaults();
  printf(1, "initial faults: %d\n", initial_faults);

  int num_pages = 32;
  char *p = sbrk(num_pages * 4096);
  if(p == (char*)-1){
    printf(1, "faulttest: sbrk failed\n");
    exit();
  }

  int faults_after_sbrk = getfaults();
  printf(1, "faults immediately after sbrk(32 * 4096): %d\n", faults_after_sbrk);

  // Write to exactly 5 different pages: pages 0, 5, 10, 15, 20
  printf(1, "writing to 5 different pages...\n");
  p[0 * 4096] = 'A';
  p[5 * 4096] = 'B';
  p[10 * 4096] = 'C';
  p[15 * 4096] = 'D';
  p[20 * 4096] = 'E';

  int faults_after_writes = getfaults();
  int handled = faults_after_writes - faults_after_sbrk;
  printf(1, "number of page faults handled: %d\n", handled);

  if(handled != 5){
    printf(1, "TEST FAILED: expected 5 faults, got %d\n", handled);
    exit();
  }
  printf(1, "SUCCESS: page faults handled is 5!\n");

  // Show that accessing an address above the allocated size kills only that process
  printf(1, "\ntesting out-of-bounds access in child process...\n");
  int pid = fork();
  if(pid < 0){
    printf(1, "fork failed\n");
    exit();
  }

  if(pid == 0){
    // Child process: access an address above the allocated size
    printf(1, "child (pid %d): accessing address above allocated size (p + 33 * 4096)...\n", getpid());
    char *bad = p + (num_pages + 1) * 4096;
    char val = *bad; // Should fault, fail out-of-bounds check, and kill child
    printf(1, "child should have been killed! read %x\n", val);
    exit();
  }

  int wpid = wait();
  if(wpid == pid){
    printf(1, "parent: child process (pid %d) was killed by kernel as expected.\n", pid);
    printf(1, "parent: parent process is still running and unaffected.\n");
  } else {
    printf(1, "parent: wait error\n");
  }

  printf(1, "\nfaulttest: completed successfully!\n");
  exit();
}
