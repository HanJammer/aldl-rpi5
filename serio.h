
#ifndef _SERIO_H
#define _SERIO_H

#include "aldl-types.h"

/************ SCOPE *********************************
  Each serial module must contain these functions.
****************************************************/

/* initalize the serial handler */
int serial_init(char *port);

/* write buffer *str to the serial port, up to len bytes */
int serial_write(byte *str, int len);

/* read data from the serial port to buf, returns number of bytes read.
   only reads UP TO len, doesn't stick around waiting for more data if it
   isn't there. */
int serial_read(byte *str, int len);

/* clears any i/o buffers */
void serial_purge(); /* both buffers */
void serial_purge_rx(); /* rx only */
void serial_purge_tx(); /* tx only */

/* device search helper */
void serial_help_devs();

/* get serial status 1=OK */
int serial_get_status();

/* milliseconds since a byte last actually arrived from the wire.  used by
   the acquisition watchdog to detect a silently dead line, where reads
   technically succeed but never return any data.  returns 0 until the
   first byte ever arrives, so the watchdog stays disarmed while waiting
   for the car to show up. */
unsigned long serial_ms_since_rx();

/* a gentle recovery: close and reopen the device. */
void serial_soft_recovery();

/* a heavier recovery: reset the usb device (if the driver supports that)
   before reopening.  safe to call on a wedged-but-enumerated adaptor. */
void serial_hard_recovery();

#endif

