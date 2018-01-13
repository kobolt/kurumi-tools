#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>



#define BLOCK_SIZE 1024 /* In bytes. */

typedef enum {
  RL78_COMMAND_RESET             = 0x00,
  RL78_COMMAND_VERIFY            = 0x13,
  RL78_COMMAND_BLOCK_ERASE       = 0x22,
  RL78_COMMAND_BLOCK_BLANK_CHECK = 0x32,
  RL78_COMMAND_PROGRAMMING       = 0x40,
  RL78_COMMAND_BAUD_RATE_SET     = 0x9a,
  RL78_COMMAND_SECURITY_SET      = 0xa0,
  RL78_COMMAND_SECURITY_GET      = 0xa1,
  RL78_COMMAND_SECURITY_RELEASE  = 0xa2,
  RL78_COMMAND_CHECKSUM          = 0xb0,
  RL78_COMMAND_SILICON_SIGNATURE = 0xc0,
} RL78_COMMAND;

typedef enum {
  RL78_STATUS_COMMAND_NUMBER_ERROR = 0x04,
  RL78_STATUS_PARAMETER_ERROR      = 0x05,
  RL78_STATUS_NORMAL_ACK           = 0x06,
  RL78_STATUS_CHECKSUM_ERROR       = 0x07,
  RL78_STATUS_VERIFY_ERROR         = 0x0f,
  RL78_STATUS_PROTECT_ERROR        = 0x10,
  RL78_STATUS_NEGATIVE_ACK         = 0x15,
  RL78_STATUS_ERASE_ERROR          = 0x1a,
  RL78_STATUS_IVERIFY_BLANK_ERROR  = 0x1b,
  RL78_STATUS_WRITE_ERROR          = 0x1c,
} RL78_STATUS;



static int print_traffic = 0;
static int print_details = 1;



static char *rl78_status_text(int status)
{
  switch (status) {
  case RL78_STATUS_COMMAND_NUMBER_ERROR:
    return "Command number error";
  case RL78_STATUS_PARAMETER_ERROR:
    return "Parameter error";
  case RL78_STATUS_NORMAL_ACK:
    return "Normal acknowledgement";
  case RL78_STATUS_CHECKSUM_ERROR:
    return "Checksum error";
  case RL78_STATUS_VERIFY_ERROR:
    return "Verify error";
  case RL78_STATUS_PROTECT_ERROR:
    return "Protect error";
  case RL78_STATUS_NEGATIVE_ACK:
    return "Negative acknowledgement";
  case RL78_STATUS_ERASE_ERROR:
    return "Erase error";
  case RL78_STATUS_IVERIFY_BLANK_ERROR:
    return "Internal verify error or blank check error";
  case RL78_STATUS_WRITE_ERROR:
    return "Write error";
  default:
    return "Unknown error";
  }
}



static int generate_checksum(unsigned char *data, int data_len)
{
  int i, checksum;

  if (data_len == 0) {
    data_len = 0x100;
  }

  checksum = (0 - data_len);
  for (i = 0; i < data_len; i++) {
    checksum -= data[i];
  }

  return checksum & 0xff;
}



static int frame_is_complete(unsigned char *frame, int frame_len)
{
  int internal_frame_len;

  if (frame_len < 5) {
    return 0;
  }

  if (frame[1] == 0) {
    internal_frame_len = 0x100;
  } else {
    internal_frame_len = frame[1];
  }

  if (internal_frame_len == (frame_len - 4)) {
    return 1;
  } else {
    return 0;
  }
}



static int frame_send(int tty_fd, unsigned char *frame, int frame_len)
{
  int result, i;

  if (print_traffic) {
    printf(">>> ");
    for (i = 0; i < frame_len; i++) {
      printf("%02x ", frame[i]);
    }
    printf("\n");
  }

  result = write(tty_fd, frame, frame_len);
  if (result == -1) {
    fprintf(stderr, "write() failed: %s\n", strerror(errno));
    return -1;
  }

  return frame_len;
}



static int frame_recv(int tty_fd, unsigned char *frame, int frame_len_max)
{
  int result, frame_len, checksum;
  unsigned char byte;

  if (print_traffic)
    printf("<<< ");

  frame_len = 0;
  while (1) {
    result = read(tty_fd, &byte, 1);
    if (result == -1) {
      if (errno != EAGAIN && errno != EWOULDBLOCK) {
        fprintf(stderr, "read() failed: %s\n", strerror(errno));
        return -1;
      }

    } else if (result > 0) {
      if (print_traffic)
        printf("%02x ", byte);

      if (frame_len == frame_len_max) {
        fprintf(stderr, "frame_recv() failed: Overflow\n");
        return -1;
      }

      frame[frame_len++] = byte;

      if (frame_is_complete(frame, frame_len)) {
        break;
      }
    }

    usleep(10);
  }

  if (print_traffic)
    printf("\n");

  checksum = generate_checksum(&frame[2], frame[1]);
  if (checksum != frame[frame_len - 2]) {
    fprintf(stderr, "frame_recv() failed: Checksum incorrect\n");
    return -1;
  }

  return frame_len;
}



static int command_baud_rate_set(int tty_fd)
{
  int cmd_frame_len;
  unsigned char cmd_frame[8];
  unsigned char status_frame[8];

  cmd_frame_len = 0;
  cmd_frame[cmd_frame_len++] = 0x01; /* Command Frame Header */
  cmd_frame[cmd_frame_len++] = 0x03; /* Command Information Length */
  cmd_frame[cmd_frame_len++] = RL78_COMMAND_BAUD_RATE_SET;
  cmd_frame[cmd_frame_len++] = 0x00; /* Baud rate setting = 115200 */
  cmd_frame[cmd_frame_len++] = 0x21; /* Voltage setting = 3.3V */
  cmd_frame[cmd_frame_len++] = generate_checksum(&cmd_frame[2], cmd_frame[1]);
  cmd_frame[cmd_frame_len++] = 0x03; /* Command Frame Footer */

  if (frame_send(tty_fd, cmd_frame, cmd_frame_len) < 0) {
    return -1;
  }

  if (frame_recv(tty_fd, status_frame, sizeof(status_frame)) < 0) {
    return -1;
  }

  if (status_frame[2] != RL78_STATUS_NORMAL_ACK) {
    fprintf(stderr, "command_baud_rate_set() failed: %s (0x%02x)\n",
      rl78_status_text(status_frame[2]), status_frame[2]);
    return -1;
  }

  if (print_details) {
    printf("Frequency: %d MHz\n", status_frame[3]);
    printf("Programming mode: %s\n", status_frame[4] ? "Wide-voltage" : "Full-speed");
  }

  return 0;
}



static int command_reset(int tty_fd)
{
  int cmd_frame_len;
  unsigned char cmd_frame[8];
  unsigned char status_frame[8];

  cmd_frame_len = 0;
  cmd_frame[cmd_frame_len++] = 0x01; /* Command Frame Header */
  cmd_frame[cmd_frame_len++] = 0x01; /* Command Information Length */
  cmd_frame[cmd_frame_len++] = RL78_COMMAND_RESET;
  cmd_frame[cmd_frame_len++] = generate_checksum(&cmd_frame[2], cmd_frame[1]);
  cmd_frame[cmd_frame_len++] = 0x03; /* Command Frame Footer */

  if (frame_send(tty_fd, cmd_frame, cmd_frame_len) < 0) {
    return -1;
  }

  if (frame_recv(tty_fd, status_frame, sizeof(status_frame)) < 0) {
    return -1;
  }

  if (status_frame[2] != RL78_STATUS_NORMAL_ACK) {
    fprintf(stderr, "command_reset() failed: %s (0x%02x)\n",
      rl78_status_text(status_frame[2]), status_frame[2]);
    return -1;
  }

  return 0;
}



static int command_silicon_signature(int tty_fd)
{
  int cmd_frame_len, status_frame_len, data_frame_len;
  unsigned char cmd_frame[8];
  unsigned char status_frame[8];
  unsigned char data_frame[32];

  cmd_frame_len = 0;
  cmd_frame[cmd_frame_len++] = 0x01; /* Command Frame Header */
  cmd_frame[cmd_frame_len++] = 0x01; /* Command Information Length */
  cmd_frame[cmd_frame_len++] = RL78_COMMAND_SILICON_SIGNATURE;
  cmd_frame[cmd_frame_len++] = generate_checksum(&cmd_frame[2], cmd_frame[1]);
  cmd_frame[cmd_frame_len++] = 0x03; /* Command Frame Footer */

  if (frame_send(tty_fd, cmd_frame, cmd_frame_len) < 0) {
    return -1;
  }

  if ((status_frame_len = frame_recv(tty_fd, status_frame, sizeof(status_frame))) < 0) {
    return -1;
  }

  if (status_frame[2] != RL78_STATUS_NORMAL_ACK) {
    fprintf(stderr, "command_silicon_signature() failed: %s (0x%02x)\n",
      rl78_status_text(status_frame[2]), status_frame[2]);
    return -1;
  }

  if ((data_frame_len = frame_recv(tty_fd, data_frame, sizeof(data_frame))) < 0) {
    return -1;
  }

  if (print_details) {
    printf("Device code: 0x%02x 0x%02x 0x%02x\n",
      data_frame[2], data_frame[3], data_frame[4]);
    printf("Device name: %c%c%c%c%c%c%c%c%c%c\n",
      data_frame[5], data_frame[6], data_frame[7], data_frame[8], data_frame[9],
      data_frame[10], data_frame[11], data_frame[12], data_frame[13], data_frame[14]);
    printf("Code flash ROM last address: 0x%02x%02x%02x\n",
      data_frame[17], data_frame[16], data_frame[15]);
    printf("Data flash ROM last address: 0x%02x%02x%02x\n",
      data_frame[20], data_frame[19], data_frame[18]);
    printf("Firmware version: %d.%d%d\n",
      data_frame[21], data_frame[22], data_frame[23]);
  }

  return 0;
}



static int command_block_erase(int tty_fd, int block_no)
{
  int cmd_frame_len, status_frame_len, start_address;
  unsigned char cmd_frame[16];
  unsigned char status_frame[8];

  start_address = block_no * BLOCK_SIZE;

  cmd_frame_len = 0;
  cmd_frame[cmd_frame_len++] = 0x01; /* Command Frame Header */
  cmd_frame[cmd_frame_len++] = 0x04; /* Command Information Length */
  cmd_frame[cmd_frame_len++] = RL78_COMMAND_BLOCK_ERASE;
  cmd_frame[cmd_frame_len++] = (start_address & 0xff);         /* Start Address, Low */
  cmd_frame[cmd_frame_len++] = ((start_address >> 8) & 0xff);  /* Start Address, Middle */
  cmd_frame[cmd_frame_len++] = ((start_address >> 16) & 0xff); /* Start Address, High */
  cmd_frame[cmd_frame_len++] = generate_checksum(&cmd_frame[2], cmd_frame[1]);
  cmd_frame[cmd_frame_len++] = 0x03; /* Command Frame Footer */

  if (frame_send(tty_fd, cmd_frame, cmd_frame_len) < 0) {
    return -1;
  }

  if ((status_frame_len = frame_recv(tty_fd, status_frame, sizeof(status_frame))) < 0) {
    return -1;
  }

  if (status_frame[2] != RL78_STATUS_NORMAL_ACK) {
    fprintf(stderr, "command_block_erase() failed: %s (0x%02x)\n",
      rl78_status_text(status_frame[2]), status_frame[2]);
    return -1;
  }

  return 0;
}



static int command_programming(int tty_fd, int first_block_no, unsigned char *block_data, int no_of_blocks)
{
  int cmd_frame_len, status_frame_len, data_frame_len, start_address, end_address, offset;
  unsigned char cmd_frame[16];
  unsigned char status_frame[8];
  unsigned char data_frame[264];

  start_address = first_block_no * BLOCK_SIZE;
  end_address = ((first_block_no + no_of_blocks) * BLOCK_SIZE) - 1;

  cmd_frame_len = 0;
  cmd_frame[cmd_frame_len++] = 0x01; /* Command Frame Header */
  cmd_frame[cmd_frame_len++] = 0x07; /* Command Information Length */
  cmd_frame[cmd_frame_len++] = RL78_COMMAND_PROGRAMMING;
  cmd_frame[cmd_frame_len++] = (start_address & 0xff);         /* Start Address, Low */
  cmd_frame[cmd_frame_len++] = ((start_address >> 8) & 0xff);  /* Start Address, Middle */
  cmd_frame[cmd_frame_len++] = ((start_address >> 16) & 0xff); /* Start Address, High */
  cmd_frame[cmd_frame_len++] = (end_address & 0xff);         /* End Address, Low */
  cmd_frame[cmd_frame_len++] = ((end_address >> 8) & 0xff);  /* End Address, Middle */
  cmd_frame[cmd_frame_len++] = ((end_address >> 16) & 0xff); /* End Address, High */
  cmd_frame[cmd_frame_len++] = generate_checksum(&cmd_frame[2], cmd_frame[1]);
  cmd_frame[cmd_frame_len++] = 0x03; /* Command Frame Footer */

  if (frame_send(tty_fd, cmd_frame, cmd_frame_len) < 0) {
    return -1;
  }

  if (frame_recv(tty_fd, status_frame, sizeof(status_frame)) < 0) {
    return -1;
  }

  if (status_frame[2] != RL78_STATUS_NORMAL_ACK) {
    fprintf(stderr, "command_programming() failed: %s (0x%02x)\n",
      rl78_status_text(status_frame[2]), status_frame[2]);
    return -1;
  }

  for (offset = 0; offset < (no_of_blocks * BLOCK_SIZE); offset += 256) {

    data_frame_len = 0;
    data_frame[data_frame_len++] = 0x02; /* Data Frame Header */
    data_frame[data_frame_len++] = 0x00; /* Data Length, Always 0x00 = 256 Bytes */
    memcpy(&data_frame[2], &block_data[offset], 256);
    data_frame_len += 256;
    data_frame[data_frame_len++] = generate_checksum(&data_frame[2], data_frame[1]);

    if ((offset + 256) >= (no_of_blocks * BLOCK_SIZE)) {
      data_frame[data_frame_len++] = 0x03; /* Data Frame Footer, End of Data */
    } else {
      data_frame[data_frame_len++] = 0x17; /* Data Frame Footer, More To Be Sent */
    }

    if (frame_send(tty_fd, data_frame, data_frame_len) < 0) {
      return -1;
    }

    if ((status_frame_len = frame_recv(tty_fd, status_frame, sizeof(status_frame))) < 0) {
      return -1;
    }

    if (status_frame[2] != RL78_STATUS_NORMAL_ACK) {
      fprintf(stderr, "command_programming() failed: %s (0x%02x)\n",
        rl78_status_text(status_frame[2]), status_frame[2]);
      return -1;
    }
  }

  if ((status_frame_len = frame_recv(tty_fd, status_frame, sizeof(status_frame))) < 0) {
    return -1;
  }

  if (status_frame[2] != RL78_STATUS_NORMAL_ACK) {
    fprintf(stderr, "command_programming() failed: %s (0x%02x)\n",
      rl78_status_text(status_frame[2]), status_frame[2]);
    return -1;
  }

  return 0;
}



static int command_checksum(int tty_fd, int first_block_no, int no_of_blocks)
{
  int cmd_frame_len, status_frame_len, data_frame_len, start_address, end_address;
  unsigned char cmd_frame[16];
  unsigned char status_frame[8];
  unsigned char data_frame[8];

  start_address = first_block_no * BLOCK_SIZE;
  end_address = ((first_block_no + no_of_blocks) * BLOCK_SIZE) - 1;

  cmd_frame_len = 0;
  cmd_frame[cmd_frame_len++] = 0x01; /* Command Frame Header */
  cmd_frame[cmd_frame_len++] = 0x07; /* Command Information Length */
  cmd_frame[cmd_frame_len++] = RL78_COMMAND_CHECKSUM;
  cmd_frame[cmd_frame_len++] = (start_address & 0xff);         /* Start Address, Low */
  cmd_frame[cmd_frame_len++] = ((start_address >> 8) & 0xff);  /* Start Address, Middle */
  cmd_frame[cmd_frame_len++] = ((start_address >> 16) & 0xff); /* Start Address, High */
  cmd_frame[cmd_frame_len++] = (end_address & 0xff);         /* End Address, Low */
  cmd_frame[cmd_frame_len++] = ((end_address >> 8) & 0xff);  /* End Address, Middle */
  cmd_frame[cmd_frame_len++] = ((end_address >> 16) & 0xff); /* End Address, High */
  cmd_frame[cmd_frame_len++] = generate_checksum(&cmd_frame[2], cmd_frame[1]);
  cmd_frame[cmd_frame_len++] = 0x03; /* Command Frame Footer */

  if (frame_send(tty_fd, cmd_frame, cmd_frame_len) < 0) {
    return -1;
  }

  if ((status_frame_len = frame_recv(tty_fd, status_frame, sizeof(status_frame))) < 0) {
    return -1;
  }

  if (status_frame[2] != RL78_STATUS_NORMAL_ACK) {
    fprintf(stderr, "command_checksum() failed: %s (0x%02x)\n",
      rl78_status_text(status_frame[2]), status_frame[2]);
    return -1;
  }

  if ((data_frame_len = frame_recv(tty_fd, data_frame, sizeof(data_frame))) < 0) {
    return -1;
  }

  return data_frame[2] + (data_frame[3] * 0x100);
}



static int command_verify(int tty_fd, int first_block_no, unsigned char *block_data, int no_of_blocks)
{
  int cmd_frame_len, status_frame_len, data_frame_len, start_address, end_address, offset;
  unsigned char cmd_frame[16];
  unsigned char status_frame[8];
  unsigned char data_frame[264];

  start_address = first_block_no * BLOCK_SIZE;
  end_address = ((first_block_no + no_of_blocks) * BLOCK_SIZE) - 1;

  cmd_frame_len = 0;
  cmd_frame[cmd_frame_len++] = 0x01; /* Command Frame Header */
  cmd_frame[cmd_frame_len++] = 0x07; /* Command Information Length */
  cmd_frame[cmd_frame_len++] = RL78_COMMAND_VERIFY;
  cmd_frame[cmd_frame_len++] = (start_address & 0xff);         /* Start Address, Low */
  cmd_frame[cmd_frame_len++] = ((start_address >> 8) & 0xff);  /* Start Address, Middle */
  cmd_frame[cmd_frame_len++] = ((start_address >> 16) & 0xff); /* Start Address, High */
  cmd_frame[cmd_frame_len++] = (end_address & 0xff);         /* End Address, Low */
  cmd_frame[cmd_frame_len++] = ((end_address >> 8) & 0xff);  /* End Address, Middle */
  cmd_frame[cmd_frame_len++] = ((end_address >> 16) & 0xff); /* End Address, High */
  cmd_frame[cmd_frame_len++] = generate_checksum(&cmd_frame[2], cmd_frame[1]);
  cmd_frame[cmd_frame_len++] = 0x03; /* Command Frame Footer */

  if (frame_send(tty_fd, cmd_frame, cmd_frame_len) < 0) {
    return -1;
  }

  if (frame_recv(tty_fd, status_frame, sizeof(status_frame)) < 0) {
    return -1;
  }

  if (status_frame[2] != RL78_STATUS_NORMAL_ACK) {
    fprintf(stderr, "command_verify() failed: %s (0x%02x)\n",
      rl78_status_text(status_frame[2]), status_frame[2]);
    return -1;
  }

  for (offset = 0; offset < (no_of_blocks * BLOCK_SIZE); offset += 256) {

    data_frame_len = 0;
    data_frame[data_frame_len++] = 0x02; /* Data Frame Header */
    data_frame[data_frame_len++] = 0x00; /* Data Length, Always 0x00 = 256 Bytes */
    memcpy(&data_frame[2], &block_data[offset], 256);
    data_frame_len += 256;
    data_frame[data_frame_len++] = generate_checksum(&data_frame[2], data_frame[1]);

    if ((offset + 256) >= (no_of_blocks * BLOCK_SIZE)) {
      data_frame[data_frame_len++] = 0x03; /* Data Frame Footer, End of Data */
    } else {
      data_frame[data_frame_len++] = 0x17; /* Data Frame Footer, More To Be Sent */
    }

    if (frame_send(tty_fd, data_frame, data_frame_len) < 0) {
      return -1;
    }

    if ((status_frame_len = frame_recv(tty_fd, status_frame, sizeof(status_frame))) < 0) {
      return -1;
    }

    if (status_frame[2] != RL78_STATUS_NORMAL_ACK) {
      fprintf(stderr, "command_verify() failed: %s (0x%02x)\n",
        rl78_status_text(status_frame[2]), status_frame[2]);
      return -1;
    }
    if (status_frame[3] != RL78_STATUS_NORMAL_ACK) {
      fprintf(stderr, "command_verify() failed: %s (0x%02x)\n",
        rl78_status_text(status_frame[3]), status_frame[3]);
      return -1;
    }
  }

  return 0;
}



static int command_block_blank_check(int tty_fd, int first_block_no, int no_of_blocks)
{
  int cmd_frame_len, start_address, end_address;
  unsigned char cmd_frame[16];
  unsigned char status_frame[8];

  start_address = first_block_no * BLOCK_SIZE;
  end_address = ((first_block_no + no_of_blocks) * BLOCK_SIZE) - 1;

  cmd_frame_len = 0;
  cmd_frame[cmd_frame_len++] = 0x01; /* Command Frame Header */
  cmd_frame[cmd_frame_len++] = 0x08; /* Command Information Length */
  cmd_frame[cmd_frame_len++] = RL78_COMMAND_BLOCK_BLANK_CHECK;
  cmd_frame[cmd_frame_len++] = (start_address & 0xff);         /* Start Address, Low */
  cmd_frame[cmd_frame_len++] = ((start_address >> 8) & 0xff);  /* Start Address, Middle */
  cmd_frame[cmd_frame_len++] = ((start_address >> 16) & 0xff); /* Start Address, High */
  cmd_frame[cmd_frame_len++] = (end_address & 0xff);         /* End Address, Low */
  cmd_frame[cmd_frame_len++] = ((end_address >> 8) & 0xff);  /* End Address, Middle */
  cmd_frame[cmd_frame_len++] = ((end_address >> 16) & 0xff); /* End Address, High */
  cmd_frame[cmd_frame_len++] = 0x00; /* Specified Block */
  cmd_frame[cmd_frame_len++] = generate_checksum(&cmd_frame[2], cmd_frame[1]);
  cmd_frame[cmd_frame_len++] = 0x03; /* Command Frame Footer */

  if (frame_send(tty_fd, cmd_frame, cmd_frame_len) < 0) {
    return -1;
  }

  if (frame_recv(tty_fd, status_frame, sizeof(status_frame)) < 0) {
    return -1;
  }

  switch (status_frame[2]) {
  case RL78_STATUS_NORMAL_ACK:
    return 0; /* Blocks are free. */
  case RL78_STATUS_IVERIFY_BLANK_ERROR:
    return 1; /* Blocks are occupied. */
  default:
    fprintf(stderr, "command_block_blank_check() failed: %s (0x%02x)\n",
      rl78_status_text(status_frame[2]), status_frame[2]);
    return -1;
  }
}



static int programmer_init(char *tty_device)
{
  int tty_fd, result;
  struct termios tio;
  unsigned int bits;

  tty_fd = open(tty_device, O_RDWR | O_NOCTTY | O_NONBLOCK);
  if (tty_fd == -1) {
    fprintf(stderr, "open(%s) failed: %s\n", tty_device, strerror(errno));
    return -1;
  }
 
  memset(&tio, '\0', sizeof(tio));
  tio.c_cflag = B115200 | CS8 | CSTOPB;
  tio.c_iflag = IGNPAR;
  tio.c_oflag = 0;
  tio.c_lflag = 0;
  result = tcsetattr(tty_fd, TCSANOW, &tio);
  if (result == -1) {
    fprintf(stderr, "tcsetattr() failed: %s\n", strerror(errno));
    close(tty_fd);
    return -1;
  }

  result = ioctl(tty_fd, TIOCMGET, &bits);
  if (result == -1) {
    fprintf(stderr, "ioctl() failed: %s\n", strerror(errno));
    close(tty_fd);
    return -1;
  }

  /* Set DTR (Reset Signal). */
  bits |= TIOCM_DTR;
  result = ioctl(tty_fd, TIOCMSET, &bits);
  if (result == -1) {
    fprintf(stderr, "ioctl() failed: %s\n", strerror(errno));
    close(tty_fd);
    return -1;
  }

  /* Turn on break. */
  result = ioctl(tty_fd, TIOCSBRK, NULL);
  if (result == -1) {
    fprintf(stderr, "ioctl() failed: %s\n", strerror(errno));
    close(tty_fd);
    return -1;
  }

  tcflush(tty_fd, TCIOFLUSH);

  /* Clear DTR (Reset Signal). */
  bits &= (~TIOCM_DTR);
  result = ioctl(tty_fd, TIOCMSET, &bits);
  if (result == -1) {
    fprintf(stderr, "ioctl() failed: %s\n", strerror(errno));
    close(tty_fd);
    return -1;
  }

  usleep(1000);

  /* Turn off break. */
  result = ioctl(tty_fd, TIOCCBRK, NULL);
  if (result == -1) {
    fprintf(stderr, "ioctl() failed: %s\n", strerror(errno));
    close(tty_fd);
    return -1;
  }

  tcflush(tty_fd, TCIOFLUSH);

  usleep(1000);

  /* Setup Two-wire UART mode. */
  result = write(tty_fd, "\x00", 1);
  if (result == -1) {
    fprintf(stderr, "write() failed: %s\n", strerror(errno));
    close(tty_fd);
    return -1;
  } else if (result == 0) {
    fprintf(stderr, "write() failed: Nothing written\n");
    close(tty_fd);
    return -1;
  }

  usleep(1000);

  tcflush(tty_fd, TCIOFLUSH);

  return tty_fd;
}



static void programmer_shutdown(int tty_fd)
{
  int result;
  unsigned int bits;

  result = ioctl(tty_fd, TIOCMGET, &bits);
  if (result == -1) {
    fprintf(stderr, "ioctl() failed: %s\n", strerror(errno));
  }

  /* Set DTR (Reset Signal). */
  bits |= TIOCM_DTR;
  result = ioctl(tty_fd, TIOCMSET, &bits);
  if (result == -1) {
    fprintf(stderr, "ioctl() failed: %s\n", strerror(errno));
  }

  usleep(1000);

  /* Clear DTR (Reset Signal). */
  bits &= (~TIOCM_DTR);
  result = ioctl(tty_fd, TIOCMSET, &bits);
  if (result == -1) {
    fprintf(stderr, "ioctl() failed: %s\n", strerror(errno));
  }

  close(tty_fd);
}



static void display_help(char *progname)
{
  fprintf(stderr, "Usage: %s <options>\n", progname);
  fprintf(stderr, "Options:\n"
     "  -h          Display this help and exit.\n"
     "  -t          Print TTY/serial traffic debugging info.\n"
     "  -q          Quiet mode, do not print anything.\n"
     "  -v          Verification mode, do not erase and program.\n"
     "  -d DEVICE   Use TTY DEVICE.\n"
     "  -f FILE     Use FILE for programming or verification.\n"
     "  -o OFFSET   Program or verify at block OFFSET instead of 0.\n"
     "\n");
}



int main(int argc, char *argv[])
{
  int c, i, tty_fd, bin_len, block_no, checksum_remote, checksum_local, result;
  FILE *bin_fh;
  unsigned char bin_data[BLOCK_SIZE];

  char *tty_device = NULL;
  char *bin_file   = NULL;
  int mode_verify    = 0;
  int block_offset   = 0;

  while ((c = getopt(argc, argv, "htqvd:f:o:")) != -1) {
    switch (c) {
    case 'h':
      display_help(argv[0]);
      return EXIT_SUCCESS;

    case 't':
      print_traffic = 1;
      break;

    case 'q':
      print_traffic = 0;
      print_details = 0;
      break;

    case 'v':
      mode_verify = 1;
      break;

    case 'd':
      tty_device = optarg;
      break;

    case 'f':
      bin_file = optarg;
      break;

    case 'o':
      block_offset = atoi(optarg);
      break;

    case '?':
    default:
      display_help(argv[0]);
      return EXIT_FAILURE;
    }
  }

  if (tty_device == NULL) {
    fprintf(stderr, "Please specify a TTY!\n");
    display_help(argv[0]);
    return EXIT_FAILURE;
  }

  if (bin_file == NULL) {
    fprintf(stderr, "Please specify a file!\n");
    display_help(argv[0]);
    return EXIT_FAILURE;
  }
 
  tty_fd = programmer_init(tty_device);
  if (tty_fd == -1) {
    return EXIT_FAILURE;
  }

  if (command_baud_rate_set(tty_fd) != 0) {
    programmer_shutdown(tty_fd);
    return EXIT_FAILURE;
  }

  if (command_reset(tty_fd) != 0) {
    programmer_shutdown(tty_fd);
    return EXIT_FAILURE;
  }

  if (command_silicon_signature(tty_fd) != 0) {
    programmer_shutdown(tty_fd);
    return EXIT_FAILURE;
  }

  bin_fh = fopen(bin_file, "rb");
  if (bin_fh == NULL) {
    fprintf(stderr, "fopen(%s) failed: %s\n", bin_file, strerror(errno));
    programmer_shutdown(tty_fd);
    return EXIT_FAILURE;
  }

  checksum_local = 0;
  block_no = block_offset;
  while ((bin_len = fread(bin_data, sizeof(unsigned char), BLOCK_SIZE, bin_fh)) > 0) {
    if (print_details) {
      if (mode_verify == 0) {
        printf("Programming Block #%d (0x%06x -> 0x%06x)\n",
          block_no, (block_no * BLOCK_SIZE), (((block_no + 1) * BLOCK_SIZE) - 1));
      } else {
        printf("Verifying Block #%d (0x%06x -> 0x%06x)\n",
          block_no, (block_no * BLOCK_SIZE), (((block_no + 1) * BLOCK_SIZE) - 1));
      }
    }

    /* Pad remaining data with 0xff */
    while (bin_len < BLOCK_SIZE) {
      bin_data[bin_len++] = 0xff;
    }

    for (i = 0; i < BLOCK_SIZE; i++) {
      checksum_local -= bin_data[i];
    }

    if (mode_verify == 0) {
      result = command_block_blank_check(tty_fd, block_no, 1);
      if (result == -1) {
        fclose(bin_fh);
        programmer_shutdown(tty_fd);
        return EXIT_FAILURE;

      } else if (result == 1) {
        if (command_block_erase(tty_fd, block_no) != 0) {
          fclose(bin_fh);
          programmer_shutdown(tty_fd);
          return EXIT_FAILURE;
        }
      }

      if (command_programming(tty_fd, block_no, bin_data, 1) != 0) {
        fclose(bin_fh);
        programmer_shutdown(tty_fd);
        return EXIT_FAILURE;
      }
    }
    
    if (command_verify(tty_fd, block_no, bin_data, 1) != 0) {
      fclose(bin_fh);
      programmer_shutdown(tty_fd);
      return EXIT_FAILURE;
    }

    block_no += 1;
  }
  fclose(bin_fh);

  checksum_local = checksum_local & 0xffff;
  checksum_remote = command_checksum(tty_fd, block_offset, (block_no - block_offset));

  if (print_details) {
    printf("Checksum Local : 0x%04x\n", checksum_local);
    printf("Checksum Remote: 0x%04x\n", checksum_remote);
  }

  programmer_shutdown(tty_fd);
  return EXIT_SUCCESS;
}



