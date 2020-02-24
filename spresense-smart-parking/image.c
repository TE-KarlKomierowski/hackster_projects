#include "image.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ESC_CHAR 0x1B

static image_size_t dimension;

typedef enum {
  BASE_RED = 0,
  BASE_GREEN,
  BASE_BLUE,
} base_color_t;

static inline uint8_t image_get_px_val(uint16_t px, base_color_t base_color) {
  uint8_t val = 0;
  /* 0xF800 */
  switch (base_color) {
  case BASE_RED:
    val = (px >> (5 + 6) & 0x001F);

    break;
  case BASE_GREEN:
    val = (px >> (5) & 0x003F); /* Skip the LSb*/

    break;
  case BASE_BLUE:
    val = (px & 0x001F);
    break;
  default:
    val = 0;
  }

  // printf("%d: 0x%02x 0x%02x \n", base_color, val, px);
  return val;
}

int image_init(uint16_t width, uint16_t height) {

  dimension.height = height;
  dimension.width = width;
  return 0;
}

int image_line_h(FAR const void *image, pixel_coord_t *start, uint16_t len, uint8_t thickness, uint16_t color) {
  uint16_t x;
  uint32_t y;
  uint16_t linew;
  uint16_t *px;
  if (!image) {
    return -1;
  }
  px = (uint16_t *)image;
  len += thickness;

  for (linew = 0; linew < thickness; linew++) {
    y = ((linew + start->y) * dimension.width) + start->x;

    if ((start->x + len) >= dimension.width) {
      len = (dimension.width - 1) - start->x;
    }

    for (x = 0; x < len; x++) {
      px[y + x] = color;
    }

    // printf("H Line x:% 3d y:% 3d, len: % 3d, (w, h % 3d,% 3d) (x, y % 3d,% 3d)  \n", start->x, start->y, len,
    // dimension.width, dimension.height, x, y);
  }

  return 0;
}

int image_line_v(FAR const void *image, pixel_coord_t *start, uint16_t len, uint8_t thickness, uint16_t color) {
  uint32_t y, start_pt;

  uint16_t linew;

  uint16_t *px;
  if (!image) {
    return -1;
  }
  len += thickness;
  px = (uint16_t *)image;

  for (linew = 0; linew < thickness; linew++) {

    start_pt = (start->y * dimension.width) + start->x + linew;

    for (y = 0; y < len; y++) {
      px[start_pt + (y * dimension.width)] = color;
    }
  }
  // printf("V Line x:%d y:%d, len: %d\n", start->x, start->y, len);

  return 0;
}

int image_rectangle_abs(FAR const void *image, rectangle_t *rectangle, uint8_t thickness, uint16_t color, bool filled) {

  uint16_t y;

  static pixel_coord_t start;
  uint32_t px_x, px_y;
  uint16_t *buff;

  if (!image) {
    return -1;
  }

  memcpy(&start, &rectangle->upper_left_px, sizeof(pixel_coord_t));

  image_line_h(image, &start, rectangle->width, thickness, color);
  image_line_v(image, &start, rectangle->height, thickness, color);

  y = start.y;
  start.y = start.y + rectangle->height;
  image_line_h(image, &start, rectangle->width, thickness, color);

  start.y = y;
  start.x = start.x + rectangle->width;
  image_line_v(image, &start, rectangle->height, thickness, color);

  if (filled) {
    buff = ((uint16_t *)image)
           + (dimension.width * (rectangle->upper_left_px.y + thickness) + rectangle->upper_left_px.x + thickness);

    for (px_y = 0; px_y < rectangle->height - thickness; px_y++) {
      for (px_x = 0; px_x < rectangle->width - thickness; px_x++) {

        buff[px_y * dimension.width + px_x] = buff[px_y * dimension.width + px_x] & color;
      }
    }
  }

  return 0;
}

void image_set_mem_from_rectangle(FAR const void *image, const void *rect_buff, rectangle_t *rectangle) {
  uint16_t *pt;
  uint16_t *src;
  uint32_t y, start;

  start = (dimension.width * rectangle->upper_left_px.y) + rectangle->upper_left_px.x;

  src = (uint16_t *)image;
  pt = (uint16_t *)rect_buff;

  for (y = 0; y < rectangle->height; y++) {
    memcpy(&src[start + (y * dimension.width)], &pt[rectangle->width * y], sizeof(uint16_t) * rectangle->width);
  }
}

void *image_get_mem_from_rectangle(FAR const void *image, rectangle_t *rectangle) {
  uint16_t *pt;
  const uint16_t *src;
  uint32_t y, start;

  start = (dimension.width * rectangle->upper_left_px.y) + rectangle->upper_left_px.x;

  src = image;

  pt = malloc(rectangle->width * rectangle->height * 2);
  // printf("%s %p size: %d x %d\n", __func__, pt, rectangle->width, rectangle->height);
  for (y = 0; y < rectangle->height; y++) {
    // memset(&src[start + (y * dimension.width)], IMAGE_COLOR_GREEN, sizeof(uint16_t) * rectangle->width);
    memcpy(&pt[rectangle->width * y], &src[start + (y * dimension.width)], sizeof(uint16_t) * rectangle->width);
  }
  return pt;
}

int image_get_histogram_from_rectangle(FAR const void *image, rectangle_t *rectangle, histogram_t *h) {
  const uint16_t *pt;
  uint16_t x, y;
  uint8_t val;
  uint16_t px;

  if (!image) {
    return -1;
  }
  if (!rectangle) {
    return -1;
  }
  if (!h) {
    return -1;
  }

  // pt = image_get_mem_from_rectangle(image, rectangle);
  pt = image;
  printf("%s BuffPTR %p\n", __func__, pt);

  for (y = 0; y < rectangle->height; y++) {
    for (x = 0; x < rectangle->width; x++) {
      // printf("%04x ", pt[y * rectangle->width + x]);
      px = pt[y * rectangle->width + x];
      val = image_get_px_val(px, BASE_RED);
      h->red[val]++;
      val = image_get_px_val(px, BASE_GREEN);
      h->green[val]++;
      val = image_get_px_val(px, BASE_BLUE);
      h->blue[val]++;
    }
  }
  if (0) {

    for (y = 0; y < rectangle->height; y++) {
      for (x = 0; x < rectangle->width; x++) {
        printf("% 4x ", pt[y * rectangle->width + x]);
      }
      printf("\n");
    }
  }
  // free(pt);

  return 0;
}
#define THRESHOLD 8
#define N_DIFF_PX (size / 3)

image_diff_px_counter_t image_diff_rectangle(void *img_buff_target, const void *img_buff_src, rectangle_t *rectangle) {
  uint16_t px, size;
  const uint16_t *src;
  uint16_t *target, *diff_buff;

  image_diff_px_counter_t px_cnt = {0};

  src = img_buff_src;

  target = (uint16_t *)img_buff_target;
  diff_buff = (uint16_t *)img_buff_target;

  size = rectangle->width * rectangle->height;
  diff_buff = malloc(size * sizeof(uint32_t));

  for (px = 0; px < size; px++) {
    int16_t r, g, b;

    uint16_t px_col;

    r = image_get_px_val(src[px], BASE_RED) - (int16_t)image_get_px_val(target[px], BASE_RED);
    g = image_get_px_val(src[px], BASE_GREEN) - (int16_t)image_get_px_val(target[px], BASE_GREEN);
    b = image_get_px_val(src[px], BASE_BLUE) - (int16_t)image_get_px_val(target[px], BASE_BLUE);

    r = abs(r);
    g = abs(g);
    b = abs(b);

    if ((r) < THRESHOLD) {
      r = 0;
    } else {
      px_cnt.r++;
    }
    if ((g) < THRESHOLD) {
      g = 0;
    } else {
      px_cnt.g++;
    }
    if ((b) < THRESHOLD) {
      b = 0;
    } else {
      px_cnt.b++;
    }

    px_col = (r << (5 + 6)) | (g << 5) | b;
    // target[px] = px_col;
    diff_buff[px] = px_col;
  }

  if (0) {
    uint16_t x, y;
    for (y = 0; y < rectangle->height; y++) {
      for (x = 0; x < rectangle->width; x++) {
        printf("% 4x ", diff_buff[y * rectangle->width + x]);
      }
      printf("\n");
    }
  }
  free(diff_buff);

  return px_cnt;
}

int image_subtract_diff_rectangle(void *img_buff_target, void *img_buff_src, rectangle_t *rectangle) {
  uint16_t px, size;
  uint16_t *src;
  uint16_t *target;

  src = img_buff_src;
  target = (uint16_t *)img_buff_target;

  size = rectangle->width * rectangle->height;

  for (px = 0; px < size; px++) {
    int16_t diff_r, diff_g, diff_b;
    uint8_t src_r, src_g, src_b;

    uint16_t px_col;

    src_r = image_get_px_val(src[px], BASE_RED);
    src_g = image_get_px_val(src[px], BASE_GREEN);
    src_b = image_get_px_val(src[px], BASE_BLUE);

    diff_r = (int16_t)image_get_px_val(target[px], BASE_RED) - src_r;
    diff_g = (int16_t)image_get_px_val(target[px], BASE_GREEN) - src_g;
    diff_b = (int16_t)image_get_px_val(target[px], BASE_BLUE) - src_b;

    if (abs(diff_r) < THRESHOLD + 2) {
      diff_r = 0;
    } else {
      (diff_r > 0) ? diff_r-- : diff_r++;
    }

    if (abs(diff_g) < THRESHOLD + 2) {
      diff_g = 0;
    } else {
      (diff_g > 0) ? diff_g-- : diff_g++;
    }

    if (abs(diff_b) < THRESHOLD + 2) {
      diff_b = 0;
    } else {
      (diff_b > 0) ? diff_b-- : diff_b++;
    }
#if 1
    diff_r = abs((src_r + diff_r));
    diff_g = abs((src_g + diff_g));
    diff_b = abs((src_b + diff_b));
#endif
    px_col = (diff_r << (5 + 6)) | (diff_g << 5) | diff_b;

    src[px] = px_col;
  }

  return 0;
}

void image_print_vt100() {
  printf("%c[?25h", ESC_CHAR); /* Hide cursor*/
  printf("%c[0;0H", ESC_CHAR);
  printf("%c[?25l", ESC_CHAR);
  // printf("%c[J", ESC_CHAR);
}

void image_print_vt100_clrscr() { printf("%c[J", ESC_CHAR); }

void image_print_histogram(histogram_t *h, uint16_t index) {
  uint16_t x, sum;

  sum = 0;
  printf("%2d ", index);

  for (x = 0; x < N_HISTOGRAM_BINS; x++) {
    printf("%- 3d ", x);
  }
  printf("\nR: ");
  for (x = 0; x < N_HISTOGRAM_BINS; x++) {
    printf("%- 4d", h->red[x]);
    sum = sum + h->red[x];
  }

  printf("\nG: ", sum);
  for (x = 0; x < N_HISTOGRAM_BINS; x++) {
    printf("%- 4d", h->green[x]);
    sum = sum + h->green[x];
  }

  printf("\nB: ");
  for (x = 0; x < N_HISTOGRAM_BINS; x++) {
    printf("%- 4d", h->blue[x]);
    sum = sum + h->blue[x];
  }
  printf("\n");
}

typedef struct {
  uint8_t dimension;
  uint8_t divisor;
  uint8_t threshold;
  int8_t kernel[];
} kernel_t;

static const kernel_t kernel_edge
    = {.dimension = 3, .threshold = 7, .divisor = 1, .kernel = {0, 1, 0, 1, -4, 1, 0, 1, 0}};
static const kernel_t kernel_blur = {.dimension = 3, .divisor = 100, .kernel = {0, 10, 0, 10, 10, 10, 0, 10, 0}};
static const kernel_t kernel_blur2
    = {.dimension = 3, .threshold = 29, .divisor = 3, .kernel = {0, 0, 0, 1, 1, 1, 0, 0, 0}};
static const kernel_t kernel_edge2 = {.dimension = 5,
                                      .kernel = {
                                          0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 1, 1, -8, 1, 1, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0,
                                      }};
static const kernel_t *filters[] = {&kernel_edge, &kernel_edge2, &kernel_blur, &kernel_blur2};

inline static uint16_t kernel_sum(kernel_t *kernel, uint16_t px) {
  uint16_t x, y;
  int16_t sum;
  uint8_t n;
  sum = 0;

#if 0
	for (y = 0; y < kernel->dimension; y++) {
		for (x = 0; x < kernel->dimension; x++) {
			sum = px * kernel->kernel[x + y * kernel->dimension];
		}
	}
#endif
  n = kernel->dimension * kernel->dimension;
  for (y = 0; y < n; y++) {
    sum = sum + px * kernel->kernel[y];
  }

  return sum;
}

void *image_convolution(uint8_t filter_index, void *img_buff_src, rectangle_t *rectangle) {
  uint16_t src_x, src_y, kern_x, kern_y;
  uint16_t *buff;
  uint8_t *result;
  uint8_t edge_offset;
  uint16_t px[3];
  const kernel_t *kernel;
  kernel = filters[filter_index];
  int16_t sum[3];
  uint16_t src_px;

  buff = (uint16_t *)img_buff_src;

  edge_offset = (kernel->dimension - 1) / 2;

  result = malloc(rectangle->height * rectangle->width);
  if (!result) {
    printf("Malloc failed for result\n");
    return NULL;
  }

  for (src_y = edge_offset; src_y < (rectangle->height - edge_offset); src_y++) {
    for (src_x = edge_offset; src_x < (rectangle->width - edge_offset); src_x++) {

      sum[BASE_RED] = 0;
      sum[BASE_GREEN] = 0;
      sum[BASE_BLUE] = 0;

      src_px = src_y * rectangle->width + src_x;

      for (kern_y = 0; kern_y < kernel->dimension; kern_y++) {
        for (kern_x = 0; kern_x < kernel->dimension; kern_x++) {

          px[BASE_RED] = image_get_px_val(buff[src_px + (rectangle->width * (kern_y - 1)) + (kern_x - 1)], BASE_RED);
          px[BASE_GREEN]
              = image_get_px_val(buff[src_px + (rectangle->width * (kern_y - 1)) + (kern_x - 1)], BASE_GREEN);
          px[BASE_BLUE] = image_get_px_val(buff[src_px + (rectangle->width * (kern_y - 1)) + (kern_x - 1)], BASE_BLUE);

          sum[BASE_RED] = sum[BASE_RED]
                          + (px[BASE_RED] * kernel->kernel[(kern_y * kernel->dimension) + kern_x] / kernel->divisor);
          sum[BASE_GREEN]
              = sum[BASE_GREEN]
                + (px[BASE_GREEN] * kernel->kernel[(kern_y * kernel->dimension) + kern_x] / kernel->divisor);
          sum[BASE_BLUE] = sum[BASE_BLUE]
                           + (px[BASE_BLUE] * kernel->kernel[(kern_y * kernel->dimension) + kern_x] / kernel->divisor);
        }
      }

      base_color_t c;
      for (c = BASE_RED; c <= BASE_BLUE; c++) {
        if (sum[c] < 0) {
          sum[c] = -sum[c];
        }
        if (sum[c] > 31) {
          sum[c] = 31;
        }
        if (sum[c] > kernel->threshold) {
          // buff[src_px] = (buff[src_px]) | IMAGE_COLOR_RED;
          result[src_px] = 1;
          break;
        } else {
          result[src_px] = 0;
        }
      }
    }
  }
  for (kern_x = 0; kern_x <= (rectangle->width * rectangle->height); kern_x++) {
    if (result[kern_x]) {

      buff[kern_x] = 0xffff;
    } else {

      buff[kern_x] = 0x0;
    }
  }
  free(result);
  return result;
}
