#include "opcode.h"
#include "led.h"
#include "script.h"

char *opcode_command(opcode_t op)
{
  switch (op) {
  case OPCODE_SLEEP_10:     return "sleep 10";
  case OPCODE_SLEEP_50:     return "sleep 50";
  case OPCODE_SLEEP_100:    return "sleep 100";
  case OPCODE_SLEEP_500:    return "sleep 500";
  case OPCODE_SLEEP_1000:   return "sleep 1000";
  case OPCODE_RED_OFF:      return "red off";
  case OPCODE_RED_ON:       return "red on";
  case OPCODE_RED_TOGGLE:   return "red toggle";
  case OPCODE_GREEN_OFF:    return "green off";
  case OPCODE_GREEN_ON:     return "green on";
  case OPCODE_GREEN_TOGGLE: return "green toggle";
  case OPCODE_BLUE_OFF:     return "blue off";
  case OPCODE_BLUE_ON:      return "blue on";
  case OPCODE_BLUE_TOGGLE:  return "blue toggle";
  case OPCODE_SCRIPT_RUN:   return "run";
  case OPCODE_SCRIPT_STOP:  return "stop";
  case OPCODE_SCRIPT_DUMP:  return "dump";
  case OPCODE_SCRIPT_CLEAR: return "clear";
  case OPCODE_SCRIPT_STATE: return "state";
  default: 
    return "";
  }
}

unsigned int opcode_execute(opcode_t op)
{
  switch (op) {
  case OPCODE_NONE:
    break;

  case OPCODE_SLEEP_10:
    return 10;

  case OPCODE_SLEEP_50:
    return 50;

  case OPCODE_SLEEP_100:
    return 100;

  case OPCODE_SLEEP_500:
    return 500;

  case OPCODE_SLEEP_1000:
    return 1000;

  case OPCODE_RED_OFF:
    led_red_command(LED_OFF);
    break;

  case OPCODE_RED_ON:
    led_red_command(LED_ON);
    break;

  case OPCODE_RED_TOGGLE:
    led_red_command(LED_TOGGLE);
    break;

  case OPCODE_GREEN_OFF:
    led_green_command(LED_OFF);
    break;

  case OPCODE_GREEN_ON:
    led_green_command(LED_ON);
    break;

  case OPCODE_GREEN_TOGGLE:
    led_green_command(LED_TOGGLE);
    break;

  case OPCODE_BLUE_OFF:
    led_blue_command(LED_OFF);
    break;

  case OPCODE_BLUE_ON:
    led_blue_command(LED_ON);
    break;

  case OPCODE_BLUE_TOGGLE:
    led_blue_command(LED_TOGGLE);
    break;

  case OPCODE_SCRIPT_RUN:
    script_run();
    break;

  case OPCODE_SCRIPT_STOP:
    script_stop();
    break;

  case OPCODE_SCRIPT_DUMP:
    script_dump();
    break;

  case OPCODE_SCRIPT_CLEAR:
    script_clear();
    break;

  case OPCODE_SCRIPT_STATE:
    script_state_print();
    break;

  default:
    break;
  }

  return 0; /* No time to sleep. */
}

