#include <stdio.h>
#include <mraa.h>

int main()
{
  printf("MRAA version: %s\n", mraa_get_version());
  return 0;
}
