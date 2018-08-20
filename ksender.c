#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "lib.h"
#include "package.c"

#define HOST "127.0.0.1"
#define PORT 10000

int main(int argc, char** argv)
{
  int fd_in, counter = 1;
  unsigned char nSeq = 0;

  init(HOST, PORT);
  printf("[%s] Starting\n", argv[0]);

  // Trimitem Pachetul Sent-Init
  printf("[%s] Sending Init [%d]\n", argv[0], nSeq);
  if(send_init(&nSeq, argv[0]) < 0)
  {
    return -1;
  }

  while (counter < argc)
  {
    // Deschidem fisierele de intrare
    fd_in = open(argv[counter], O_RDONLY);
    if (fd_in < 0){
      printf("[%s] Error opening input file.\n", argv[0]);
      return 2;
    }

    // Trimitem file-header
    printf("[%s] Sending File-Header [%d] \n", argv[0], nSeq);

    // Formam numele
    char *filename = (char*) malloc(sizeof(char) * 40);
    strcpy(filename, "recv_");
    strcat(filename , argv[counter]);

    if (send_header(&nSeq, argv[0], filename) < 0)
    {
      if (fd_in > 0)
        close(fd_in);

      free(filename);
      return -1;
    }

    // Trimitem datele
    char *holder = (char*) malloc(sizeof(char) *DEFAULT_MAXL);
    int num = 0;
    int size = 0;
    while ((size = read(fd_in, holder, DEFAULT_MAXL - 1)) > 0)
    {
      printf("[%s] Sending Data, chunk %d [%d].\n", argv[0], num, nSeq);
      if (send_data(&nSeq, argv[0], holder, size) < 0)
      {
        if (fd_in > 0) {
          close(fd_in);
        }

        free(holder);
        free(filename);
        return -1;
      }
      num++;
      memset(holder, 0, DEFAULT_MAXL);
    }

    // Trimiteme end of file
    printf("[%s] Sending End of File [%d] \n", argv[0], nSeq);
    if (send_end_of_file(&nSeq, argv[0]) < 0)
    {
      if (fd_in > 0)
        close(fd_in);

      free(holder);
      free(filename);
      return -1;
    }

    free(holder);
    free(filename);
    counter++;
  }

  // Trimitem end of transmission
  printf("[%s] Sending End of Transmission [%d] \n", argv[0], nSeq);
  if (send_end_of_transmission(&nSeq, argv[0]) < 0)
  {
    if (fd_in > 0)
      close(fd_in);

    return -1;
  }

  printf("[%s] Closing.\n", argv[0]);
  return 0;
}
