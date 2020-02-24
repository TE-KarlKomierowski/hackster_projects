#include "parking_data.h"
#include "image.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define N_SPACES 23
#define N_DIFF_PX (size / 3)
#define STATE_TRANSITION_COUNT 20

#define RECT_WIDTH 18
#define RECT_HEIGTH 30

#define SWIPE_FROM -5
#define SWIPE_TO 5

#define LINE_WIDTH 1

typedef struct {
  uint8_t number;
  union {
    pixel_coord_t coords[2];
    rectangle_t rect;
  };
  histogram_t init_hist;
  parking_state_t state;
  uint8_t state_counter;

  uint16_t *init_img_buff;
  image_diff_px_counter_t px_cnt;

} parking_space_t;

static char *parking_states_str[] = {PARKING_STATES};

typedef struct {
  int spaces;
  parking_space_t parking[N_SPACES];

} parking_lot_t;

char *parking_data_state_tostr(parking_state_t state) {
  if (state >= 0 && state < PARKING_STATE_END) {
    return parking_states_str[state];
  }
  return NULL;
}

// clang-format off
static parking_lot_t lot = {.spaces = N_SPACES,
                            .parking = {
#undef WOODEN_RIG
#ifdef WOODEN_RIG
		{ .number =  15, .coords = { { 179, 108},{ 197, 138} } }, /* Autogen by python recv.py */
		{ .number =  16, .coords = { { 207, 113},{ 225, 143} } }, /* Autogen by python recv.py */
		{ .number =  18, .coords = { { 261, 111},{ 279, 141} } }, /* Autogen by python recv.py */
		{ .number =  17, .coords = { { 236, 112},{ 254, 142} } }, /* Autogen by python recv.py */
		{ .number =  19, .coords = { {  30, 194},{  60, 212} } }, /* Autogen by python recv.py */
		{ .number =  21, .coords = { { 149, 191},{ 179, 209} } }, /* Autogen by python recv.py */
		{ .number =  20, .coords = { {  89, 193},{ 119, 211} } }, /* Autogen by python recv.py */
		{ .number =  22, .coords = { { 204, 191},{ 234, 209} } }, /* Autogen by python recv.py */
		{ .number =   1, .coords = { {  53,  51},{  71,  81} } }, /* Autogen by python recv.py */
		{ .number =  23, .coords = { { 261, 191},{ 291, 209} } }, /* Autogen by python recv.py */
		{ .number =   2, .coords = { {  79,  51},{  97,  81} } }, /* Autogen by python recv.py */
		{ .number =   3, .coords = { { 102,  51},{ 120,  81} } }, /* Autogen by python recv.py */
		{ .number =   4, .coords = { { 128,  51},{ 146,  81} } }, /* Autogen by python recv.py */
		{ .number =   5, .coords = { { 151,  51},{ 169,  81} } }, /* Autogen by python recv.py */
		{ .number =   6, .coords = { { 175,  51},{ 193,  81} } }, /* Autogen by python recv.py */
		{ .number =   7, .coords = { { 200,  50},{ 218,  80} } }, /* Autogen by python recv.py */
		{ .number =   8, .coords = { { 224,  53},{ 242,  83} } }, /* Autogen by python recv.py */
		{ .number =   9, .coords = { { 249,  49},{ 267,  79} } }, /* Autogen by python recv.py */
		{ .number =  10, .coords = { {  40, 112},{  58, 142} } }, /* Autogen by python recv.py */
		{ .number =  11, .coords = { {  68, 111},{  86, 141} } }, /* Autogen by python recv.py */
		{ .number =  12, .coords = { {  96, 111},{ 114, 141} } }, /* Autogen by python recv.py */
		{ .number =  13, .coords = { { 125, 108},{ 143, 138} } }, /* Autogen by python recv.py */
		{ .number =  14, .coords = { { 150, 111},{ 168, 141} } }, /* Autogen by python recv.py */
#else
		{ .number =   6, .coords = { { 191,  18}, {214,     53   }                         } }, /* Autogen by python recv.py */
		{ .number =   7, .coords = { { 225,  20}, {248,     55   }                         } }, /* Autogen by python recv.py */
		{ .number =   9, .coords = { { 291,  20}, {314,     55   }                         } }, /* Autogen by python recv.py */
		{ .number =   8, .coords = { { 259,  19}, {282,     54   }                         } }, /* Autogen by python recv.py */
		{ .number =  10, .coords = { {  18, 111}, {41,     146   }                         } }, /* Autogen by python recv.py */
		{ .number =  12, .coords = { {  86, 110}, {109,     145  }                         } }, /* Autogen by python recv.py */
		{ .number =  11, .coords = { {  51, 111}, {74,     146   }                         } }, /* Autogen by python recv.py */
		{ .number =  13, .coords = { { 119, 108}, {142,     143  }                         } }, /* Autogen by python recv.py */
		{ .number =  15, .coords = { { 190, 107}, {213,     142  }                         } }, /* Autogen by python recv.py */
		{ .number =  14, .coords = { { 152, 107}, {175,     142  }                         } }, /* Autogen by python recv.py */
		{ .number =  16, .coords = { { 224, 109}, {247,     144  }                         } }, /* Autogen by python recv.py */
		{ .number =  18, .coords = { { 295, 110}, {318,     145  }                         } }, /* Autogen by python recv.py */
		{ .number =  17, .coords = { { 262, 113}, {285,     148  }                         } }, /* Autogen by python recv.py */
		{ .number =  19, .coords = { {  19, 211}, {54,     234   }                         } }, /* Autogen by python recv.py */
		{ .number =  21, .coords = { { 147, 211}, {182,     234  }                         } }, /* Autogen by python recv.py */
		{ .number =  20, .coords = { {  80, 210}, {115,     233  }                         } }, /* Autogen by python recv.py */
		{ .number =  22, .coords = { { 216, 211}, {251,     234  }                         } }, /* Autogen by python recv.py */
		{ .number =   1, .coords = { {  20,  15}, {43,     50    }                         } }, /* Autogen by python recv.py */
		{ .number =  23, .coords = { { 275, 213}, {310,     236  }                         } }, /* Autogen by python recv.py */
		{ .number =   2, .coords = { {  58,  15}, {81,     50    }                         } }, /* Autogen by python recv.py */
		{ .number =   3, .coords = { {  91,  16}, {114,     51   }                         } }, /* Autogen by python recv.py */
		{ .number =   4, .coords = { { 124,  18}, {147,     53   }                         } }, /* Autogen by python recv.py */
		{ .number =   5, .coords = { { 157,  17}, {180,     52   }                         } }, /* Autogen by python recv.py */
#endif
                            }};
// clang-format on

void parking_data_outline_spaces(FAR const void *image) {
  uint16_t space;
  bool filled;
  uint8_t linew;

  uint16_t color = 0;

  for (space = 0; space < lot.spaces; space++) {
    linew = LINE_WIDTH;
    //    filled = true;
    filled = false;

    switch (lot.parking[space].state) {
    case PARKING_STATE_NONE:
    case PARKING_STATE_INIT:
      color = IMAGE_COLOR_GREEN | IMAGE_COLOR_RED;
      break;
    case PARKING_STATE_FREE:
      color = IMAGE_COLOR_GREEN;
      linew = LINE_WIDTH + 1;
      filled = false;
      break;
    case PARKING_STATE_ENTERING:
      color = IMAGE_COLOR_BLUE;
      break;
    case PARKING_STATE_OCCUPIED:
      color = IMAGE_COLOR_RED;
      break;
    case PARKING_STATE_LEAVING:
      color = IMAGE_COLOR_GREEN | IMAGE_COLOR_BLUE;
      break;
    case PARKING_STATE_END:
      color = IMAGE_COLOR_GREEN;

      break;
    }

    image_rectangle_abs(image, &lot.parking[space].rect, linew, color, filled);
  }
}

void parking_data_init() {
  rectangle_t rect;
  uint16_t space;

#if 0
  uint16_t xoffset, yoffset;
  xoffset = 0;
  yoffset = 0;


  for (space = 0; space < N_SPACES; space++) {
    if (space == (320 / (RECT_WIDTH))) {
      xoffset = RECT_WIDTH;
      yoffset += RECT_HEIGTH;
    }

    lot.parking[space].coords[0].x = xoffset;
    lot.parking[space].coords[0].y = yoffset;

    lot.parking[space].coords[1].x = lot.parking[space].coords[0].x + RECT_WIDTH;
    lot.parking[space].coords[1].y = lot.parking[space].coords[0].y + RECT_HEIGTH;
    lot.parking[space].number = space + 1;
    xoffset = xoffset + RECT_WIDTH;
  }
#endif

  for (space = 0; space < lot.spaces; space++) {
    rect.upper_left_px.x = lot.parking[space].coords[0].x;
    rect.upper_left_px.y = lot.parking[space].coords[0].y;
    rect.width = lot.parking[space].coords[1].x - lot.parking[space].coords[0].x;
    rect.height = lot.parking[space].coords[1].y - lot.parking[space].coords[0].y;
    printf("% 2d: id: % 2d x: % 3d y: % 3d w: % 3d h: % 3d\n", space, lot.parking[space].number, rect.upper_left_px.x,
           rect.upper_left_px.y, rect.width, rect.height);
    memcpy(&lot.parking[space].rect, &rect, sizeof(rectangle_t));
    lot.parking[space].init_img_buff = NULL;
    lot.parking[space].state_counter = 0;
    lot.parking[space].state = PARKING_STATE_FREE;
  }
}

rectangle_t *parking_data_get_rectangle(uint8_t space) {
  uint16_t sp;
  for (sp = 0; sp < lot.spaces; sp++) {
    if (lot.parking[sp].number == space) {
      return &lot.parking[sp].rect;
    }
  }
  printf("Parking space %d not found!\n", space);
  return NULL;
}

void parking_data_print_cfg_str() {

  uint16_t sp;
  printf(",%02d,\n", lot.spaces);
  for (sp = 0; sp < lot.spaces; sp++) {
    printf("%02d,%02d,%03d,%03d,%03d,%03d,\n", sp, lot.parking[sp].number, lot.parking[sp].rect.upper_left_px.x,
           lot.parking[sp].rect.upper_left_px.y, lot.parking[sp].rect.width, lot.parking[sp].rect.height);
  }
}

uint16_t parking_data_get_nparkings() { return lot.spaces; }

int parking_data_add_hist(uint8_t space, histogram_t *hist) {
  int ret = -1;
  uint16_t sp_i;
  if (!hist) {
    ret = -1;
  } else {
    ret = 0;
  }

  if (!ret) {

    for (sp_i = 0; sp_i < lot.spaces; sp_i++) {
      if (lot.parking[sp_i].number == space) {
        memcpy(&lot.parking[sp_i].init_hist, hist, sizeof(histogram_t));
        image_print_histogram(&lot.parking[sp_i].init_hist, sp_i);
        ret = 0;
        break;
      }
    }

    if (sp_i == lot.spaces) {
      printf("Space not found %d", sp_i);
      ret = -1;
    }
  }

  return ret;
}

int parking_data_is_occupied(uint8_t space, histogram_t *hist) {

  uint8_t i, y;
  uint16_t sp_i;
  int ret = -1;
  histogram_t res;
  int8_t swipe;
  int swipe_index;
  int16_t r_cnt, g_cnt, b_cnt;

  for (sp_i = 0; sp_i < lot.spaces; sp_i++) {
    if (lot.parking[sp_i].number == space) {

      for (swipe = SWIPE_FROM; swipe < SWIPE_TO; swipe++) {

        for (i = 0; i < N_HISTOGRAM_BINS; i++) {
          int16_t r, g, b;

          swipe_index = i + swipe;
          if (swipe_index >= 0 && swipe_index < N_HISTOGRAM_BINS) {
            r = hist->red[i];
            g = hist->green[i];
            b = hist->blue[i];
          } else {
            r = lot.parking[sp_i].init_hist.red[i];
            g = lot.parking[sp_i].init_hist.green[i];
            b = lot.parking[sp_i].init_hist.blue[i];
          }
          res.red[i] = lot.parking[sp_i].init_hist.red[i] - r;
          res.green[i] = lot.parking[sp_i].init_hist.green[i] - g;
          res.blue[i] = lot.parking[sp_i].init_hist.blue[i] - b;
        }
        r_cnt = g_cnt = b_cnt = 0;
        for (y = 0; y < N_HISTOGRAM_BINS; y++) {
          r_cnt += abs(res.red[y]) > 10 ? abs(res.red[y]) : 0;
          g_cnt += abs(res.green[y]) > 10 ? abs(res.green[y]) : 0;
          b_cnt += abs(res.blue[y]) > 10 ? abs(res.blue[y]) : 0;
        }
        // printf("%d, r: % 5d, g: %5 d, b: % 5d\n", swipe, r_cnt, g_cnt, b_cnt);
        // image_print_histogram(&res, space);
      }
      ret = 0;
      break;
    }
  }

  return ret;
}

int parking_data_set_space_img(uint8_t space, const void *imgdata) {
  int ret = -1;
  uint16_t sp_i;

  if (!imgdata) {
    ret = -1;
  } else {
    ret = 0;
  }

  if (!ret) {
    for (sp_i = 0; sp_i < lot.spaces; sp_i++) {
      if (lot.parking[sp_i].number == space) {
        ssize_t s = lot.parking[sp_i].rect.height * lot.parking[sp_i].rect.width * sizeof(uint16_t);

        lot.parking[sp_i].init_img_buff = malloc(s);
        if (lot.parking[sp_i].init_img_buff) {
          memcpy(lot.parking[sp_i].init_img_buff, imgdata, s);
          ret = 0;
          printf("Setting imgmem for %d, size %d\n", space, s);
        } else {
          ret = -1;
        }
        break;
      }
    }
    if (sp_i == lot.spaces) {
      ret = -1;
    }
  }

  return ret;
}

void *parking_data_get_space_img(uint8_t space) {
  uint16_t sp_i;

  for (sp_i = 0; sp_i < lot.spaces; sp_i++) {
    if (lot.parking[sp_i].number == space) {
      return lot.parking[sp_i].init_img_buff;
    }
  }
  return NULL;
}

parking_state_t parking_data_occupied_img(uint16_t space, image_diff_px_counter_t *px_cnt) {
  parking_state_t ret = PARKING_STATE_NONE;
  uint16_t size;
  uint16_t sp_i;

  for (sp_i = 0; sp_i < lot.spaces; sp_i++) {
    if (lot.parking[sp_i].number == space) {
      size = lot.parking[sp_i].rect.width * lot.parking[sp_i].rect.height;

      if (px_cnt->r > N_DIFF_PX || px_cnt->g > N_DIFF_PX || px_cnt->b > N_DIFF_PX) {

        switch (lot.parking[sp_i].state) {
        case PARKING_STATE_FREE:
          lot.parking[sp_i].state = PARKING_STATE_ENTERING;
          lot.parking[sp_i].state_counter = 0;
          break;
        case PARKING_STATE_ENTERING:
          lot.parking[sp_i].state_counter++;
          if (lot.parking[sp_i].state_counter > STATE_TRANSITION_COUNT) {
            lot.parking[sp_i].state = PARKING_STATE_OCCUPIED;
            lot.parking[sp_i].state_counter = 0;
          }
          break;
        case PARKING_STATE_OCCUPIED:
          break;
        case PARKING_STATE_LEAVING:
          lot.parking[sp_i].state = PARKING_STATE_FREE;
          break;
        case PARKING_STATE_NONE:
        case PARKING_STATE_INIT:
        case PARKING_STATE_END:
          break;
        }

      } else {
        switch (lot.parking[sp_i].state) {
        case PARKING_STATE_FREE:
          break;
        case PARKING_STATE_ENTERING:
          lot.parking[sp_i].state_counter = 0;
          lot.parking[sp_i].state = PARKING_STATE_FREE;
          break;
        case PARKING_STATE_OCCUPIED:
          lot.parking[sp_i].state = PARKING_STATE_LEAVING;
          lot.parking[sp_i].state_counter = 0;
          break;
        case PARKING_STATE_LEAVING:
          lot.parking[sp_i].state_counter++;
          if (lot.parking[sp_i].state_counter > STATE_TRANSITION_COUNT) {
            lot.parking[sp_i].state = PARKING_STATE_FREE;
            lot.parking[sp_i].state_counter = 0;
          }
          break;
        case PARKING_STATE_NONE:
        case PARKING_STATE_INIT:
        case PARKING_STATE_END:
          break;
        }
      }

      ret = lot.parking[sp_i].state;
    }
  }
  return ret;
}
