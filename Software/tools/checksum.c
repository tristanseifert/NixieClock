/**
 * Computes an application's checksum and writes it into the header.
 *
 * NOTE: this will not work right on big endian systems, since it still swaps
 * bytes.
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
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

  printf("File size: %d bytes\n", size);

  // read the file into memory
  uint8_t *blob = (uint8_t *) malloc(size);

  err = fseek(app, 0, SEEK_SET);
  if(err != 0) {
    perror("Couldn't seek to start of file");
    return -1;
  }

  err = fread(blob, 1, size, app);

  if(err != size) {
    perror("Couldn't read full file");
    return -1;
  }

  // validate file; then extract offset (they are big endian)
  uint16_t magic = __builtin_bswap16(*((uint16_t *) blob));

  if(magic != 420) {
    fprintf(stderr, "Invalid magic value: 0x%04x\n", magic);
    return -1;
  }

  // get the start and size (they are big endian)
  uint32_t startOff = __builtin_bswap32(*((uint32_t *) &blob[4]));
  uint32_t endOff = startOff + __builtin_bswap32(*((uint32_t *) &blob[8]));

  printf("Computing checksum over bytes %u to %u\n", startOff, endOff);

  // compute checksum
  uint32_t checksum = 0;

  for(int i = startOff; i < endOff; i++) {
    checksum += blob[i];
  }

  printf("Checksum: 0x%08x\n", checksum);

  // write  checksum into header
  err = fseek(app, 0xC, SEEK_SET);
  if(err != 0) {
    perror("Couldn't seek to checksum");
    return -1;
  }

  uint32_t swappedChecksum = __builtin_bswap32(checksum);

  err = fwrite(&swappedChecksum, sizeof(uint32_t), 1, app);

  if(err != 1) {
    perror("Couldn't write checksum");
    return -1;
  }

  // clean up
  free(blob);
  fclose(app);

  return 0;
}
