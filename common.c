#include <math.h>

float
percentage(unsigned long int n, unsigned long int d)
{
  /* TBD: Error check here */
  float percent = ((float)n / (float)d)*100.0;
  return percent;
}
