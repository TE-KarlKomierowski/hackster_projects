#pragma once

#include "common.h"
#include <nuttx/lcd/lcd.h>
#include <nuttx/nx/nx.h>
#include <nuttx/nx/nxglib.h>
#include <stdint.h>

#define IMAGE_COLOR_GREEN 0x03E0
#define IMAGE_COLOR_BLUE 0x001F
#define IMAGE_COLOR_RED 0xF800

EXTERN_C

typedef struct {
  uint16_t x;
  uint16_t y;
} pixel_coord_t;

typedef struct {
  pixel_coord_t upper_left_px;
  uint16_t width;
  uint16_t height;
} rectangle_t;

typedef struct {
  uint16_t width;
  uint16_t height;
} image_size_t;

typedef struct {
  uint16_t r;
  uint16_t g;
  uint16_t b;
} image_diff_px_counter_t;

#define N_HISTOGRAM_BINS 32

typedef struct {
  int16_t red[N_HISTOGRAM_BINS];
  int16_t green[N_HISTOGRAM_BINS];
  int16_t blue[N_HISTOGRAM_BINS];
} histogram_t;

int image_rectangle_abs(FAR const void *image, rectangle_t *rectangle, uint8_t thickness, uint16_t color, bool filled);

int image_line_v(FAR const void *image, pixel_coord_t *start, uint16_t len, uint8_t thickness, uint16_t color);
int image_line_h(FAR const void *image, pixel_coord_t *start, uint16_t len, uint8_t thickness, uint16_t color);

int image_init(uint16_t width, uint16_t height);
int image_get_histogram_from_rectangle(FAR const void *image, rectangle_t *rectangle, histogram_t *h);
void image_print_histogram(histogram_t *h, uint16_t index);

void image_set_mem_from_rectangle(FAR const void *image, const void *rect_buff, rectangle_t *rectangle);
void *image_get_mem_from_rectangle(FAR const void *image, rectangle_t *rectangle);
void image_print_vt100(void);
void image_print_vt100_clrscr(void);
image_diff_px_counter_t image_diff_rectangle(void *img_buff_target, const void *img_buff_src, rectangle_t *rectangle);
int image_subtract_diff_rectangle(void *img_buff_target, void *img_buff_src, rectangle_t *rectangle);

void *image_convolution(uint8_t filter_index, void *img_buff_src, rectangle_t *rectangle);

EXTERN_C_END
