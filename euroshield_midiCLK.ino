#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h> 

#include <MIDI.h>
#define MIDI_PLAY 250
#define MIDI_STOP 252
#define MIDI_CLK 248
#define MIDI_CONTINUE 251
//#define DEBUG

#define buttonPin     2
#define upperPotInput 20
#define lowerPotInput 21
#define kMaxPotInput  1024
#define ledPinCount   4
#define debounceMS    100


AudioInputI2S            audioInput;
AudioAnalyzeRMS          input_1;
AudioAnalyzeRMS          input_2;
AudioSynthWaveformDc     dc1;
AudioSynthWaveformDc     dc2;
AudioSynthWaveform       waveform1;
AudioOutputI2S           audioOutput;
AudioConnection          patchCord1(audioInput, 0, input_1, 0);
AudioConnection          patchCord2(audioInput, 1, input_2, 0);

//AudioConnection          patchCord3(waveform1, 0, audioOutput, 0);
AudioConnection          patchCord3(dc1, 0, audioOutput, 0);
AudioConnection          patchCord4(dc2, 0, audioOutput, 1);
AudioControlSGTL5000     sgtl5000_1;

AudioAnalyzePeak readDC1;
AudioAnalyzePeak readDC2;
AudioConnection          patchCord5(audioInput, 0, readDC1, 0);
AudioConnection          patchCord6(audioInput, 1, readDC2, 0);   



// 1010MIDIMonWithThru - target: the 1010music Euroshield with the PJRC Teensy 3.2
//
// This sketch demonstrates MIDI setup with automatic thruing. The LEDs will blink
// each time a message is received.
//
// This code is in the public domain.

MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, MIDI);

#define ledPinCount 4
int ledPins[ledPinCount] = { 3, 4, 5, 6 };
int ledPos = 0;

void setup()
{

  AudioMemory(12);
  sgtl5000_1.enable();
  sgtl5000_1.inputSelect(AUDIO_INPUT_LINEIN);
  sgtl5000_1.volume(0.82);
  sgtl5000_1.adcHighPassFilterDisable();
  sgtl5000_1.lineInLevel(0,0);
  sgtl5000_1.unmuteHeadphone();
  
  for (int i = 0; i < ledPinCount; i++)
    pinMode(ledPins[i], OUTPUT);
  MIDI.begin(MIDI_CHANNEL_OMNI);
  #ifdef DEBUG
  Serial.begin(38400);
  #endif

  waveform1.frequency(100);
  waveform1.amplitude(0.0); //start as OFF state
  waveform1.offset(0.5);
  waveform1.begin(WAVEFORM_SQUARE);
  
}

void advanceLED()
{
  digitalWrite(ledPins[ledPos % ledPinCount], LOW);
  ledPos++;  
  digitalWrite(ledPins[ledPos % ledPinCount], HIGH);
}

unsigned long t=0, c_hi=0, c_lo=0;
bool isRun=false;
bool clockUp = false;
float trigDuration = 1; //ms
int pulseTime = 0;
bool isReset=false;

#define MCLK_STEPS 24
#define RESET_BEATS 32
float prev_CLK_t = -1.0;
float CLK_t = -1.0;
float MCLK_Itvl = 0;
float MCLK_p[MCLK_STEPS];
short int count_CLK = 0;
float avg_p = 0;
float avg_p_secs = 0;
float prev_avg_p_secs = 0;
float sum_p = 0;
short int CLK_MULT[11] = {1, 2, 4, 8, 16, 32, 1/32, 1/16, 1/8, 1/4, 1/2};
short int CLK_beat_count = 0;



float average (float * array, int len)  // assuming array is int.
{
  double sum = 0 ;  // sum will be larger than an item, long for safety.
  for (int i = 0 ; i < len ; i++)
    sum += array [i] ;
  return  ((float) sum) / len ;  // average will be fractional, so float may be appropriate.
}

void loop()
{

  // *************
  // MIDI SECTION
  // *************
  int note, velocity, channel, d1, d2;
  if (MIDI.read())                    // If we have received a message
  {
      
      byte type = MIDI.getType();
      // MIDI Thru happens automatically with the library
      d1 = MIDI.getData1();
      d2 = MIDI.getData2();
      //Serial.println(String("Message, type=") + type + ", data = " + d1 + " " + d2);
      t = millis();

      if ((type == MIDI_PLAY) || (type == MIDI_CONTINUE)){
        // run the clock
        isRun = true;
        // handle reset
        isReset = true;
        pulseTime = t;  // set current time for start Reset pulse

        //start the clock
        CLK_beat_count = 0; // reset beat counter
        count_CLK = 0;
        //waveform1.restart();
        //waveform1.amplitude(0.5);
        

      }

      if (type == MIDI_STOP){
        isRun = false;
        clockUp=false;
        waveform1.amplitude(0.0);
      }

      if (type == MIDI_CLK){


        if(isRun){
          if (count_CLK == 0){
            clockUp = !clockUp;
          } 
          count_CLK = (count_CLK + 1) % 3;  //12 is x1/1, 6 is x2/1, 3 is x4/1 
        }


        
        if (prev_CLK_t < 0){
          prev_CLK_t = micros();
        }else{
          
          /*
          CLK_t = micros();
          MCLK_p[count_CLK] = CLK_t - prev_CLK_t;
          prev_CLK_t = CLK_t; // advance current measurement
          //Serial.print("MCLK_Itvl-"); Serial.print(count_CLK); Serial.print(" "); Serial.println(MCLK_p[count_CLK]);
          avg_p = average(MCLK_p, MCLK_STEPS);
          */

          /*
          sum_p = sum_p - MCLK_p[count_CLK];
          CLK_t = micros();   // we use micros cause it has higher resolution
          MCLK_p[count_CLK] = CLK_t - prev_CLK_t;
          prev_CLK_t = CLK_t; // advance current measurement
          // compute moving average
          sum_p = sum_p + MCLK_p[count_CLK];
          count_CLK = (count_CLK + 1) % MCLK_STEPS;
          avg_p = sum_p / MCLK_STEPS;
          avg_p *= MCLK_STEPS; 
          avg_p /= 1000; // from us to ms
          avg_p_secs = avg_p / 1000;  // from ms to s
          //Serial.println(avg_p);
          // read multiplier knob value
          int mult_idx = analogRead(upperPotInput);
          mult_idx = map(mult_idx, 0, 1023, 0, 10);
          waveform1.frequency((1.0/avg_p_secs)*CLK_MULT[mult_idx]);  // update frequency of clock
          */

          /*
          if( prev_avg_p_secs == 0){
            prev_avg_p_secs = avg_p_secs;
          }else{
            //if (avg_p_secs - prev_avg_p_secs > )
            Serial.println(avg_p_secs - prev_avg_p_secs);
          }
          */
          
          // we count every beats
          // NOT IMPLEMENTED RIGHT?? THE OUTPUT CLOCK IS NOT VERY STABLE WITH THIS THING
          /*
          if (count_CLK == 0){
            CLK_beat_count++;
          }
          // every RESET_BEATS beats, we reset the waveform to keep it synched
          if (CLK_beat_count == RESET_BEATS) {
            waveform1.restart();  // restart the LFO
            CLK_beat_count = 0;  // reset the count of beats
          }
          */
        }
        
        if(isRun){
          advanceLED();
        }
        
      }
      
      
  }

  // 
  // ******************
  // RESET TRIG OUTPUT SECTION
  // ******************
  //
  if(isReset){
    dc2.amplitude(1.0); // start/continue pulse
    
    if ((millis() - pulseTime) > trigDuration){  //if trigger duration is passed bys
      dc2.amplitude(0.0); // stop reset pulse
      isReset = false; // reset state for RESET 
    }
    
  }else{
    dc2.amplitude(0.0); // no reset if !isReset
    
  }

  float gateval = clockUp;
  dc1.amplitude(gateval);



  /*
  DEBUG

  if (millis() - t > 1500) {
    t += 1500;
    Serial.print(t);
    Serial.println(" (inactivity)");
  }
  */
    
}