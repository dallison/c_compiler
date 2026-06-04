#include <stdio.h>
#include <stdint.h>

int32_t factorial(int32_t i) 
{
   if (i < 2)
      return i;
   else
      return i * factorial(i - 1);
}

int main()
{
   int Count;

   for (Count = 1; Count <= 10; Count++)
      printf("%ld\n", factorial(Count));

   return 0;
}

/* vim: set expandtab ts=4 sw=3 sts=3 tw=80 :*/
