#include <stdio.h>
void loop(int times);

int main(int argc, char** argv)
{
  loop(6);

  return 0;
}

void loop(int times)
{
  for (int i = 0; i < times; i++)
  {
    puts("Hello, world!");
  }
}
