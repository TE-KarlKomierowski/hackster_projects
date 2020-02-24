#pragma once

#include "common.h"
#include <stdint.h>

#define BINARY 0
#define ASCII 1

EXTERN_C

typedef enum {
  IMAGE_CMD_NONE,

  IMAGE_CMD_REQ_IMG,
} image_cmd_t;

int xmit_image(void *buff, uint16_t width, uint16_t height);
image_cmd_t xmit_read_request(void);
int xmit_data(void);

EXTERN_C_END
