#include "xmit.h"
#include "parking_data.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MARK_START "$$$$$$$$"
#define MARK_START_DATA "DDDDDDDD"
#define MARK_END "@@@@@@@@"
#define NL "\n"
#define NL3 NL NL NL

#define MAX_READ_SIZE 50
#define NL_CHAR 13

#define IMAGE_YUV_SIZE (320 * 240 * 2)

int xmit_data() {

  printf(NL3 "%s", MARK_START_DATA);
  parking_data_print_cfg_str();
  return 0;
}

int xmit_image(void *buff, uint16_t width, uint16_t height) {

  uint32_t n_pixels;
  uint32_t x, y;
  uint16_t *source_buff = buff;
  if (!buff) {
    return -1;
  }
  n_pixels = width * height;

  /* Been having alot of issues when transferring the image in binary mode.
   * ASCII mode seem to work very well. */
  y = 0;
  printf(NL3 MARK_START ",,%06d,%06d,%06d,\n", n_pixels, width, height);
  for (x = 0; x < n_pixels; x++) {
    y = y + printf("% 4x\n", source_buff[x]);
  }

  usleep(10000);

  return y;
}

static const char *cmd_img_req[] = {"REQ_IMG", "REQ_SOMETHING"};

image_cmd_t xmit_read_request() {
  int res;
  char c;
  image_cmd_t ret = IMAGE_CMD_NONE;
  uint8_t char_cnt;
  char_cnt = 0;
  char read_cmd[MAX_READ_SIZE] = {' '};
  read_cmd[0] = '\0';
  int cmd;

  printf("Press enter to proceed or: \n");
  for (cmd = 0; cmd < (sizeof(cmd_img_req) / sizeof(cmd_img_req[0])); cmd++) {
    printf("* %s\n", cmd_img_req[cmd]);
  }

  do {
    c = getc(stdin);
    if (char_cnt >= MAX_READ_SIZE - 2) {
      break;
    }
    read_cmd[char_cnt] = c;
    printf("%c", c);

    char_cnt++;
  } while (c != NL_CHAR);

  read_cmd[char_cnt] = '\n';

  res = strncmp(cmd_img_req[0], read_cmd, strlen(cmd_img_req[0]));
  if (res == 0) {
    ret = IMAGE_CMD_REQ_IMG;
  }
  printf("Ret: %d %s %s res: %d\n", ret, cmd_img_req[0], read_cmd, res);

  return ret;
}
