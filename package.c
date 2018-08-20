#include "lib.h"
////////////////////// Functie ce creaza un pachet ////////////////////////////
MPackage* createMPackage (char type)
{
  MPackage *package = (MPackage*) malloc(sizeof(MPackage));

  if (package == NULL)
  {
    return NULL;
  }
  memset(package->data, 0, DEFAULT_MAXL);

  package->type = type;
  package->soh  = DEFAULT_SOH;
  package->len  = DEFAULT_LEN;
  package->seq  = DEFAULT_SEQ;
  package->mark = DEFAULT_EOL;

  return package;
}
////////////////////////////////////////////////////////////////////////////////


////////////////// Functie ce creaza un camp de date ///////////////////////////
DataSendInit* createDataInit()
{
  DataSendInit *data = (DataSendInit*) malloc(sizeof(MPackage));

  if (data == NULL)
  {
    return NULL;
  }

  data->maxl = DEFAULT_MAXL;
	data->time = DEFAULT_TIME;
	data->eol  = DEFAULT_EOL;
	data->qctl = data->qbin = data->chkt = data->npad = DEFAULT_VALUE;
	data->rept = data->capa = data->padc = data->r    = DEFAULT_VALUE;

  return data;
}
////////////////////////////////////////////////////////////////////////////////


/////////////////////////// Functie ce calculeaza crc //////////////////////////
unsigned short calculateCrc(MPackage *package, int len)
{
    char *holder = (char*)malloc(len + 4);

    if (holder == NULL)
    {
      return -1;
    }

    // Copiem campurile in noua structura
    memcpy(holder, &(package->soh), 1);
	  memcpy(holder + 1, &(package->len), 1);
	  memcpy(holder + 2, &(package->seq), 1);
    memcpy(holder + 3, &(package->type), 1);
    memcpy(holder + 4, package->data, len);

    int result = crc16_ccitt(holder, 4 + len);
    free(holder);
    return result;
}
////////////////////////////////////////////////////////////////////////////////


//////////////////// Functie ce retrimite un pachet de 3 ori ///////////////////
int resend(msg *t, MPackage *package, unsigned char *nSeq, char *arg)
{
  int tries = 0;
  msg* r = NULL;
  for(;;)
  {
    // Daca au trecut 3 timeout la retransmitere
    if (tries == 3)
    {

      printf("[%s] Resending failed\n",arg);
      if (r != NULL)
        free(r);

      return -1;
    }

    r = receive_message_timeout(DEFAULT_TIME * 1100);

    // Daca s-a pierdut mesajul
    if (r == NULL)
    {
      printf("[%s] Timeout\n",arg);
      send_message(t);
      tries += 1;
    }

    else
    {
      // Daca corespunde secventa noua cu cea veche trimitem mesajul
      // cu secventa nemodificata
      if (*nSeq == *(r->payload+2))
      {

        if (*(r->payload+3) == TYPE_ACKNOWLEDGE)
        {
          printf("[%s] Got ACK [%d]\n",arg, *nSeq);
        }

        if (*(r->payload+3) == TYPE_NOT_ACKNOWLEDGE)
        {
          printf("[%s] Got NACK [%d]\n",arg, *nSeq);
        }

        send_message(t);
      }

      else
      {
        tries = 0;
        *nSeq += 2;
        *nSeq %= MAX_SEQUENCES;
        char type;
        memcpy(&type, r->payload + 3, 1);

        // Daca am primit ACK, transmisia s-a terminat
        if (type == TYPE_ACKNOWLEDGE)
        {
          printf("[%s] Got ACK [%d]\n",arg, *nSeq);
          free(r);
          return 0;
      }

      // Schimbam numarul de secventa si recalculam check
      if (type == TYPE_NOT_ACKNOWLEDGE)
      {
        printf("[%s] Got NACK [%d]\n",arg, *nSeq);
        memcpy(t->payload + 2, nSeq, 1);
        unsigned short crc = crc16_ccitt(t->payload, t->len - 3);
        memcpy(t->payload + t->len - 3, &crc, 2);
        free(r);
        send_message(t);
      }
    }
  }
}
return 0;
}
////////////////////////////////////////////////////////////////////////////////


////////////////// Functie ce trimite un pachet send_init///////////////////////
int send_init(unsigned char *nSeq, char *arg)
{
  msg t;
  MPackage *package = createMPackage(TYPE_SEND_INIT);
  DataSendInit *data = createDataInit();
  package->len += sizeof(DataSendInit);
  memcpy(package->data, &data, sizeof(DataSendInit));
  package->check = calculateCrc(package, sizeof(DataSendInit));
  memset(t.payload, 0, MAX_CAPACITY);
  memcpy(t.payload, &(package->soh), 1);
  memcpy(t.payload + 1, &(package->len), 1);
  memcpy(t.payload + 2, &(package->seq), 1);
  memcpy(t.payload + 3, &(package->type), 1);
  memcpy(t.payload + 4, package->data, package->len - 5);
  memcpy(t.payload + package->len - 1, &(package->check), sizeof(short));
  memcpy(t.payload + package->len + 1, &(package->mark), sizeof(char));
  t.len = package->len + 2;
  free(data);
  send_message(&t);

  if (resend(&t, package, nSeq, arg) < 0 )
  {
    free(package);
    return -1;
  }

  free(package);
  return 0;
}
////////////////////////////////////////////////////////////////////////////////


//////////////// Functie ce trimite un pachet cu file-header ///////////////////
int send_header(unsigned char *nSeq, char *arg, char *name)
{
  msg t;
  MPackage *package = createMPackage(TYPE_FILE_HEADER);
  package->len += strlen(name);
  memcpy(package->data, name, strlen(name));
  package->check = calculateCrc(package, strlen(name));
  memset(t.payload, 0, MAX_CAPACITY);
  memcpy(t.payload, &(package->soh), 1);
  memcpy(t.payload + 1, &(package->len), 1);
  memcpy(t.payload + 2, &(package->seq), 1);
  memcpy(t.payload + 3, &(package->type), 1);
  memcpy(t.payload + 4, package->data, package->len - 5);
  memcpy(t.payload + package->len - 1, &(package->check), sizeof(short));
  memcpy(t.payload + package->len + 1, &(package->mark), sizeof(char));
  t.len = package->len + 2;
  send_message(&t);

  if (resend(&t, package, nSeq, arg) < 0 )
  {
    free(package);
    return -1;
  }
  free(package);
  return 0;
}
////////////////////////////////////////////////////////////////////////////////


////////////// Functie ce trimite un pachet cu informatii utile ////////////////
int send_data(unsigned char *nSeq, char *arg, char *holder, int size)
{
  msg t;
  MPackage *package = createMPackage(TYPE_DATA);
  package->len += size;
  memcpy(package->data, holder, size);
  package->check = calculateCrc(package, size);
  memset(t.payload, 0, MAX_CAPACITY);
  memcpy(t.payload, &(package->soh), 1);
  memcpy(t.payload + 1, &(package->len), 1);
  memcpy(t.payload + 2, &(package->seq), 1);
  memcpy(t.payload + 3, &(package->type), 1);
  memcpy(t.payload + 4, package->data, package->len - 5);
  memcpy(t.payload + package->len - 1, &(package->check), sizeof(short));
  memcpy(t.payload + package->len + 1, &(package->mark), sizeof(char));
  t.len = package->len + 2;
  send_message(&t);

  if (resend(&t, package, nSeq, arg) < 0 )
  {
    free(package);
    return -1;
  }

  free(package);
  return 0;
}
////////////////////////////////////////////////////////////////////////////////


//////////////// Functie ce trimite un pachet de incheiere /////////////////////
int send_end_of_file(unsigned char *nSeq, char *arg)
{
  msg t;
  MPackage *package = createMPackage(TYPE_END_OF_FILE);
  package->check = calculateCrc(package, 0);
  memset(t.payload, 0, MAX_CAPACITY);
  memcpy(t.payload, &(package->soh), 1);
  memcpy(t.payload + 1, &(package->len), 1);
  memcpy(t.payload + 2, &(package->seq), 1);
  memcpy(t.payload + 3, &(package->type), 1);
  memcpy(t.payload + 4, package->data, package->len - 5);
  memcpy(t.payload + package->len - 1, &(package->check), sizeof(short));
  memcpy(t.payload + package->len + 1, &(package->mark), sizeof(char));
  t.len = package->len + 2;
  send_message(&t);

  if (resend(&t, package, nSeq, arg) < 0 )
  {
    free(package);
    return -1;
  }
  free(package);
  return 0;
}
////////////////////////////////////////////////////////////////////////////////


//////////// Functie ce trimite un pachet de sfarsit de transmisie /////////////
int send_end_of_transmission(unsigned char *nSeq, char *arg)
{
  msg t;
  MPackage *package = createMPackage(TYPE_END_OF_TRANSMISSION);
  package->check = calculateCrc(package, 0);
  memset(t.payload, 0, MAX_CAPACITY);
  memcpy(t.payload, &(package->soh), 1);
  memcpy(t.payload + 1, &(package->len), 1);
  memcpy(t.payload + 2, &(package->seq), 1);
  memcpy(t.payload + 3, &(package->type), 1);
  memcpy(t.payload + 4, package->data, package->len - 5);
  memcpy(t.payload + package->len - 1, &(package->check), sizeof(short));
  memcpy(t.payload + package->len + 1, &(package->mark), sizeof(char));
  t.len = package->len + 2;
  send_message(&t);

  if (resend(&t, package, nSeq, arg) < 0 )
  {
    free(package);
    return -1;
  }

  free(package);
  return 0;
}
////////////////////////////////////////////////////////////////////////////////
