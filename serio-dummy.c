#include <stdio.h>
#include <malloc.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>

#include "serio.h"
#include "aldl-io.h"
#include "error.h"
#include "config.h"

/************ SCOPE *********************************
  A dummy serial handler object that pretends to be
  a fake LT1 (EE) ECM and just send random garbage
  (00-FF) as a datastream.
****************************************************/

/****************GLOBALSn'STRUCTURES*****************************/

unsigned char *databuff;
byte lastwrite[5];
int lastwrite_len;
char txmode;

void gen_pkt(int len);

/****************FUNCTIONS**************************************/

void serial_close() {
  #ifdef SERIAL_VERBOSE
  printf("SERIAL CLOSE (discarded)\n");
  #endif
  return;
}

void gen_pkt(int len) {
  int x;
  if(len < 4) return;
  if(len > ALDL_COMMBUFFER) len = ALDL_COMMBUFFER;
  databuff[0]=(lastwrite_len > 0) ? lastwrite[0] : 0xF4;
  databuff[1]=calc_msglength(len);
  databuff[2]=0x01;
  for(x=3;x<len-1;x++) databuff[x] = ( (byte)rand() % 256 ) - 1;
  databuff[len-1] = checksum_generate(databuff,len-1);
  #ifdef DUMMY_CORRUPTION_ENABLE
  /* insert random bullshit sometimes */
  if( ( (byte)rand() % 100 ) < DUMMY_CORRPUTION_RATE ) {
    #ifdef SERIAL_VERBOSE
    printf("serial dummy driver - inserting random corruption!\n");
    #endif
    for(x=0;x<=DUMMY_CORRUPTION_AMOUNT;x++) {
      databuff[(byte)rand() % 60] = ( (byte)rand() % 256 ) - 1;
    }
  }
  #endif
}

int serial_init(char *port) {
  #ifdef SERIAL_VERBOSE
  printf("Serial dummy driver initialized!\n");
  #endif
  txmode=0;
  lastwrite_len=0;
  memset(lastwrite,0,sizeof(lastwrite));
  databuff=malloc(ALDL_COMMBUFFER);
  return 1;
}

void serial_purge() {
  #ifdef SERIAL_VERBOSE
  printf("SERIAL PURGE RX/TX (Dummy Ignored)\n");
  #endif
  return;
}

void serial_purge_rx() {
  #ifdef SERIAL_VERBOSE
  printf("SERIAL PURGE RX (Dummy Ignored)\n");
  #endif
  return;
}

void serial_purge_tx() {
  #ifdef SERIAL_VERBOSE
  printf("SERIAL PURGE TX (Dummy Ignored)\n");
  #endif
  return;
}

int serial_write(byte *str, int len) {
  #ifdef SERIAL_VERBOSE
  printf("WRITE: ");
  printhexstring(str,len); 
  #endif
  if(len > 5) len = 5;
  memcpy(lastwrite,str,len);
  lastwrite_len=len;
  /* determine mode */
  if(len == 4 && str[2] == 0x08) {
     txmode = 1;
  } else if(len == 4 && str[2] == 0x00) {
     txmode = 0;
  } else if(len == 5 && str[2] == 0x01) {
     txmode = 2;
  }
  return 0;
}

inline int serial_read(byte *str, int len) {
  if(txmode == 0) { /* idle traffic req */
    usleep(SERIAL_BYTES_PER_MS * 64 * 1000); /* fake baud delay */
    str[0] = 0x33;
    txmode++;
    #ifdef SERIAL_VERBOSE
    printf("DUMMY MODE: Idle Traffic Req: ");
    printhexstring(str,1);
    #endif
    return 1;
  } if(txmode == 1) { /* shutup req */
    usleep(SERIAL_BYTES_PER_MS * 5 * 1000); /* fake baud delay */
    if(lastwrite_len > len) return 0;
    memcpy(str,lastwrite,lastwrite_len);
    txmode++;
    #ifdef SERIAL_VERBOSE
    printf("DUMMY MODE: Silence Request: ");
    printhexstring(str,lastwrite_len);
    #endif
    return lastwrite_len;
  } if(txmode == 2) { /* data request reply */
    usleep(SERIAL_BYTES_PER_MS * 5 * 1000); /* fake baud delay */
    if(lastwrite_len > len) return 0;
    memcpy(str,lastwrite,lastwrite_len);
    txmode = 3; 
    #ifdef SERIAL_VERBOSE
    printf("DUMMY MODE: Data Req. Reply: ");
    printhexstring(str,lastwrite_len);
    #endif
    return lastwrite_len;
  } if(txmode == 3) { /* data send */
    usleep(SERIAL_BYTES_PER_MS * len * 1000); /* fake baud delay */
    txmode = 2;
    gen_pkt(len);
    #ifdef SERIAL_VERBOSE
    printf("DUMMY MODE: Generated packet...\n");
    #endif
    int x;
    for(x=0;x<len;x++) {
      str[x] = databuff[x]; 
    }
    return len;
  }
  return 0;
}

void serial_help_devs() {
  error(1,ERROR_GENERAL,"this serial driver has no devices......");
}

int serial_get_status() {
  return 1;
}
