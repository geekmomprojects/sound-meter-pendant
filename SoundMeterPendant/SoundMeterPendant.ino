/****************************************
Sound level sketch to read values from a microphone breakout and 
display on an 8x8 mini Dotstar LED matrix. Currently using an Adafruit
matrix (https://www.adafruit.com/product/3444) and a MEMS microphone
breakout board: https://www.adafruit.com/product/2716

Algorithm adapted from Adafruit's CPX VU meter code here:
https://learn.adafruit.com/adafruit-microphone-amplifier-breakout/measuring-sound-levels
****************************************/

#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_DotStarMatrix.h>
#include <Adafruit_DotStar.h>
#ifndef PSTR
 #define PSTR // Make Arduino Due happy
#endif

#define DATAPIN   4
#define CLOCKPIN  3
#define MICPIN    1
#define MATRIX_WIDTH  8
#define MATRIX_HEIGHT 8

Adafruit_DotStarMatrix matrix = Adafruit_DotStarMatrix(
  MATRIX_WIDTH, MATRIX_HEIGHT, DATAPIN, CLOCKPIN,
  DS_MATRIX_BOTTOM     + DS_MATRIX_RIGHT +
  DS_MATRIX_COLUMNS + DS_MATRIX_PROGRESSIVE,
  DOTSTAR_BRG);


uint16_t  soundLevels[MATRIX_WIDTH];
uint8_t   soundLevelPointer = 0;
float     input_floor;
float     input_ceiling;

#define NUM_SAMPLES 160
// CURVE should be a value between -10 and 10 - empirically tested to arrive at best value for this setup
float CURVE = 4.0;
float SCALE_EXPONENT = pow(10, CURVE * -0.1);

float samples[NUM_SAMPLES];
float sample_buffer[NUM_SAMPLES];

float log_scale(float input_value, float input_min, float input_max, float output_min, float output_max) {
  float normalized_input_value = (input_value - input_min) / (input_max - input_min);
  return output_min + pow(normalized_input_value, SCALE_EXPONENT) * (output_max - output_min);
}

float sum(float* values, uint16_t nvalues) {
  float sum = 0;
  for (int i = 0; i < nvalues; i++) {
    sum = sum + values[i];
  }
  return sum;
}

float mean(float* values, uint16_t nvalues) {
  return sum(values, nvalues)/nvalues;
}

float normalized_rms(float *values, uint16_t nvalues) {
  int minbuf = (int) mean(values, nvalues);
  for (int i = 0; i < nvalues; i++) {
    sample_buffer[i] = pow((values[i] - minbuf),2);
  }
  return sqrt(sum(sample_buffer, nvalues)/nvalues);
}

void recordSamples() {
  for (int i = 0; i < NUM_SAMPLES; i++) {
    samples[i] = analogRead(MICPIN);
  }
}

void setup() 
{
   Serial.begin(112500);
   matrix.begin();
   matrix.setBrightness(72);
   matrix.fillScreen(0);
   matrix.setRotation(2);

   // Sound levels array to store data for last 8 readings
   for (int i = 0; i < MATRIX_WIDTH; i++) {
     soundLevels[i] = 0;
   }

   // Record an initial sample to calibrate. Assume it's quiet when we start
   // recordSamples();
   //input_floor = normalized_rms(samples, NUM_SAMPLES) + .1;

   // Empirically determined by testing different values
   input_floor = 1.0;
   input_ceiling = input_floor + 6;
}


int peak = 0;
void loop() 
{
   float magnitude = 0;
   uint8_t  level;

   recordSamples();
   magnitude = normalized_rms(samples, NUM_SAMPLES);
   float c = log_scale(constrain(magnitude, input_floor, input_ceiling), input_floor, input_ceiling, 0, MATRIX_HEIGHT-1);


/*
   Serial.print(magnitude);
   Serial.print(" ");
   Serial.print(input_floor);
   Serial.print(" ");
   Serial.print(input_ceiling);
   Serial.print(" ");
   Serial.println(c);
*/
   level = (int) c;

   //Storing last 8 sound levels in an array for a running level display - not currently used
   soundLevels[soundLevelPointer] = level;
   soundLevelPointer = (soundLevelPointer + 1) % MATRIX_WIDTH;

   //uint16_t color = matrix.Color(32*level, 0, 255-32*level);
   uint16_t peak_color = matrix.Color(255,255,0);
   // Let the peak fall a little more gradually than the sound levels
   matrix.fill(0);
   if (level >= peak) {
    peak = level;
   } else {
    peak = max(peak - 1, level);
   }
   matrix.drawLine(0,0,3,peak,peak_color);
   matrix.drawLine(4,peak,7,0,peak_color);
   matrix.show();
   
   delay(5);

 
}
