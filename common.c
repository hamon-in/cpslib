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
