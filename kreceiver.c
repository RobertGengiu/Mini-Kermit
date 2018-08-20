#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include "lib.h"
#include "package.c"

#define HOST "127.0.0.1"
#define PORT 10001

int main(int argc, char** argv) {
  msg *r, t;
  MPackage *package, *package2;
  int fd_out = -1, counter = 0;
  unsigned char nSeq = 0;
  char last = 'M';
  init(HOST, PORT);

  printf("[%s] Starting\n", argv[0]);

  for(;;)
  {
    // Daca nu s-a primit in total in 4 timeout-uri
    if (counter == 4)
    {
      printf("[%s] Receiving failed\n", argv[0]);

      if (fd_out > 0)
      {
        close(fd_out);
      }

      return -1;
    }

    r = receive_message_timeout(DEFAULT_TIME * 1000);
    if (r == NULL)
    {
      printf("[%s] Timeout\n", argv[0]);

      if (last == TYPE_NOT_ACKNOWLEDGE)
      {
        printf("[%s] Sendig NACK [%d]\n", argv[0], nSeq - 1);
      }

      if (last == TYPE_ACKNOWLEDGE)
      {
        printf("[%s] Sendig ACK [%d]\n", argv[0], nSeq - 1);
      }

      // Nu retransmitem nimic pentru primul pachet
      // Apoi retransmitem ultimul pachet in caz de timeout
      if (last != 'M')
      {
        package = createMPackage(last);
        package->seq = nSeq;
        memset(t.payload, 0, MAX_CAPACITY);
        memcpy(t.payload, &(package->soh), 1);
    	  memcpy(t.payload + 1, &(package->len), 1);
    	  memcpy(t.payload + 2, &(package->seq), 1);
        memcpy(t.payload + 3, &(package->type), 1);
        memcpy(t.payload + package->len - 1, &(package->check), sizeof(short));
        memcpy(t.payload + package->len + 1, &(package->mark), sizeof(char));
        t.len = package->len + 2;
        send_message(&t);
        free(package);
      }

      counter += 1;

    }
    else {

      // Manipulam date din mesajul primit daca este nul
      package2 = (MPackage*)malloc(sizeof(MPackage));
      memset(package2->data, 0, DEFAULT_MAXL);
      package2->len = r->len - 2;
      memcpy(&(package2->soh), r->payload, 1);
      memcpy(&(package2->seq), r->payload + 2, 1);
      memcpy(&(package2->type), r->payload + 3, 1);
      memcpy(&(package2->check), r->payload + r->len - 3, sizeof(short));
	    memcpy(&(package2->mark), r->payload + r->len - 1, sizeof(char));

	    if (r->len > DEFAULT_LEN + 2) {
		    memcpy(package2->data, r->payload + 4, r->len - DEFAULT_LEN - 2);
	    }

      // Actualizam numarul de secventa
      nSeq += 2;
      nSeq %= MAX_SEQUENCES;
      memset(t.payload, 0, MAX_CAPACITY);

      // Verificam daca este corupt sau nu
      if (package2->check == crc16_ccitt(r->payload, r->len - 3))
      {
        counter = 0;
        printf("[%s] Sendig ACK [%d]\n", argv[0], nSeq -1);
        last = TYPE_ACKNOWLEDGE;
        package = createMPackage(TYPE_ACKNOWLEDGE);
        package->seq = nSeq;
        memset(t.payload, 0, MAX_CAPACITY);
        memcpy(t.payload, &(package->soh), 1);
    	  memcpy(t.payload + 1, &(package->len), 1);
    	  memcpy(t.payload + 2, &(package->seq), 1);
        memcpy(t.payload + 3, &(package->type), 1);
        memcpy(t.payload + package->len - 1, &(package->check), sizeof(short));
        memcpy(t.payload + package->len + 1, &(package->mark), sizeof(char));
        t.len = package->len + 2;
        send_message(&t);

        if (package2->type == TYPE_SEND_INIT)
        {

        }

        // Deschidem fisier de iesire
        if (package2->type == TYPE_FILE_HEADER)
        {
          fd_out = open(package2->data, O_WRONLY | O_CREAT, 0644);

          if (fd_out < 0)
          {
            printf("[%s] Error opening output file, exiting.\n", argv[0]);
            return -1;
          }

        }

        // Scriem date in fisier
        if (package2->type == TYPE_DATA)
        {
          printf("[%s] Writing chunk.\n", argv[0]);
          write(fd_out, package2->data, package2->len - 5);
        }

        // Inchidem fisier de iesire
        if (package2->type == TYPE_END_OF_FILE)
        {
          printf("[%s] Finishing file.\n", argv[0]);
          close(fd_out);
        }

        // Oprim transmisia
        if (package2->type == TYPE_END_OF_TRANSMISSION)
        {
          printf("[%s] Closing.\n", argv[0]);
          free(r);
          free(package);
          free(package2);
        	return 0;
        }


        free(package);
        free(package2);

      }
      // Daca am primit un mesaj corupt, trimitem un NACK
      else
      {
        counter = 0;
        printf("[%s] Sendig NACK [%d]\n", argv[0], nSeq - 1);
        last = TYPE_NOT_ACKNOWLEDGE;
        package = createMPackage(TYPE_NOT_ACKNOWLEDGE);
        package->seq = nSeq;
        memset(t.payload, 0, MAX_CAPACITY);
        memcpy(t.payload, &(package->soh), 1);
    	  memcpy(t.payload + 1, &(package->len), 1);
    	  memcpy(t.payload + 2, &(package->seq), 1);
        memcpy(t.payload + 3, &(package->type), 1);
        memcpy(t.payload + package->len - 1, &(package->check), sizeof(short));
        memcpy(t.payload + package->len + 1, &(package->mark), sizeof(char));
        t.len = package->len + 2;
        send_message(&t);
        free(package);
      }
      free(r);
    }
  }

  printf("[%s] Closing.\n", argv[0]);
	return 0;
}
