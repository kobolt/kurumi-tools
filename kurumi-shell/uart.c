#include <iodefine.h>
#include <iodefine_ext.h>
#include "uart.h"

static volatile char uart0_recv_byte = '\0';
static volatile char *uart0_send_byte;
static volatile char uart0_send_done = 0;

__attribute__((interrupt))
void sr0_handler(void)
{
  uart0_recv_byte = SDR01.sdr01;
}

__attribute__((interrupt))
void st0_handler(void)
{
  if (*uart0_send_byte != '\0') {
    SDR00.sdr00 = *uart0_send_byte;
    uart0_send_byte++;
  } else {
    uart0_send_done = 1;
  }
}

__attribute__((interrupt))
void tm01h_handler(void) /* INTSRE0 is shared with INTTM01H. */
{
  SIR01.sir01 = SSR01.ssr01 & 0x7; /* Clear error flags. */
}

void uart0_setup(void)
{
  SAU0EN = 1; /* Supply input clock to serial array unit 0. */
  SPS0.sps0 = 0x44; /* Set CK00 and CK01 to 2 MHz. */

  /* UART0 Setup: 9600 baud, 8 data bits, 1 stop bit, no parity. */

  ST0.st0 = 0x3; /* Stop operation of channels 0 and 1. */
  
  STMK0  = 1; /* Disable INTST0 interrupt... */
  STIF0  = 0; /* ...and clear the interrupt request flag. */
  SRMK0  = 1; /* Disable INTSR0 interrupt... */
  SRIF0  = 0; /* ...and clear the interrupt request flag. */
  SREMK0 = 1; /* Disable INTSRE0 interrupt... */
  SREIF0 = 0; /* ...and clear the interrupt request flag. */

  STPR10  = 1; /* INTST0 interrupt priority level... */
  STPR00  = 1; /* ...select level 3 (lowest). */
  SRPR10  = 1; /* INTSR0 interrupt priority level... */
  SRPR00  = 1; /* ...select level 3 (lowest). */
  SREPR10 = 1; /* INTSRE0 interrupt priority level... */
  SREPR00 = 1; /* ...select level 3 (lowest). */

  SMR00.smr00 = 0x22;   /* SAU channel 0 operation mode setting. */
  SCR00.scr00 = 0b1000010010010111; /* SAU channel 0 communication operation setting. */
  SDR00.sdr00 = 0xce00; /* SAU channel 0 transfer clock. */
  NFEN0.nfen0 = 0x1;    /* Noise filter for RxD0 pin. */
  SIR01.sir01 = 0x7;    /* Clear the error flag. */
  SMR01.smr01 = 0x122;  /* SAU channel 1 operation mode setting. */
  SCR01.scr01 = 0b0100010010010111; /* SAU channel 1 communication operation setting. */
  SDR01.sdr01 = 0xce00; /* SAU channel 1 transfer clock. */

  SO0.BIT.bit0  = 1; /* Prepare the use of channel 0 (Serial Output). */
  SOE0.BIT.bit0 = 1; /* Prepare the use of channel 0 (Serial Output Enable). */

  PM1.BIT.bit1 = 1; /* Set the RxD0 pin (input mode). */
  PM1.BIT.bit2 = 0; /* Set the TxD0 pin (output mode). */
  P1.BIT.bit2  = 1; /* Set the TxD0 pin. */
}

void uart0_start(void)
{
  /* Enable transmission interrupt. */
  STIF0 = 0;
  STMK0 = 0;

  /* Enable reception interrupt. */
  SRIF0 = 0;
  SRMK0 = 0;

  /* Enable reception error interrupt. */
  SREIF0 = 0;
  SREMK0 = 0;

  /* Set the TxD0 output level. */
  SO0.BIT.bit0 = 1;

  /* Enable UART0 output. */
  SOE0.BIT.bit0 = 1;

  /* Enable UART0 operation. */
  SS0.BIT.bit0 = 1;
  SS0.BIT.bit1 = 1;
}

void uart0_send(char *s)
{
  if (*s == '\0')
    return;
  uart0_send_byte = s;
  uart0_send_done = 0;

  /* Send first byte, let interrupt handle the rest... */
  STMK0 = 1; /* Mask transmission interrupt. */
  SDR00.sdr00 = *uart0_send_byte;
  uart0_send_byte++;
  STMK0 = 0; /* Cancel transmission interrupt mask. */

  while (uart0_send_done == 0) {
    asm("nop");
  }
}

char uart0_recv(void)
{
  char c;

  /* Enable reception interrupt. */
  SRMK0  = 0;
  SREMK0 = 0;

  if (uart0_recv_byte != '\0') {
    c = uart0_recv_byte;
    uart0_recv_byte = '\0';
  } else {
    c = '\0';
  }

  /* Disable reception interrupt. */
  SRMK0  = 1;
  SREMK0 = 1;

  return c;
}

