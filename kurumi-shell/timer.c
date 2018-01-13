#include <iodefine.h>
#include <iodefine_ext.h>
#include "timer.h"

static volatile unsigned char timer_left = 0;

__attribute__((interrupt))
void it_handler(void)
{
  if (timer_left > 0) {
    timer_left--;
  }
}

void timer_setup(void)
{
  RTCEN = 1; /* Enable RTC and interval timer. */

  ITMK  = 1; /* Disable INTIT interrupt... */
  ITIF  = 0; /* ..and clear the interrupt request flag. */

  ITPR1 = 0; /* INTIT interrupt priority level... */
  ITPR0 = 0; /* ...select level 0 (highest). */

  OSMC.osmc = 0x10; /* Use low-speed on-chip oscillator. */
  ITMC.itmc = 0x8096; /* Start and trigger every ~10ms (150 / 15000Hz). */

  ITMK  = 0; /* Enable timer interrupt. */
}

unsigned char timer_read(void)
{
  return timer_left;
}

void timer_set(unsigned char countdown)
{
  timer_left = countdown;
}

