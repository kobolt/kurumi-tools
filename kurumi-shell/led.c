#include <iodefine.h>
#include <iodefine_ext.h>
#include "led.h"

#define LED_RED   P1.BIT.bit7
#define LED_GREEN P5.BIT.bit1
#define LED_BLUE  P5.BIT.bit0

void led_setup(void)
{
  /* Turn off all LEDs. */
  LED_RED   = 1;
  LED_GREEN = 1;
  LED_BLUE  = 1;
}

void led_red_command(LED_COMMAND cmd)
{
  switch (cmd) {
  case LED_OFF:
    LED_RED = 1;
    break;

  case LED_ON:
    LED_RED = 0;
    break;

  case LED_TOGGLE:
    LED_RED ^= 1;
    break;
  }
}

void led_green_command(LED_COMMAND cmd)
{
  switch (cmd) {
  case LED_OFF:
    LED_GREEN = 1;
    break;

  case LED_ON:
    LED_GREEN = 0;
    break;

  case LED_TOGGLE:
    LED_GREEN ^= 1;
    break;
  }
}

void led_blue_command(LED_COMMAND cmd)
{
  switch (cmd) {
  case LED_OFF:
    LED_BLUE = 1;
    break;

  case LED_ON:
    LED_BLUE = 0;
    break;

  case LED_TOGGLE:
    LED_BLUE ^= 1;
    break;
  }
}

