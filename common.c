#include <string.h>
#include <math.h>

float
percentage(unsigned long int n, unsigned long int d)
{
  /* TBD: Error check here */
  float percent = ((float)n / (float)d)*100.0;
  return percent;
}


int
str_comp(const void *key, const void *memb) 
{
  const char **a = (const char **)key;
  const char **b = (const char **)memb;
  return strcmp(*a, *b);
}

int
int_comp(const void *key, const void *memb) 
{
  const int a = *(int *)key;
  const int b = *(int *)memb;
  if (a == b) 
    return 0;
  else
    return -1;
}

  
/* TBD : Write something to extract fields from a string */
  
  
  
