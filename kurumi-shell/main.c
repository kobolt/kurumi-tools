#include <iodefine.h>
#include <iodefine_ext.h>
#include "led.h"
#include "uart.h"
#include "timer.h"
#include "command.h"

static void io_port_setup_default(void)
{
  /* Configure analog input alternate-function pins for digital I/O. */
  ADPC.adpc = 0b00000001;
  PMC.pmc   = 0;

  /* Setup port mode register. */
  PM0.pm0   = 0;
  PM1.pm1   = 0;
  PM2.pm2   = 0;
  PM3.pm3   = 0;
  PM4.pm4   = 0;
  PM5.pm5   = 0;
  PM6.pm6   = 0;
  PM7.pm7   = 0;
  PM12.pm12 = 0;
  PM14.pm14 = 0;

  /* Setup pull-up resistor option register. */
  PU0.pu0   = 0;
  PU1.pu1   = 0;
  PU3.pu3   = 0;
  PU4.pu4   = 0;
  PU5.pu5   = 0;
  PU7.pu7   = 0;
  PU12.pu12 = 0;
  PU14.pu14 = 0;

  /* Setup port register. */
  P0.p0   = 0;
  P1.p1   = 0;
  P2.p2   = 0;
  P3.p3   = 0;
  P4.p4   = 0;
  P5.p5   = 0;
  P6.p6   = 0;
  P7.p7   = 0;
  P12.p12 = 0;
  P13.p13 = 0;
  P14.p14 = 0;
}

static void cpu_clock_setup(void)
{
  CMC.cmc = 0x0; /* Disable high-speed system clock and subsystem clock. */

  MSTOP   = 1; /* Disable X1 oscillator. */
  XTSTOP  = 1; /* Disable XT1 oscillator. */
  HIOSTOP = 0; /* Enable high-speed on-chip oscillator. */

  MCM0 = 0; /* Select high-speed on-chip oscillator as main system clock. */
  CSS  = 0; /* Select main system clock as CPU/peripheral hardware clock .*/
}

static void hardware_init(void)
{
  asm("di"); /* Disable interrupts */

  PIOR.pior = 0; /* Disable peripheral I/O redirection function. */

  io_port_setup_default();
  cpu_clock_setup();
  uart0_setup();
  timer_setup();
  led_setup();

  asm("ei"); /* Enable interrupts */
}

int main(void)
{
  hardware_init();
  command_loop();
  return 0;
}

