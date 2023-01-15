#include <stdio.h>
#include <stdint.h> // *should* typedef uint8_t
// Check that uint8_t type exists
#ifndef UINT8_MAX
#error "No support for uint8_t"
#endif

int main(int argc, char *argv[]) {
  if (argc != 2) 
  {
    fprintf(stderr, "Usage: %s <filename>))", argv[0]);
    return 1;
  }
  
  FILE *fp = fopen(argv[1], "r");

  if (fp == NULL)
  {
    fprintf(stderr, "Unable to open file %s for reading data to encode in base64", argv[1]);
    return 1;
  }


  printf("Hello world peopleesss!");

}
