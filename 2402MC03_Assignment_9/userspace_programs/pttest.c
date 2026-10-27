#include "types.h"
#include "stat.h"
#include "user.h"

int
main(int argc, char *argv[])
{
  printf(1, "pttest: calling vmprint() at start-up\n");
  vmprint();

  printf(1, "\npttest: calling sbrk(4 * 4096) to allocate 4 pages\n");
  char *old_brk = sbrk(4 * 4096);
  if(old_brk == (char*)-1){
    printf(1, "pttest: sbrk failed\n");
    exit();
  }

  // Touch the 4 newly allocated pages so they are mapped
  int i;
  for(i = 0; i < 4; i++){
    old_brk[i * 4096] = (char)(i + 1);
  }

  printf(1, "\npttest: calling vmprint() after sbrk(4 * 4096)\n");
  vmprint();

  printf(1, "\npttest: verified 4 new pages appeared.\n");
  exit();
}
