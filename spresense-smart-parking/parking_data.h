#pragma once
#include "common.h"
#include "image.h"

EXTERN_C

#define PARKING_STATES                                                                                                 \
  M(NONE)                                                                                                              \
  M(INIT)                                                                                                              \
  M(FREE)                                                                                                              \
  M(ENTERING)                                                                                                          \
  M(OCCUPIED)                                                                                                          \
  M(LEAVING)

#define M(C) PARKING_STATE_##C,

typedef enum { PARKING_STATES PARKING_STATE_END } parking_state_t;

#undef M
#define M(C) #C,

void parking_data_init(void);
char *parking_data_state_tostr(parking_state_t state);
void parking_data_outline_spaces(FAR const void *image);
rectangle_t *parking_data_get_rectangle(uint8_t space);
void parking_data_print_cfg_str(void);
uint16_t parking_data_get_nparkings(void);
int parking_data_add_hist(uint8_t space, histogram_t *hist);
int parking_data_is_occupied(uint8_t space, histogram_t *hist);
int parking_data_set_space_img(uint8_t space, const void *imgdata);
void *parking_data_get_space_img(uint8_t space);
parking_state_t parking_data_occupied_img(uint16_t space, image_diff_px_counter_t *px_cnt);

EXTERN_C_END
