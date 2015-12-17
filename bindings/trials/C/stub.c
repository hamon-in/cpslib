#include <stdio.h>
#include <dlfcn.h>

#include "../common.h"

typedef int (*foo)(int);

int
main() 
{
  void *lib = dlopen("./libpslib.so", RTLD_NOW);
  foo symb;
  check(lib, "Couldn't open it");
  symb = (foo)dlsym(lib, "cpu_count");
  check(lib, "Couldn't get symbol");
  printf("%d number of cpus\n", (*symb)(0));
  dlclose(lib);
  return 0;
 error:
  ;

}
