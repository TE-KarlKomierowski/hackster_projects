#define CONFIG_CXD56_SPI5
#include <Camera.h>
#include <SPI.h>

#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"
#include "Fonts/FreeMonoBold18pt7b.h"

#define TFT_RST 26 /* For mainboard */
#define TFT_DC  25
#define TFT_CS  24

Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS , TFT_DC , TFT_RST);

#define BAUDRATE    115200

#define CENTER_X_OFFSET 5
#define CENTER_Y_OFFSET 38

#define IMG_WIDTH       320
#define IMG_HEIGHT     240

#define OFFSET_X        IMG_WIDTH/2 - BOX_LARGE_WIDTH /2
#define OFFSET_Y        IMG_HEIGHT / 2 - BOX_LARGE_HEIGHT / 2
#define BOX_LARGE_SIDE 120
#define BOX_SIZE 10

#define BOX_LARGE_WIDTH       BOX_LARGE_SIDE
#define BOX_LARGE_HEIGHT     BOX_LARGE_SIDE

#define BOX_SMALL_SIDE      100
#define BOX_SMALL_WIDTH       100
#define BOX_SMALL_HEIGHT     100

#define BOX_CENTER_WIDTH       5
#define BOX_CENTER_HEIGHT     5

#define SCAN_LINE_PX_WIDTH 3
#define SCAN_LINE_SIDES_N  1

#define BOX_LARGE OFFSET_X, OFFSET_Y, BOX_LARGE_WIDTH, BOX_LARGE_HEIGHT
#define BOX_SMALL OFFSET_X, OFFSET_Y, BOX_SMALL_WIDTH, BOX_SMALL_HEIGHT

#define BOX_CENTER OFFSET_X+BOX_LARGE_WIDTH/2-BOX_CENTER_WIDTH/2, OFFSET_Y + BOX_LARGE_HEIGHT /2-BOX_CENTER_WIDTH/2, BOX_CENTER_WIDTH, BOX_CENTER_HEIGHT

#define SCAN_LENGTH_LARGE 100
#define SCAN_LENGTH_SMALL 55
#define SCAN_LINE_PX_WIDTH 6
#define SCAN_AREA_DISTANCE 50

#define SIDE_LEFT 0
#define SIDE_TOP 1
#define SIDE_RIGHT 2

#define INNER_Y_BASE ((IMG_HEIGHT / 2 + CENTER_Y_OFFSET) + SCAN_AREA_DISTANCE - 30)
#define OUTER_Y_BASE ((IMG_HEIGHT / 2 + CENTER_Y_OFFSET) - SCAN_AREA_DISTANCE)
#define TABLE_SIZE(t) (sizeof(t)/sizeof(t[0]))

typedef struct {
  uint16_t x;
  uint16_t y;
  uint8_t scan_width_px;
  uint16_t scan_length;
  uint8_t *buf;
} buf_t;

typedef enum {
  SCAN_DIRECTION_HORIZONTAL,
  SCAN_DIRECTION_DOWN,
  SCAN_DIRECTION_UP,
} scan_direction_t;

typedef struct {
  uint16_t x;
  uint16_t y;
  uint8_t scan_width_px;
  uint8_t scan_length;
  scan_direction_t  dir;
} scan_area_t;

typedef struct kern_t {
  uint8_t dim_w ;
  uint8_t dim_h ;
  uint8_t denom;
  int8_t kern[SCAN_LINE_PX_WIDTH * SCAN_LINE_PX_WIDTH];
} ;

typedef struct {
  int16_t x;
  int16_t y;
} px_coord_t;

static kern_t vertical_line_detect = {
  .dim_w = SCAN_LINE_PX_WIDTH,
  .dim_h = SCAN_LINE_PX_WIDTH,
  .denom = 1,
  .kern = {
    -1, 2, 2, -1,
    -1, 2, 2, -1,
    -1, 2, 2, -1,
    -1, 2, 2, -1,
  },
};

#define A 25
static kern_t average = {
  .dim_w = 5,
  .dim_h = 5,
  .denom = 5 * 5 ,
  .kern = {
    10 , 10 , 10 , 10 , 10 ,
    10 , 10 , 10 , 10 , 10 ,
    10 , 10 , 10 , 10 , 10 ,
    10 , 10 , 10 , 10 , 10 ,
    10 , 10 , 10 , 10 , 10 ,
  },
};

static kern_t average_1dim = {
  .dim_w = 15,
  .dim_h = 1,
  .denom = 100,
  .kern = {
    10 , 10 , 10 , 10 , 10 , 10 , 10 , 10 , 10 , 10 , 10 , 10 , 10 , 10 , 10 , 10 , 10 , 10 , 10 , 10 , 10 , 10 , 10 , 10 , 10
  },
};

typedef struct {
  float input;
  uint16_t gauge;
  float k;
  float m;
} translate_t;

typedef struct {
  uint16_t low_bound;
  uint16_t high_bound;
  uint16_t color;
  float low_angle;
  float high_angle;

} range_t;

translate_t translate_tbl[] = {
  {.input = -40  , .gauge = 20,   .k = 0, .m = 0},
  {.input = -13  , .gauge = 200,  .k = 0, .m = 0},
  {.input = 49   , .gauge = 400,  .k = 0, .m = 0},
  {.input = 91.5 , .gauge = 600,  .k = 0, .m = 0},
  {.input = 144  , .gauge = 800,  .k = 0, .m = 0},
  {.input = 190  , .gauge = 1000, .k = 0, .m = 0},
  {.input = 216  , .gauge = 1100, .k = 0, .m = 0},

};

static range_t alarms[] = {
  {
    .low_bound = 200,
    .high_bound = 400,
    .color = 0x001F,
    .low_angle = 0.0,
    .high_angle = 130.0,
  },

  {
    .low_bound = 400,
    .high_bound = 800,
    .color = 0x0EC0,
    .low_angle = 0.0,
    .high_angle = 0.0,
  },
  {
    .low_bound = 800,
    .high_bound = 1000,
    .color = 0xF800,
    .low_angle = 0.0,
    .high_angle = 0.0,
  }

};

void tbl_range_init() {

  for (uint8_t range = 0; range < TABLE_SIZE(alarms); range++) {
    for (uint8_t i = 0; i < TABLE_SIZE(translate_tbl) - 1; i++) {

      if (alarms[range].low_bound >= translate_tbl[i].gauge &&  alarms[range].low_bound < translate_tbl[i + 1].gauge) {
        alarms[range].low_angle = (alarms[range].low_bound - translate_tbl[i].m) /  translate_tbl[i].k;
      }
      if (alarms[range].high_bound >= translate_tbl[i].gauge &&  alarms[range].high_bound < translate_tbl[i + 1].gauge) {
        alarms[range].high_angle = (alarms[range].high_bound - translate_tbl[i].m) /  translate_tbl[i].k;
      }
    }
  }
}

void tbl_init() {
  uint8_t i;
  int16_t res = -1;
  for (i = 0; i < (TABLE_SIZE(translate_tbl) - 1) ; i++) /* Don't run to the absolute end! */
  {
    translate_tbl[i].k = (translate_tbl[i + 1].gauge - translate_tbl[i].gauge) / (translate_tbl[i + 1].input - translate_tbl[i].input);
    translate_tbl[i].m = (float)translate_tbl[i].gauge - (translate_tbl[i].input *   translate_tbl[i].k);

  }
  return res;
}

int16_t intrapolate(float input) {
  uint8_t i;
  int16_t res = -1;
  if (input < translate_tbl[0].input) {
    res = translate_tbl[0].gauge;
  } else if (input > translate_tbl[(TABLE_SIZE(translate_tbl) - 1)].input) {
    res = translate_tbl[(TABLE_SIZE(translate_tbl) - 1)].gauge;
  } else {

    for (i = 0; i < (TABLE_SIZE(translate_tbl) - 1) ; i++) /* Don't run to the absolute end! */
    {
      if (input >= translate_tbl[i].input && input < translate_tbl[i + 1].input) {

        res = translate_tbl[i].k * input +  translate_tbl[i].m;

      }
    }
  }
  return res ;
}

scan_area_t inner_array[] = {
  {

    .x = IMG_WIDTH / 2 - SCAN_LENGTH_SMALL / 2 + SCAN_LENGTH_SMALL,
    .y = INNER_Y_BASE - SCAN_LENGTH_SMALL + SCAN_LINE_PX_WIDTH,
    .scan_width_px = SCAN_LINE_PX_WIDTH,
    .scan_length = SCAN_LENGTH_SMALL,
    .dir = SCAN_DIRECTION_DOWN,
  }
  ,

  {

    .x = IMG_WIDTH / 2 - SCAN_LENGTH_SMALL / 2,
    .y = INNER_Y_BASE ,
    .scan_width_px = SCAN_LINE_PX_WIDTH,
    .scan_length = SCAN_LENGTH_SMALL,
    .dir = SCAN_DIRECTION_HORIZONTAL,
  }

  ,
#if 1
  {

    .x = IMG_WIDTH / 2 - SCAN_LENGTH_SMALL / 2,
    .y = INNER_Y_BASE ,
    .scan_width_px = SCAN_LINE_PX_WIDTH,
    .scan_length = SCAN_LENGTH_SMALL,
    .dir = SCAN_DIRECTION_UP,
  }

#endif
};

scan_area_t outer_array[] = {
#if 1
  {

    .x = IMG_WIDTH / 2 - SCAN_LENGTH_LARGE / 2,
    .y = OUTER_Y_BASE + SCAN_LENGTH_LARGE ,
    .scan_width_px = SCAN_LINE_PX_WIDTH,
    .scan_length = SCAN_LENGTH_LARGE,
    .dir = SCAN_DIRECTION_UP,
  },
#endif
  {

    .x = IMG_WIDTH / 2 - SCAN_LENGTH_LARGE / 2,
    .y = OUTER_Y_BASE,
    .scan_width_px = SCAN_LINE_PX_WIDTH,
    .scan_length = SCAN_LENGTH_LARGE,
    .dir = SCAN_DIRECTION_HORIZONTAL,
  }
  ,
#if 1
  {

    .x = IMG_WIDTH / 2 - SCAN_LENGTH_LARGE / 2 + SCAN_LENGTH_LARGE,
    .y = OUTER_Y_BASE  ,
    .scan_width_px = SCAN_LINE_PX_WIDTH,
    .scan_length = SCAN_LENGTH_LARGE,
    .dir = SCAN_DIRECTION_DOWN,
  },
#endif

};

uint8_t opposite_side_table[TABLE_SIZE(outer_array)] {0, 1, 2};
uint8_t outerbuf[SCAN_LENGTH_LARGE * SCAN_LINE_PX_WIDTH ];
uint8_t innerbuf[SCAN_LENGTH_SMALL * SCAN_LINE_PX_WIDTH ];

buf_t Inner = {

  .x = 0,
  .y = 0,
  .scan_width_px = SCAN_LINE_PX_WIDTH,
  .scan_length = SCAN_LENGTH_SMALL ,
  .buf = innerbuf,
};

buf_t Outer = {

  .x = 0,
  .y = 0,
  .scan_width_px = SCAN_LINE_PX_WIDTH,
  .scan_length = SCAN_LENGTH_LARGE ,
  .buf = outerbuf,
};

#define B 0x001F
#define G 0x0EC
#define R 0xF800

inline uint16_t grayscale(uint16_t rgb) {
  uint8_t r, g, b;
  uint16_t res, gray;

  g = ((rgb) >> 5) & 0x3F;
  b = ((rgb) >> 11) & 0x1F;

  gray = ((r + g / 2 + b ) * 10) / 2;
  gray = gray / 10 ;
  res = (gray << 11) | (gray << 6) | (gray);

  return res;
}

int find_max(buf_t *buf)
{
  int x, pos = -1;
  uint8_t maxi = 0;

  for (x = 0; x < buf->scan_length; x++)
  {

    if (buf->buf[x] > maxi) {
      maxi = buf->buf[x] ;
      pos = x;

    }
  }
  return pos;
}

void conv(kern_t *k, buf_t *buf, buf_t *out_result) {
  uint16_t x, y, kernx, kerny;
  uint8_t *avg;
  uint16_t divisor = 1;
  uint8_t kern_center = k->dim_w / 2;

  avg = malloc(buf->scan_length);
  out_result->x = 0;
  out_result->y = 0;
  out_result->scan_width_px = 1;
  out_result->scan_length = buf->scan_length;
  out_result->buf = avg;

  memset(avg, 0, buf->scan_length);

  for (y = 0; y < 1; y++) {
    for (x = kern_center; x < buf->scan_length - kern_center ; x++) {
      uint16_t sum = 0;

      for (kerny = 0; kerny < k->dim_h; kerny++) {
        for (kernx = 0; kernx < k->dim_w; kernx++) {
          sum = sum + (k->kern[kerny * k->dim_w + kernx] * buf->buf[(kernx + x - kern_center) + ((kerny + y) * buf->scan_length)]);

        }
      }

      avg[x] = (uint8_t) (sum / k->denom);

    }
  }
}

void GetPixels_scan_area(const scan_area_t *scan_area, uint16_t *imgbuf, buf_t *target) {

  uint16_t x, y, bufx, bufy;
  uint8_t i;
  uint16_t target_px;
  target_px = 0;

  switch (scan_area->dir) {
    case SCAN_DIRECTION_HORIZONTAL:
      for (y = scan_area->y, bufy = 0; y < scan_area->y + scan_area->scan_width_px; y++, bufy++) {
        for (x = scan_area->x, bufx = target_px; x < ( scan_area->x +  scan_area->scan_length); x++, bufx++) {
          uint16_t gray;
          gray = grayscale(imgbuf[x + y * IMG_WIDTH]);
          target->buf[bufy * scan_area->scan_length + bufx] = (gray & 0x1F) < 0x08 ? 0x002 : 0x0000;
          //          imgbuf[x + y * IMG_WIDTH] = (gray & 0x1F) < 0x08 ? 0xF000 : gray ;

        }
      }
      break;
    case SCAN_DIRECTION_DOWN:
      for (x = scan_area->x, bufy = 0; x < ( scan_area->x +  scan_area->scan_width_px); x++, bufy++) {
        for (y = scan_area->y, bufx = target_px; y < scan_area->y + scan_area->scan_length; y++, bufx++) {

          uint16_t gray;
          gray = grayscale(imgbuf[x + y * IMG_WIDTH]);
          target->buf[bufy * scan_area->scan_length + bufx] = (gray & 0x1F) < 0x08 ? 0x002 : 0x0000;
          //imgbuf[x + y * IMG_WIDTH] = (gray & 0x1F) < 0x08 ? 0xF00f : gray ;

        }
      }

      break;
    case SCAN_DIRECTION_UP:
      for (x = scan_area->x, bufy = 0; x < ( scan_area->x +  scan_area->scan_width_px); x++, bufy++) {
        for (y = scan_area->y, bufx = target_px; y > scan_area->y - scan_area->scan_length; y--, bufx++) {

          uint16_t gray;
          gray = grayscale(imgbuf[x + y * IMG_WIDTH]);
          target->buf[bufy * scan_area->scan_length + bufx] = (gray & 0x1F) < 0x08 ? 0x002 : 0x0000;
          //imgbuf[x + y * IMG_WIDTH] = (gray & 0x1F) < 0x08 ? 0xF0f0 : gray ;

        }
      }

      break;
  }
}

float getAngle(const px_coord_t *a, const px_coord_t *b)
{
  px_coord_t diff;
  float angle;

  diff.x = b->x - a->x;
  diff.y = b->y - a->y;

  angle = atanf(((float)diff.y) / diff.x);
  angle = angle * 180 / 3.14;

  if (diff.x < 0) {
    angle = 180 + angle;
  }

  return angle;
}

static uint8_t frameupdate = 0;

int8_t getCoordfromPos(scan_area_t *scan_area, px_coord_t *out, uint16_t maxpos) {

  uint8_t i;
  uint16_t area_x;
  int8_t ret = -1;
  area_x = 0;

  if (maxpos >= area_x && maxpos < area_x + scan_area->scan_length)
  {
    switch (scan_area->dir) {
      case   SCAN_DIRECTION_HORIZONTAL:
        out->x =  scan_area->x + (maxpos - area_x);
        out->y = scan_area->y;
        break;
      case  SCAN_DIRECTION_DOWN:
        out->x = scan_area->x;
        out->y = scan_area->y + (maxpos - area_x);
        break;
      case  SCAN_DIRECTION_UP:
        out->x = scan_area->x;
        out->y = scan_area->y - (maxpos - area_x);
        break;
    }
    ret = 0;
  }

  return ret;
}

void DrawArc(const px_coord_t *center, uint16_t radius, float angle_start, float angle_end, void *buf, uint8_t thickness, uint16_t color)
{
  float a;
  float angle_compensated;
  int16_t x, y;
  uint16_t *img_buf = buf;
  //thickness = 1;
  for (a = angle_start  ; a < angle_end ; a = a + 2) {
    angle_compensated = a;
    for (uint8_t r = 0; r < thickness; r = r + 1) {
      x = cos(angle_compensated * 3.14 / 180) * (radius + r);
      y = sin(angle_compensated * 3.14 / 180) * (radius + r);

      x =  center->x - x;
      y =  center->y - y;
      //img_buf[x + y * IMG_WIDTH] = img_buf[x + y * IMG_WIDTH] &  color;

      for (int8_t ty = -2; ty <= 2; ty++) {
        for (int8_t tx = -2; tx <= 2; tx++) {
          img_buf[x +  tx + (ty + y) * IMG_WIDTH] =  color;
        }

      }

    }
  }

}
static px_coord_t center = {.x = IMG_WIDTH / 2 + CENTER_X_OFFSET - 5, .y = IMG_HEIGHT / 2 + CENTER_Y_OFFSET};

void CamCB(CamImage img) {
  buf_t result_avg_2d, result_avg_1d;
  px_coord_t out,  in;
  float angle;
  int max_pos_inner, max_pos_outer;
  uint8_t area_i;
  uint16_t *b;
  b = (uint16_t *)img.getImgBuff();
  int16_t gaugeVal;
  gaugeVal = 0;

  if (!img.isAvailable()) return;
  img.convertPixFormat(CAM_IMAGE_PIX_FMT_RGB565);

  for (area_i = 0; area_i < TABLE_SIZE(outer_array); area_i++) {
    GetPixels_scan_area(&outer_array[area_i], b, &Outer);

    conv(&average, &Outer, &result_avg_2d);
    conv(&average_1dim, &result_avg_2d, &result_avg_1d);
    free(result_avg_2d.buf);
    max_pos_outer = find_max(&result_avg_1d);
    free(result_avg_1d.buf);

    if (max_pos_outer == -1) {
      continue;
    }

    GetPixels_scan_area(&inner_array[opposite_side_table[area_i]], b, &Inner);
    conv(&average, &Inner, &result_avg_2d);
    conv(&average_1dim, &result_avg_2d, &result_avg_1d);
    free(result_avg_2d.buf);
    max_pos_inner = find_max(&result_avg_1d);
    free(result_avg_1d.buf);
    if (max_pos_inner == -1)
    {
      continue;
    }

    getCoordfromPos(&inner_array[opposite_side_table[area_i]], &in, max_pos_inner);

    getCoordfromPos(&outer_array[area_i], &out, max_pos_outer);

    angle = getAngle(&out, &in);
    gaugeVal = intrapolate(angle);

  }

  if (frameupdate++ % 1 == 0) {
    printf("gauge: % 3d angle: %3.1f\n", gaugeVal, angle);
    if (gaugeVal < 0) {
      gaugeVal = 0;
    }
    for (uint16_t y = 0; y < 24; y++) {
      for (uint16_t x = 0; x < 22 * (gaugeVal >= 1000 ? 4 : 3); x++) {
        b[y * IMG_WIDTH + x] = 0x0000;
      }
    }

    tft.setBuffer(b);

    tft.drawLine(in.x , in.y , out.x, out.y , tft.color565(255, 0, 255));

    tft.drawRect(IMG_WIDTH / 2 - BOX_SIZE + CENTER_X_OFFSET, (IMG_HEIGHT / 2 - BOX_SIZE) + CENTER_Y_OFFSET,  BOX_SIZE,  BOX_SIZE , tft.color565(0, 255, 0));
    tft.setCursor(0, 22);

    for (uint8_t r = 0; r < TABLE_SIZE(alarms); r++) {
      DrawArc(&center, 100, alarms[r].low_angle, alarms[r].high_angle, b, 1, alarms[r].color);
      if (gaugeVal >= alarms[r].low_bound && gaugeVal < alarms[r].high_bound) {
        tft.setTextColor(alarms[r].color);
      }
    }
    tft.print(gaugeVal);

    tft.drawRGBBitmap();
    frameupdate = 1;
  }

}

void setup() {
  Serial.begin(BAUDRATE);

  tft.begin();
  tft.setRotation(3);
  theCamera.begin(2, CAM_VIDEO_FPS_120, CAM_IMGSIZE_QVGA_H, CAM_IMGSIZE_QVGA_V, CAM_IMAGE_PIX_FMT_YUV422);
  theCamera.startStreaming(true, CamCB);
  tbl_init();
  tbl_range_init();

  tft.setTextColor(0xffff);
  tft.setFont(&FreeMonoBold18pt7b);
}

void loop() {
  /* do nothing here */
}
