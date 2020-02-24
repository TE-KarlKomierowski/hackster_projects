#include "spi.h"
#include <nuttx/spi/spi_bitbang.h>
#include <stdio.h>

#include <arch/board/board.h>
#include <arch/chip/pin.h>
#include <debug.h>

#if 1
#define LED0 PIN_I2S1_BCK
#define LED1 PIN_I2S1_LRCK
#define LED2 PIN_I2S1_DATA_IN
#define LED3 PIN_I2S1_DATA_OUT
#else
#define LED0 0
#define LED1 0
#define LED2 0
#define LED3 0
#endif

#define SPI_SCK PIN_EMMC_DATA3
#define SPI_MOSI PIN_EMMC_DATA2
#define SPI_CS PIN_I2S0_DATA_OUT
#define SPI_MISO PIN_I2S0_DATA_IN

#define SPI_SETSCK board_gpio_write(SPI_SCK, 1)
#define SPI_CLRSCK board_gpio_write(SPI_SCK, 0)

#define SPI_SETMOSI board_gpio_write(SPI_MOSI, 1)
#define SPI_CLRMOSI board_gpio_write(SPI_MOSI, 0)

#define SPI_GETMISO board_gpio_read(SPI_MISO)

#define SPI_SETCS board_gpio_write(SPI_CS, 1)
#define SPI_CLRCS board_gpio_write(SPI_CS, 0)

/* Only mode 0 */

#undef SPI_BITBANG_DISABLEMODE0
#define SPI_BITBANG_DISABLEMODE1 1
#define SPI_BITBANG_DISABLEMODE2 1
#define SPI_BITBANG_DISABLEMODE3 1

/* Only 8-bit data width */

#undef SPI_BITBANG_VARWIDTH

/* Calibration value for timing loop */

#define SPI_BITBAND_LOOPSPERMSEC CONFIG_BOARD_LOOPSPERMSEC

/* SPI_PERBIT_NSEC is the minimum time to transfer one bit.  This determines
 * the maximum frequency and is also used to calculate delays to achieve
 * other SPI frequencies.
 */

#define SPI_PERBIT_NSEC 100

/* Misc definitions */

#include <nuttx/spi/spi_bitbang.c>

static struct spi_dev_s *spidev = NULL;

struct spi_dev_s *spi_init() {
  board_gpio_write(LED0, -1);
  board_gpio_config(LED0, 0, 0, true, PIN_FLOAT);
  board_gpio_write(LED3, -1);
  board_gpio_config(LED3, 0, 0, true, PIN_FLOAT);
  board_gpio_write(LED1, -1);
  board_gpio_config(LED1, 0, 0, true, PIN_FLOAT);
  board_gpio_write(LED2, -1);
  board_gpio_config(LED2, 0, 0, true, PIN_FLOAT);
  board_gpio_write(LED2, 1);

  board_gpio_write(SPI_SCK, -1);
  board_gpio_config(SPI_SCK, 0, 0, true, PIN_FLOAT);

  board_gpio_write(SPI_MOSI, -1);
  board_gpio_config(SPI_MOSI, 0, 0, true, PIN_FLOAT);

  board_gpio_write(SPI_CS, -1);
  board_gpio_config(SPI_CS, 0, 0, true, PIN_FLOAT);

  board_gpio_write(SPI_MISO, -1);
  board_gpio_config(SPI_MISO, 0, true, false, PIN_PULLUP);

  SPI_SETCS;
  // SPI_SETSCK;

  /* FAR struct spi_dev_s *spi_create_bitbang(FAR const struct spi_bitbang_ops_s *low); */

  spidev = spi_create_bitbang(&g_spiops);
  printf("%s() %p\n", __func__, spidev);
  return spidev;
}

/****************************************************************************
 * Name: spi_select
 *
 * Description:
 *   Select or de-selected the SPI device specified by 'devid'
 *
 * Input Parameters:
 *   priv     - An instance of the bit-bang driver structure
 *   devid    - The device to select or de-select
 *   selected - True:select false:de-select
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

static void spi_select(FAR struct spi_bitbang_s *priv, uint32_t devid, bool selected) {

  if (selected) {
    SPI_CLRCS;
  } else {

    SPI_SETCS;
  }
}

/****************************************************************************
 * Name: spi_status
 *
 * Description:
 *   Return status of the SPI device specified by 'devid'
 *
 * Input Parameters:
 *   priv     - An instance of the bit-bang driver structure
 *   devid    - The device to select or de-select
 *
 * Returned Value:
 *   An 8-bit, bit-encoded status byte
 *
 ****************************************************************************/

static uint8_t spi_status(FAR struct spi_bitbang_s *priv, uint32_t devid) { return 0; }

/****************************************************************************
 * Name: spi_cmddata
 *
 * Description:
 *   If there were was a CMD/DATA line, this function would manage it
 *
 * Input Parameters:
 *   priv  - An instance of the bit-bang driver structure
 *   devid - The device to use
 *   cmd   - True=MCD false=DATA
 *
 * Returned Value:
 *  OK
 *
 ****************************************************************************/

#ifdef CONFIG_SPI_CMDDATA
static int spi_cmddata(FAR struct spi_bitbang_s *priv, uint32_t devid, bool cmd) { return OK; }

#endif
