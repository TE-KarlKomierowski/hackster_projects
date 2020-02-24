// Copyright (c) Sandeep Mistry. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef LORA_H
#define LORA_H

#include "gpio.h"
#include <spi.h>
#include <stdint.h>

#define SPI

#define LORA_DEFAULT_SPI SPI
#define LORA_DEFAULT_SPI_FREQUENCY 8E6
#define LORA_DEFAULT_SS_PIN PIN_UART2_RTS
#define LORA_DEFAULT_RESET_PIN PIN_UART2_CTS
#define LORA_DEFAULT_DIO0_PIN PIN_SEN_IRQ_IN

/* Reserved to SPI:
#define SPI_SCK 		PIN_EMMC_DATA3
#define SPI_MOSI 		PIN_EMMC_DATA2
#define SPI_CS 			PIN_I2S0_DATA_IN
#define SPI_MISO 		PIN_I2S0_DATA_OUT
*/

#define PA_OUTPUT_RFO_PIN 0
#define PA_OUTPUT_PA_BOOST_PIN 1

typedef uint8_t byte;

class SPIClass {
public:
  void beginTransaction(int spi);
  uint16_t transfer(uint16_t data);
  void endTransaction();
  void begin(void);

  void end(void);

private:
  int test;
  struct spi_dev_s *spidev;
};

class LoRaClass {
public:
  LoRaClass();

  int begin(long frequency);
  void end();

  int beginPacket(int implicitHeader = false);
  int endPacket(bool async = false);

  int parsePacket(int size = 0);
  int packetRssi();
  float packetSnr();
  long packetFrequencyError();

  // from Print
  size_t write(uint8_t byte);
  size_t write(const uint8_t *buffer, size_t size);

  // from Stream
  int available();
  int read();
  int peek();
  void flush();

#ifndef ARDUINO_SAMD_MKRWAN1300
  void onReceive(void (*callback)(int));

  void receive(int size = 0);
#endif
  void idle();
  void sleep();

  void setTxPower(int level, int outputPin = PA_OUTPUT_PA_BOOST_PIN);
  void setFrequency(long frequency);
  void setSpreadingFactor(int sf);
  void setSignalBandwidth(long sbw);
  void setCodingRate4(int denominator);
  void setPreambleLength(long length);
  void setSyncWord(int sw);
  void enableCrc();
  void disableCrc();
  void enableInvertIQ();
  void disableInvertIQ();

  void setOCP(uint8_t mA); // Over Current Protection control

  // deprecated
  void crc() { enableCrc(); }
  void noCrc() { disableCrc(); }

  byte random();

  void setPins(int ss = LORA_DEFAULT_SS_PIN, int reset = LORA_DEFAULT_RESET_PIN, int dio0 = LORA_DEFAULT_DIO0_PIN);
  void setSPI(SPIClass &spi);
  void setSPIFrequency(uint32_t frequency);

  void dumpRegisters();

private:
  void explicitHeaderMode();
  void implicitHeaderMode();

  void handleDio0Rise();
  bool isTransmitting();

  int getSpreadingFactor();
  long getSignalBandwidth();

  void setLdoFlag();

  uint8_t readRegister(uint8_t address);
  void writeRegister(uint8_t address, uint8_t value);
  uint8_t singleTransfer(uint8_t address, uint8_t value);

  static void onDio0Rise();

private:
  int _spiSettings;
  SPIClass *_spi;
  int _ss;
  int _reset;
  int _dio0;
  long _frequency;
  int _packetIndex;
  int _implicitHeaderMode;
  void (*_onReceive)(int);
};

extern LoRaClass LoRa;

#endif
