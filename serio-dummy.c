#include <stdio.h>
#include <malloc.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>

#include "serio.h"
#include "aldl-io.h"
#include "error.h"
#include "config.h"
#include "useful.h"

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

/* nonzero = pretend to be a wedged adaptor: all i/o calls still succeed
   but no data ever arrives.  toggled by SIGUSR1 at runtime, for bench
   testing the acquisition watchdog without a car. */
volatile sig_atomic_t dummy_silent;
char dummy_silent_reported;

/* rx freshness tracking for serial_ms_since_rx() */
timespec_t last_rx;
byte last_rx_valid;

void gen_pkt(int len);

static int dummy_read(byte *str, int len);

/* SIGUSR1 toggles silence, simulating the ft232r wedge observed in the
   2026-08-08 road test: the 'wire' goes completely quiet but the device
   is still there and every call succeeds. */
void dummy_toggle_silence(int signum) {
  (void)signum;
  dummy_silent = !dummy_silent;
}

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
  dummy_silent=0;
  dummy_silent_reported=0;
  signal(SIGUSR1,dummy_toggle_silence);
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

int serial_read(byte *str, int len) {
  int resp;
  if(dummy_silent_reported != (dummy_silent != 0)) {
    dummy_silent_reported = (dummy_silent != 0);
    fprintf(stderr,"DUMMY DRIVER: silence mode %s\n",
            dummy_silent_reported ? "ON (wire dead)" : "OFF (wire alive)");
  }
  if(dummy_silent == 1) { /* wedged adaptor: succeed with no data, ever */
    usleep(SLEEPYTIME);
    return 0;
  }
  resp = dummy_read(str,len);
  if(resp > 0) {
    last_rx = get_time();
    last_rx_valid = 1;
  }
  return resp;
}

static int dummy_read(byte *str, int len) {
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

unsigned long serial_ms_since_rx() {
  if(last_rx_valid == 0) return 0;
  return get_elapsed_ms(last_rx);
}

/* recovery is deliberately a no-op here: the simulated wedge, like the
   real one, survives reopen and usb reset.  clear it with SIGUSR1, which
   plays the role of the physical power cycle. */
void serial_soft_recovery() {
  fprintf(stderr,"DUMMY DRIVER: soft recovery requested (no-op)\n");
}

void serial_hard_recovery() {
  fprintf(stderr,"DUMMY DRIVER: hard recovery requested (no-op)\n");
}
