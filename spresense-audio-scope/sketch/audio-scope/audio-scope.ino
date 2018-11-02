#define SPI_HAS_TRANSACTION
#define USE_HW_SPI
#include <Audio.h>
#include <SPI.h>
#include <TFT.h>
#include <arch/board/board.h>

#define SAMPLE_NUM 256     // Select from 16, 64, 256, 1024
#define SAMPLE_FREQ 48.0e3 // Sample frequency of ADC
#define CHANNELS 2	 // Channels of ADCs, 4 or 2

#define READ_PACKET_SIZE 6144
#define BUFFER_SIZE READ_PACKET_SIZE * 4

/*
    Trigger level, signal must be higher than this value for a sweep to begin.
*/
#define TRIGGLEVEL 3

/*
  TFT display WIDHT and HEIGHT.
*/
#define TFT_WIDTH 160
#define TFT_HEIGHT 128
#define TFT_HEIGHT_HALF TFT_HEIGHT / 2

/*
  TFT display control pin definitions.
*/
#define cs PIN_D10
#define dc PIN_D09
#define rst PIN_D08

AudioClass *audio;

/*
   Audio buffer to be used with the Audio library.
*/
char buffer[BUFFER_SIZE];

/*
   Audio capture and wave form print state machine.
*/
typedef enum {
  STATE_INIT,
  STATE_CAPTURE_START,
  STATE_CAPTURE,
  STATE_CAPTURE_END,
  STATE_PRINT_WAVE,
} state_e;

/*
   The trigger states for the scope function.
*/
typedef enum scope_e {
  NOT_TRIGGED,
  WAITING_FOR_TRIGG,
  TRIGGED,
  REACHED_LIMIT,
};

state_e fsm_state = STATE_INIT;

/*
   Instance of the ST7735 TFT display driver.
*/
TFT TFTscreen = TFT(cs, dc, rst);

/*
  Functon to transform the wave amplitude to a color. To give the demo a more
  colorful apparence.
*/
uint16_t plot_color(int y) {

  int red, green, blue;
  uint16_t col;

  if (y < 0) {
    y = -y;
  }

  blue = map(y, 0, 64, 8, 0);
  green = map(y, 0, 64, 50, 0);
  red = map(y, 0, 30, 5, 31);
  green = green << 5;

  col = red | green | blue;

  return col;
}

/*
   Plot a pixel on the screen, increase horisontal x pointer for each pixel that
   has been printed.
   A simple trigger function is implemented so that the wave form will always be
   started on a rising slope when signal is higher than TRIGGLEVEL.
*/
int plotPx(int val) {
  static int x_ptr = 0;
  uint16_t col;
  static int previous_y = 0;
  static char triggd = 0;
  int y;
  static int y_prev[TFT_WIDTH] = {0};
  scope_e ret = NOT_TRIGGED;

  /* Map the input value to the screens vertical resolution. */
  y = map(val, -1024, 1024, -TFT_HEIGHT_HALF, TFT_HEIGHT_HALF);

  /* Set the screen y boundaries for the signal. */
  if (y < -TFT_HEIGHT_HALF) {
    y = -TFT_HEIGHT_HALF;
  } else if (y > TFT_HEIGHT_HALF) {
    y = TFT_HEIGHT_HALF;
  }

  /* Assign a color to the signal waveform depending on the amplitude of the
     signal. */
  col = plot_color(y);

  y = y + TFT_HEIGHT_HALF;

  /* The trigger logic. */
  if (triggd == 0 && (y < TFT_HEIGHT_HALF - TRIGGLEVEL) && (y < previous_y)) {
    triggd = 1;
    ret = TRIGGED;
  } else {
    previous_y = y;
    ret = WAITING_FOR_TRIGG;
  }
  if (triggd == 0) {
    return NOT_TRIGGED;
  }

  /* Paint the previous pixel on position x black to erase it. */
  if (y_prev[x_ptr] != y) {
    TFTscreen.drawPixel(x_ptr, y_prev[x_ptr], 0x00);
  }

  TFTscreen.drawPixel(x_ptr, y, col);

  y_prev[x_ptr] = y;

  /* Code to handle once the x limit of the screen has been reached. */
  if (++x_ptr > TFT_WIDTH) {
    x_ptr = 0;
    triggd = 0;
    previous_y = 0;
    ret = REACHED_LIMIT;
  }

  return ret;
}

/*
   The classic arduino setup function for application initialisation.
*/
void setup() {
  audio = AudioClass::getInstance();
  audio->begin();
  audio->setRecorderMode(AS_SETRECDR_STS_INPUTDEVICE_MIC_A);
  audio->initRecorder(AS_CODECTYPE_PCM, AS_SAMPLINGRATE_48000,
                      AS_CHANNEL_STEREO);

  fsm_state = STATE_INIT;

  TFTscreen.begin();
  TFTscreen.setRotation(1);
  TFTscreen.background(0, 0, 0);

  Serial.begin(115200);
}
/*
*/
void loop() {
  // put your main code here, to run repeatedly:
  static uint32_t read_size = 0;
  static uint32_t total_read_size = 0;
  int err;

  switch (fsm_state) {
    /* First state, bascially just don't do anything else than go to next state.
    */
    case STATE_INIT:

      fsm_state = STATE_CAPTURE_START;
      break;

    case STATE_CAPTURE_START:
      /* In this state the audio started to be captured into 'buffer'. */
      err = audio->readFrames(buffer, BUFFER_SIZE, &read_size);
      audio->startRecorder();
      total_read_size = 0;

      fsm_state = STATE_CAPTURE;

      break;

    case STATE_CAPTURE:
      /* We will stay in this state and fill 'buffer' with audio data until
         the we have captured the desired amount of data. */
      err = audio->readFrames(&buffer[total_read_size],
                              BUFFER_SIZE - total_read_size, &read_size);
      total_read_size += read_size;
      if (err != AUDIOLIB_ECODE_OK &&
          err != AUDIOLIB_ECODE_INSUFFICIENT_BUFFER_AREA) {
        sleep(1);
        audio->stopRecorder();
        exit(1);
      }

      /* Exit this state when the desired amout of data has been captured. */
      if (total_read_size > SAMPLE_NUM * CHANNELS * 2) {
        fsm_state = STATE_CAPTURE_END;
      }

      break;

    case STATE_CAPTURE_END: {
        /* Finalize the audio capture and display the wave form on the screen.
        */
        int pad;
        int res;
        int ystep = (SAMPLE_NUM * CHANNELS * 2) / 512;
        audio->stopRecorder();

        for (uint32_t i = 0; i < SAMPLE_NUM * CHANNELS * 2 - 100; i++) {
          short dac0 =
            ((((short)buffer[8 * i + 1]) << 8) | (short)buffer[8 * i]);

          res = plotPx(dac0);

          if (res == REACHED_LIMIT) {
            break;
          }
        }

        fsm_state = STATE_INIT;
        break;
      }
  }
}
