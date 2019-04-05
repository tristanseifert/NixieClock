/**
 * Computes an application's checksum and writes it into the header.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char const *argv[]) {
  int err, size;

  // validate args
  if(argc != 2) {
    fprintf(stderr, "usage: %s [input file]\n", argv[0]);
  }

  // open file
  FILE *app = fopen(argv[1], "r+");

  if(app == NULL) {
    perror("Couldn't open app binary");
    return -1;
  }

  // get the size
  err = fseek(app, 0, SEEK_END);
  if(err != 0) {
    perror("Couldn't seek to end of file");
    return -1;
  }

  size = ftell(app);

  if(size == -1) {
    perror("Couldn't get file position");
    return -1;
  }

  // clean up
  fclose(app);

  return 0;
}
