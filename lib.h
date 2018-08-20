#ifndef LIB
#define LIB

// Constrains of the protocol
#define DEFAULT_MAXL 250
#define DEFAULT_TIME 5
#define MAX_CAPACITY 1400 // same as payload
#define MAX_SEQUENCES 64
#define DEFAULT_SOH 0x01
#define DEFAULT_EOL 0x0D
#define DEFAULT_SEQ 0X00
#define DEFAULT_LEN 5     // sizeof(MKPackage) - 2
#define DEFAULT_VALUE 0x00
#define MAX_RETRIES 3

// Types of packets
#define TYPE_SEND_INIT 'S'
#define TYPE_FILE_HEADER 'F'
#define TYPE_DATA 'D'
#define TYPE_END_OF_FILE 'Z'
#define TYPE_END_OF_TRANSMISSION 'B'
#define TYPE_ACKNOWLEDGE 'Y'
#define TYPE_NOT_ACKNOWLEDGE 'N'



typedef struct {
    int len;
    char payload[1400];
} msg;

// The structure of MINI-KERMIT package
typedef struct {
  unsigned char soh;        // Start-of-Header
  unsigned char len;        // Length of the package
  unsigned char seq;        // Number of the sequence
  unsigned char type;       // Type of the packet
  char data[DEFAULT_MAXL];  // Actual data
  unsigned short check;     // Used for checking errors
  unsigned char mark;       // Marks the end of the block
} __attribute__ ((__packed__)) MPackage;

// The structure of Send-Init
typedef struct {
  unsigned char maxl;       // The maximum capacity of data
  unsigned char time;       // Timeout duration
  unsigned char npad;       // Number of padding characters
  unsigned char padc;       // Character used for padding
  unsigned char eol;        // Character used in mark
  unsigned char qctl;       // The rest are not used
	unsigned char qbin;
	unsigned char chkt;
	unsigned char rept;
	unsigned char capa;
	unsigned char r;
} __attribute__ ((__packed__)) DataSendInit;

void init(char* remote, int remote_port);
void set_local_port(int port);
void set_remote(char* ip, int port);
int send_message(const msg* m);
int recv_message(msg* r);
msg* receive_message_timeout(int timeout); //timeout in milliseconds
unsigned short crc16_ccitt(const void *buf, int len);

#endif
