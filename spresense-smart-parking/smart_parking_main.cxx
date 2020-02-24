
/****************************************************************************
 * camera/camera_main.c
 *
 *   Copyright 2018 Sony Semiconductor Solutions Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name of Sony Semiconductor Solutions Corporation nor
 *    the names of its contributors may be used to endorse or promote
 *    products derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <sdk/config.h>

#include <debug.h>
#include <errno.h>
#include <fcntl.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

#include "video/video.h"
#include <nuttx/arch.h>
#include <nuttx/board.h>
#include <nuttx/fs/mkfatfs.h>

#include <sys/boardctl.h>
#include <sys/ioctl.h>
#include <sys/mount.h>

#include <arch/board/board.h>
#include <arch/chip/cisif.h>
#include <arch/chip/pm.h>
#include <pthread.h>

#ifdef CONFIG_EXAMPLES_SMART_PARKING_OUTPUT_LCD
#include "nximage.h"

#include <nuttx/lcd/lcd.h>
#include <nuttx/nx/nx.h>
#include <nuttx/nx/nxglib.h>

#ifdef CONFIG_IMAGEPROC
#include <imageproc/imageproc.h>
#endif
#endif

#include "common.h"
#include "image.h"

#include "lora/LoRa.h"
#include "parking_data.h"
#include "spi.h"
#include "trx.h"
#include "xmit.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
/* Display of vsync timing */
/* #define CAMERA_MAIN_CISIF_INTRTRACE */

/* Note: Buffer size must be multiple of 32. */

#define IMAGE_JPG_SIZE (512 * 1024) /* 512kB */

#define IMAGE_QVGA_W (320)
#define IMAGE_QVGA_H (240)
#define IMAGE_YUV_SIZE (IMAGE_QVGA_W * IMAGE_QVGA_H * 2) /* QVGA YUV422 */

#define VIDEO_BUFNUM (3)
#define STILL_BUFNUM (1)

#define DEFAULT_REPEAT_NUM (10)

#define IMAGE_FILENAME_LEN (32)

#ifdef CONFIG_EXAMPLES_SMART_PARKING_OUTPUT_LCD
#ifndef CONFIG_EXAMPLES_CAMERA_LCD_DEVNO
#define CONFIG_EXAMPLES_CAMERA_LCD_DEVNO 0
#endif

#define itou8(v) ((v) < 0 ? 0 : ((v) > 255 ? 255 : (v)))
#endif /* CONFIG_EXAMPLES_SMART_PARKING_OUTPUT_LCD */

#define LED0 PIN_I2S1_BCK
#define LED1 PIN_I2S1_LRCK
#define LED2 PIN_I2S1_DATA_IN
#define LED3 PIN_I2S1_DATA_OUT

/****************************************************************************
 * Private Types
 ****************************************************************************/
struct uyvy_s {
  uint8_t u0;
  uint8_t y0;
  uint8_t v0;
  uint8_t y1;
};

struct v_buffer {
  uint32_t *start;
  uint32_t length;
};
typedef struct v_buffer v_buffer_t;

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Private Data
 ****************************************************************************/

static struct v_buffer *buffers_video;
static struct v_buffer *buffers_still;
static unsigned int n_buffers;
static bool autostart = false;
static const char *save_dir;

#ifdef CONFIG_EXAMPLES_SMART_PARKING_OUTPUT_LCD
struct nximage_data_s g_nximage = {
    NULL,  /* hnx */
    NULL,  /* hbkgd */
    0,     /* xres */
    0,     /* yres */
    false, /* havpos */
    {0},   /* sem */
    0      /* exit code */
};
#endif

/****************************************************************************
 * Public Data
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

int trx_init() { return LoRa.begin(915E6); }

int trx_transmit(const char *msg) {
  int ret;
  ssize_t len;
  ret = -1;
  digitalWrite(LED0, 1);

  if (msg != NULL) {
    uint8_t *msg_p = (uint8_t *)msg;
    len = strlen(msg);
    LoRa.beginPacket();
    LoRa.write(msg_p, len);
    LoRa.endPacket();
    ret = len;
  }
  digitalWrite(LED0, 0);
  return ret;
}

#ifdef CONFIG_EXAMPLES_SMART_PARKING_OUTPUT_LCD
static inline int nximage_initialize(void) {
  FAR NX_DRIVERTYPE *dev;
  nxgl_mxpixel_t color;
  int ret;

  /* Initialize the LCD device */

  printf("nximage_initialize: Initializing LCD\n");
  ret = board_lcd_initialize();
  if (ret < 0) {
    printf("nximage_initialize: board_lcd_initialize failed: %d\n", -ret);
    return ERROR;
  }

  /* Get the device instance */

  dev = board_lcd_getdev(CONFIG_EXAMPLES_CAMERA_LCD_DEVNO);
  if (!dev) {
    printf("nximage_initialize: board_lcd_getdev failed, devno=%d\n", CONFIG_EXAMPLES_CAMERA_LCD_DEVNO);
    return ERROR;
  }

  ret = image_init(320, 240);

  /* Turn the LCD on at 75% power */

  (void)dev->setpower(dev, ((3 * CONFIG_LCD_MAXPOWER + 3) / 4));

  /* Then open NX */

  printf("nximage_initialize: Open NX\n");
  g_nximage.hnx = nx_open(dev);
  if (!g_nximage.hnx) {
    printf("nximage_initialize: nx_open failed: %d\n", errno);
    return ERROR;
  }

  /* Set background color to black */

  color = 0;
  nx_setbgcolor(g_nximage.hnx, &color);
  ret = nx_requestbkgd(g_nximage.hnx, &g_nximagecb, NULL);
  if (ret < 0) {
    printf("nximage_initialize: nx_requestbkgd failed: %d\n", errno);
    nx_close(g_nximage.hnx);
    return ERROR;
  }

  while (!g_nximage.havepos) {
    (void)sem_wait(&g_nximage.sem);
  }
  printf("nximage_initialize: Screen resolution (%d,%d)\n", g_nximage.xres, g_nximage.yres);

  return 0;
}

#ifndef CONFIG_IMAGEPROC
static inline void ycbcr2rgb(uint8_t y, uint8_t cb, uint8_t cr, uint8_t *r, uint8_t *g, uint8_t *b) {
  int _r;

  int _g;
  int _b;
  _r = (128 * (y - 16) + 202 * (cr - 128) + 64) / 128;
  _g = (128 * (y - 16) - 24 * (cb - 128) - 60 * (cr - 128) + 64) / 128;
  _b = (128 * (y - 16) + 238 * (cb - 128) + 64) / 128;
  *r = itou8(_r);
  *g = itou8(_g);
  *b = itou8(_b);
}

static inline uint16_t ycbcrtorgb565(uint8_t y, uint8_t cb, uint8_t cr) {
  uint8_t r;
  uint8_t g;
  uint8_t b;

  ycbcr2rgb(y, cb, cr, &r, &g, &b);
  r = (r >> 3) & 0x1f;
  g = (g >> 2) & 0x3f;
  b = (b >> 3) & 0x1f;
  return (uint16_t)(((uint16_t)r << 11) | ((uint16_t)g << 5) | (uint16_t)b);
}

/* Color conversion to show on display devices. */

static void yuv2rgb(void *buf, uint32_t size) {
  struct uyvy_s *ptr;
  struct uyvy_s uyvy;
  uint16_t *dest;
  uint32_t i;

  ptr = (struct uyvy_s *)buf;
  dest = (uint16_t *)buf;
  for (i = 0; i < size / 4; i++) {
    /* Save packed YCbCr elements due to it will be replaced with
     * converted color data.
     */

    uyvy = *ptr++;

    /* Convert color format to packed RGB565 */

    *dest++ = ycbcrtorgb565(uyvy.y0, uyvy.u0, uyvy.v0);
    *dest++ = ycbcrtorgb565(uyvy.y1, uyvy.u0, uyvy.v0);
  }
}
#endif /* !CONFIG_IMAGEPROC */
#endif /* CONFIG_EXAMPLES_SMART_PARKING_OUTPUT_LCD */

static int camera_prepare(int fd, enum v4l2_buf_type type, uint32_t buf_mode, uint32_t pixformat, uint16_t hsize,
                          uint16_t vsize, uint8_t buffernum) {
  int ret;
  uint8_t cnt;
  uint32_t fsize;
  struct v4l2_format fmt = {0};
  struct v4l2_requestbuffers req = {0};
  struct v4l2_buffer buf = {0};
  struct v_buffer *buffers;

  /* VIDIOC_REQBUFS initiate user pointer I/O */

  req.type = type;
  req.memory = V4L2_MEMORY_USERPTR;
  req.count = buffernum;
  req.mode = buf_mode;

  ret = ioctl(fd, VIDIOC_REQBUFS, (unsigned long)&req);
  if (ret < 0) {
    printf("Failed to VIDIOC_REQBUFS: errno = %d\n", errno);
    return ret;
  }

  /* VIDIOC_S_FMT set format */

  fmt.type = type;
  fmt.fmt.pix.width = hsize;
  fmt.fmt.pix.height = vsize;
  fmt.fmt.pix.field = V4L2_FIELD_ANY;
  fmt.fmt.pix.pixelformat = pixformat;

  ret = ioctl(fd, VIDIOC_S_FMT, (unsigned long)&fmt);
  if (ret < 0) {
    printf("Failed to VIDIOC_S_FMT: errno = %d\n", errno);
    return ret;
  }

  /* VIDIOC_QBUF enqueue buffer */

  if (type == V4L2_BUF_TYPE_VIDEO_CAPTURE) {
    buffers_video = (struct v_buffer *)malloc(sizeof(v_buffer_t) * buffernum);
    buffers = buffers_video;
  } else {
    buffers_still = (v_buffer_t *)malloc(sizeof(v_buffer_t) * buffernum);
    buffers = buffers_still;
  }

  if (!buffers) {
    printf("Out of memory\n");
    return ret;
  }

  if (pixformat == V4L2_PIX_FMT_UYVY) {
    fsize = IMAGE_YUV_SIZE;
  } else {
    fsize = IMAGE_JPG_SIZE;
  }

  for (n_buffers = 0; n_buffers < buffernum; ++n_buffers) {
    buffers[n_buffers].length = fsize;

    /* Note: VIDIOC_QBUF set buffer pointer. */
    /*       Buffer pointer must be 32bytes aligned. */

    buffers[n_buffers].start = (uint32_t *)memalign(32, fsize);
    if (!buffers[n_buffers].start) {
      printf("Out of memory\n");
      return ret;
    }
  }

  for (cnt = 0; cnt < n_buffers; cnt++) {
    memset(&buf, 0, sizeof(v4l2_buffer_t));
    buf.type = type;
    buf.memory = V4L2_MEMORY_USERPTR;
    buf.index = cnt;
    buf.m.userptr = (unsigned long)buffers[cnt].start;
    buf.length = buffers[cnt].length;

    ret = ioctl(fd, VIDIOC_QBUF, (unsigned long)&buf);
    if (ret) {
      printf("Fail QBUF %d\n", errno);
      return ret;
      ;
    }
  }

  /* VIDIOC_STREAMON start stream */

  ret = ioctl(fd, VIDIOC_STREAMON, (unsigned long)&type);
  if (ret < 0) {
    printf("Failed to VIDIOC_STREAMON: errno = %d\n", errno);
    return ret;
  }

  return OK;
}

static void free_buffer(struct v_buffer *buffers, uint8_t bufnum) {
  uint8_t cnt;
  if (buffers) {
    for (cnt = 0; cnt < bufnum; cnt++) {
      if (buffers[cnt].start) {
        free(buffers[cnt].start);
      }
    }

    free(buffers);
  }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/
const uint8_t lorastr[] = "Smart";
#ifdef CONFIG_BUILD_KERNEL
int main(int argc, FAR char *argv[])
#else
extern "C" int smart_parking_main(int argc, char *argv[])
// int smart_parking_main(int argc, char *argv[])
#endif
{
  int ret;
  int exitcode = ERROR;
  int v_fd;
  struct stat stat_buf;
  uint32_t loop;
  uint32_t buf_type;
  uint32_t format;
  struct v4l2_buffer buf;
  uint16_t loopetime = 0;
  bool dump_img = false;
  bool hist_initialised = false;
  uint16_t spaces, ispace;
  void *rect_buf;
  void *filter_buf;
  bool state_has_changed;
  bool has_trx = false;
  bool run_edge_detect = false;

  /* select capture mode */

#ifdef CONFIG_EXAMPLES_SMART_PARKING_OUTPUT_LCD
  ret = nximage_initialize();
  if (ret < 0) {
    printf("camera_main: Failed to get NX handle: %d\n", errno);
    return ERROR;
  }
#ifdef CONFIG_IMAGEPROC
  imageproc_initialize();
#endif
#endif /* CONFIG_EXAMPLES_SMART_PARKING_OUTPUT_LCD */

  if (trx_init() == 1) {
    has_trx = true;
    trx_transmit("SmartParking");
  } else {
    printf("Initialsing serial: %d\n", serial_init());
  }
  printf("TRX init is: %d\n", has_trx);
  // has_trx = false;

  parking_data_init();
  xmit_data();

  /* In SD card is available, use SD card.
   * Otherwise, use SPI flash.
   */
  if (autostart == false && xmit_read_request() == IMAGE_CMD_REQ_IMG) {
    dump_img = true;
  }

  ret = stat("/mnt/sd0", &stat_buf);
  if (ret < 0) {
    save_dir = "/mnt/spif";
  } else {
    save_dir = "/mnt/sd0";
  }

  if (argc >= 2 && strncmp(argv[1], "cap", 4) == 0) {
    buf_type = V4L2_BUF_TYPE_STILL_CAPTURE;
    format = V4L2_PIX_FMT_JPEG;
  } else {
    buf_type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    format = V4L2_PIX_FMT_UYVY;
  }

  if (argc == 3) {
    loop = atoi(argv[2]);
  } else {
    loop = DEFAULT_REPEAT_NUM;
  }

  ret = video_initialize("/dev/video");
  if (ret != 0) {
    printf("ERROR: Failed to initialize video: errno = %d\n", errno);
    goto errout_with_nx;
  }

  v_fd = open("/dev/video", 0);
  if (v_fd < 0) {
    printf("ERROR: Failed to open video.errno = %d\n", errno);
    goto errout_with_video_init;
  }

  /* Prepare VIDEO_CAPTURE */

  ret = camera_prepare(v_fd, V4L2_BUF_TYPE_VIDEO_CAPTURE,
                       // V4L2_BUF_MODE_RING,
                       V4L2_BUF_MODE_FIFO, V4L2_PIX_FMT_UYVY, VIDEO_HSIZE_QVGA, VIDEO_VSIZE_QVGA, VIDEO_BUFNUM);
  if (ret < 0) {
    goto errout_with_buffer;
  }

  /* Prepare STILL_CAPTURE */

  ret = camera_prepare(v_fd, V4L2_BUF_TYPE_STILL_CAPTURE, V4L2_BUF_MODE_FIFO, V4L2_PIX_FMT_JPEG, VIDEO_HSIZE_FULLHD,
                       VIDEO_VSIZE_FULLHD, STILL_BUFNUM);
  if (ret < 0) {
    goto errout_with_buffer;
  }

  typedef struct {
    parking_state_t previous;
    parking_state_t latest;
    uint8_t id;
  } space_state_t;

  spaces = parking_data_get_nparkings();
  space_state_t *parking_states;

  if (spaces > 0) {
    spaces = parking_data_get_nparkings();
    parking_states = (space_state_t *)malloc(spaces * sizeof(space_state_t));

    for (int a = 0; a < spaces; a++) {
      parking_states[a].previous = PARKING_STATE_INIT;
      parking_states[a].latest = PARKING_STATE_NONE;
      parking_states[a].id = a + 1;
    }
  } else {
    return -1;
  }

  while (loop > 0) {
    /* Note: VIDIOC_DQBUF acquire capture data. */

    memset(&buf, 0, sizeof(v4l2_buffer_t));
    buf.type = buf_type;
    buf.memory = V4L2_MEMORY_USERPTR;

    ret = ioctl(v_fd, VIDIOC_DQBUF, (unsigned long)&buf);
    if (ret) {
      printf("Fail DQBUF %d\n", errno);
      goto errout_with_buffer;
    }

#ifdef CONFIG_EXAMPLES_SMART_PARKING_OUTPUT_LCD
    if (format == V4L2_PIX_FMT_UYVY) {

#ifdef CONFIG_IMAGEPROC
      imageproc_convert_yuv2rgb((void *)buf.m.userptr, VIDEO_HSIZE_QVGA, VIDEO_VSIZE_QVGA);
#else
      yuv2rgb((void *)buf.m.userptr, buf.bytesused);
#endif

      rectangle_t *rect;
      histogram_t hist = {0};

      // spaces = 1;
      image_print_vt100();

      for (ispace = 1; ispace <= spaces; ispace++) {
        memset(&hist, 0, sizeof(hist));

        rect = parking_data_get_rectangle(ispace);

        if (!rect) {
          printf("No parking space found!\n");
        } else {
#define LOOPS_BEFORE_HIST_INIT 10
#define LOOPS_SLOW_RESET_INIT 100

          rect_buf = image_get_mem_from_rectangle((void *)buf.m.userptr, rect);
          // image_get_histogram_from_rectangle(rect_buf, rect, &hist);
          // image_print_histogram(&hist, ispace);

          if (hist_initialised == false && loopetime == LOOPS_BEFORE_HIST_INIT) {
            parking_data_add_hist(ispace, &hist);
            parking_data_set_space_img(ispace, rect_buf);
            image_print_vt100();
            image_print_vt100_clrscr();
          }

          if (hist_initialised && loopetime > LOOPS_BEFORE_HIST_INIT) {
            void *orig_img;
            parking_state_t ps;
            // parking_data_is_occupied(ispace, &hist);

            orig_img = parking_data_get_space_img(ispace);
            image_diff_px_counter_t cnt;
            cnt = image_diff_rectangle(rect_buf, orig_img, rect);

            ps = parking_data_occupied_img(ispace, &cnt);
            printf("% 2d %- 8s (r %3d g %3d b %3d) %d", ispace, parking_data_state_tostr(ps), cnt.r, cnt.g, cnt.b, ps);
            if (ispace == 23 && ps == PARKING_STATE_OCCUPIED) {
              run_edge_detect = true;
            } else if (ispace == 23 && ps == PARKING_STATE_FREE) {
              run_edge_detect = false;
            }

            parking_states[ispace - 1].latest = ps;

            if (!(loopetime % LOOPS_SLOW_RESET_INIT) && ps == PARKING_STATE_FREE) {
              image_subtract_diff_rectangle(rect_buf, orig_img, rect);
              printf("*");
            } else {
              printf(" ");
            }
/* Run a edge detection filter. */
#define EDGE_DETECT
#ifdef EDGE_DETECT
            if (run_edge_detect) {
              filter_buf = image_convolution(0, (void *)rect_buf, rect);
              filter_buf = image_convolution(3, (void *)rect_buf, rect);

              image_set_mem_from_rectangle((void *)buf.m.userptr, rect_buf, rect);
            }
#endif
            printf("\n");
          }
          free(rect_buf);
        }
      }
      printf("%d %d!\n", hist_initialised, loopetime);
      if (hist_initialised == false && loopetime == LOOPS_BEFORE_HIST_INIT) {
        hist_initialised = true;
      }

      if (1) {
        char *lora_ptr, *lora_msg;
        int nchars, l;
        nchars = 0;

        static uint32_t tx_counter = 0;
        l = sizeof("00,F:") * spaces;
        lora_msg = (char *)malloc(l);
        lora_ptr = lora_msg;
        lora_ptr[0] = '\0';

        for (ispace = 0; ispace < spaces; ispace++) {
          if ((parking_states[ispace].latest != parking_states[ispace].previous)
              || (loopetime && loopetime % 300 == 0)) {
            if ((parking_states[ispace].latest == PARKING_STATE_FREE)
                || (parking_states[ispace].latest == PARKING_STATE_OCCUPIED)) {

              parking_states[ispace].previous = parking_states[ispace].latest;
              nchars
                  += sprintf(&lora_ptr[nchars], "%2d,%2d:", parking_states[ispace].id, parking_states[ispace].latest);
              state_has_changed = true;
            }
          }
        }
        if (state_has_changed) {
          printf("Lora message # %d: [% *s]\n", ++tx_counter, l, lora_msg);
          if (has_trx) {
            trx_transmit(lora_msg);
          } else {
            serial_write((const uint8_t *)lora_msg, strlen(lora_msg));
          }

          state_has_changed = false;
        }

        free(lora_msg);
      }

#undef XMIT
#define XMIT
#if defined(XMIT)
      if (dump_img) {
        if (loopetime && loopetime % 15 == 0) {
          xmit_image((void *)buf.m.userptr, IMAGE_QVGA_W, IMAGE_QVGA_H);
        }
      }

#endif
#endif /* CONFIG_EXAMPLES_SMART_PARKING_OUTPUT_LCD */
      loopetime++;
      parking_data_outline_spaces((void *)buf.m.userptr);

      nximage_image(g_nximage.hbkgd, (void *)buf.m.userptr);

      ret = ioctl(v_fd, VIDIOC_QBUF, (unsigned long)&buf);
      if (ret) {
        printf("Fail QBUF %d\n", errno);
        goto errout_with_buffer;
      }
    }

#if 0
    if (buf_type == V4L2_BUF_TYPE_STILL_CAPTURE) {
      ret = ioctl(v_fd, VIDIOC_TAKEPICT_STOP, false);
      if (ret < 0) {
        printf("Failed to start taking picture\n");
      }
    }
#endif
  }

  exitcode = OK;

errout_with_buffer:
  close(v_fd);

  free_buffer(buffers_video, VIDEO_BUFNUM);
  free_buffer(buffers_still, STILL_BUFNUM);

errout_with_video_init:

  video_uninitialize();

errout_with_nx:
#ifdef CONFIG_EXAMPLES_SMART_PARKING_OUTPUT_LCD
#ifdef CONFIG_IMAGEPROC
  imageproc_finalize();
#endif
  nx_close(g_nximage.hnx);
#endif /* CONFIG_EXAMPLES_SMART_PARKING_OUTPUT_LCD */

  return exitcode;
}

extern "C" int smart_parking(int, char **);

int smart_parking(int argc, char *argv[]) {
  (void)boardctl(BOARDIOC_INIT, 0); /* Needed to be able to use this function as the entry point in the SDK. */
  autostart = true;
  return smart_parking_main(argc, argv);
}
