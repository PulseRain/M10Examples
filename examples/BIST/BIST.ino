/*
###############################################################################
# Copyright (c) 2017, PulseRain Technology LLC 
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License (LGPL) as 
# published by the Free Software Foundation, either version 3 of the License,
# or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful, but 
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY 
# or FITNESS FOR A PARTICULAR PURPOSE.  
# See the GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
###############################################################################
*/

//============================================================================
// BIST (Built-in Self-Test)
//============================================================================
//     This sketch does the BIST for M10 board, and it will be the default
// image for UFM
//
//     To run the BIST, connect the M10 board to Windows PC through USB, 
// and open the Tera-Term with 115200bps
//
//     Press the reset button to reset the board, and the Serial Console 
// should display something like  "BIST Vx.xx for PulseRain M10"
//
//     Press the push button next to the DC power jack to switch test steps.
//
//     Read the prompts on the Serial Console for instructions.
//============================================================================

#include "M10SD.h"
#include "M10SRAM.h"
#include "M10CODEC.h"
#include "M10ADC.h"
#include "M10DTMF.h"

// Firmware Version
#define BIST_VERSION "1.01"

// total number of test steps
#define TOTAL_NUM_OF_TEST_STEPS 3

// Pin number for LEDs
#define LED_BUILTIN_GREEN 13
#define LED_BUILTIN_RED   15

// Pin for the push button (The button next to the DC power jack)
#define PUSH_BUTTON_PIN   11


// DTMF Key index code
#define DTMF_ASTERISK 14
#define DTMF_POUND 15

// 64 bit chip id
uint8_t chip_id [8];

// index for test steps
uint8_t test_step;

// for push button debouncer
uint8_t prev_button_status;

// for LED test
uint8_t led_select = 0;

// flag for microSD
uint8_t SD_available;


// Test Instructions
uint8_t* test_name[TOTAL_NUM_OF_TEST_STEPS] = {
"IDLE, All LED on",
"LED and ADC Test\n\n This test will adjust the LED flashing interval based on the ADC channel 0.\n Set IO Jumper (JP1) to 5V, and use a wire to connect IOREF to A0.\n\
 Turn the potentiometer to observe the ADC value displayed on the Serial Console.\n\
 The LED will flash slow or fast accordingly.\n",
 "DTMF, Speaker, SRAM, microSD Test\n\n This test will detect DTMF tone through the onboard microphone.\n\
 And it will load the correspondent wav file from microSD card to SRAM, then play the audio out through an external speaker.\n\
 The wav files to be stored on the microSD card can be found in the audio folder next to this sketch file.\n"
};




//----------------------------------------------------------------------------
// button_read()
//
// Parameters:
//      None
//
// Return Value:
//      Push Button Status
//
// Remarks:
//      function to read push button with debouncer
//----------------------------------------------------------------------------

uint8_t button_read ()
{
    uint8_t t1, t2;

    do {
      t1 = digitalRead(PUSH_BUTTON_PIN);
      delay (100);
      t2 = digitalRead(PUSH_BUTTON_PIN);
    } while (t1 != t2);

    return t1;
    
} // End of button_read()





//----------------------------------------------------------------------------
// adc_isr_handler()
//
// Parameters:
//      None
//
// Return Value:
//      None
//
// Remarks:
//      isr handler for ADC (IRQ 5)
//----------------------------------------------------------------------------

uint16_t adc_value;

void adc_isr_handler()
{
   uint8_t high, low;
   
   high = ADC_DATA_HIGH;
   low  = ADC_DATA_LOW;

   adc_value = (high << 8) + low;
   low = ADC_CSR;

} // End of adc_isr_handler()



//----------------------------------------------------------------------------
// print_test_message()
//
// Parameters:
//      None
//
// Return Value:
//      None
//
// Remarks:
//      print message for new test steps
//----------------------------------------------------------------------------

void print_test_message()
{
    Serial.write ("\n\n Test Step ==> ");
    Serial.print(test_step);
    Serial.write ("  ");
    Serial.write(test_name[test_step]);
    Serial.write ("\n");
    
} // End of print_test_message()




//----------------------------------------------------------------------------
// test_led()
//
// Parameters:
//      interval : the interval for LED flashing
//
// Return Value:
//      None
//
// Remarks:
//      function to test LED with adjustable flashing interval
//----------------------------------------------------------------------------

void test_led (uint16_t interval)
{
      led_select = 1 - led_select;

      if (led_select) {
          digitalWrite(LED_BUILTIN_GREEN, LOW);  // turn the LED on (HIGH is the voltage level)
      } else {
          digitalWrite(LED_BUILTIN_RED, LOW);    // turn the LED on (HIGH is the voltage level) 
      }
  
      delay(interval);                          // wait 
      digitalWrite(LED_BUILTIN_GREEN, HIGH);    // turn the LED off by making the voltage LOW
      digitalWrite(LED_BUILTIN_RED, HIGH);      // turn the LED off by making the voltage LOW
   //   delay(interval);                          // wait 

} // End of test_led()


//----------------------------------------------------------------------------
// get_u16()
//
// Parameters:
//      ptr    : pointer to BYTE
//
// Return Value:
//      (*ptr) + ((*(ptr+1)) << 8)
//
// Remarks:
//      get a word from the pointer
//----------------------------------------------------------------------------

uint16_t get_u16 (uint8_t *ptr)
{
  uint16_t t;
  t = (uint16_t)(*ptr);
  ++ptr;
  t += (uint16_t)(*ptr) << 8;

  return t;
} // get_u16()


//----------------------------------------------------------------------------
// get_u32()
//
// Parameters:
//      ptr    : pointer to BYTE
//
// Return Value:
//      (*ptr) + ((*(ptr+1)) << 8) + ((*(ptr+2)) << 16) + ((*(ptr+3)) << 24) 
//
// Remarks:
//      get a DWORD from the pointer
//----------------------------------------------------------------------------

uint32_t get_u32(uint8_t *ptr)
{
  uint16_t t;
  uint32_t ret;
  
  ret = get_u16(ptr);
  
  ptr += 2;
  
  t = get_u16(ptr);

  ret += (uint32_t)t << 16;
  
  return ret;  
} // End of get_u32()


//----------------------------------------------------------------------------
// sample buffer and scratch buffer
//----------------------------------------------------------------------------

uint8_t wav_samp_buffer [256];
uint8_t wav_samp_scratch_buffer[256];

//----------------------------------------------------------------------------
// str_cmp_case_insensitive()
//
// Parameters:
//      a    : pointer to buffer A
//      b    : pointer to buffer B
//      cnt  : buffer size to compare 
//
// Return Value:
//      return 0 if buffer A and buffer B are the same (case insensitive)
//
// Remarks:
//      compare byte buffer with case insensitive
//----------------------------------------------------------------------------

uint8_t str_cmp_case_insensitive (uint8_t *a, uint8_t *b, uint8_t cnt)
{
  uint8_t i, ta, tb;

  for (i = 0; i < cnt; ++i) {
      ta = a[i]; 
      tb = b[i];
  
      if ((ta >= 'a') && (ta <= 'z')) {
        ta = ta - 'a' + 'A';
      }
  
      if ((tb >= 'a') && (tb <= 'z')) {
        tb = tb - 'a' + 'A';
      }
  
      if (ta != tb) {
        return 1;
      }
  }

  return 0;
  
} // End of str_cmp_case_insensitive()


//----------------------------------------------------------------------------
// read_file()
//
// Parameters:
//      cnt  : buffer size to read
//      buf  : string to compare against (set it to 0 for no string comparison
//
// Return Value:
//      return 0 if data is read ok and is the same as the buf (case insensitive)
//
// Remarks:
//      read data into wav_samp_buffer, and compare it against buf
//----------------------------------------------------------------------------

uint8_t read_file(uint8_t cnt, uint8_t *buf)
{
  uint16_t br, btr;
  uint8_t res;

  btr = cnt; br = 0;
  res = SD.fread(wav_samp_buffer, btr, &br);
  if ((res) || (btr != br)) {
    Serial.write ("wav read error\n");
    return 0xff;
  }

  if(buf) {
      res = str_cmp_case_insensitive(wav_samp_buffer, buf, btr);
    
      if (res) {
   //     Serial.write ("!!!!!!!! not match the subtrunk of");
   //     Serial.write (buf, cnt);
   //     Serial.write ("\n");
        return 0xff;
      } else {
   //     Serial.write ("==> match ");
   //     Serial.write (buf, cnt);
   //     Serial.write ("\n");
      }
  }

  return 0;
  
} // End of read_file()


//----------------------------------------------------------------------------
// flags for wav file
//----------------------------------------------------------------------------

uint16_t block_align;
uint16_t num_of_channels;
uint32_t num_of_samples;
uint8_t wav_file_header_size;


//----------------------------------------------------------------------------
// parse_wav_file_head()
//
// Parameters:
//      None
//
// Return Value:
//      return 0 if wav file head is parsed ok 
//
// Remarks:
//      parse wav file head, only mono 8KHz sample rate is supported.
//----------------------------------------------------------------------------

uint8_t parse_wav_file_head () 
{
  uint16_t t;
  uint8_t res;
  uint32_t subchunk_size, sample_rate;
  
  uint8_t cnt = 0;

//  Serial.write ("==========================================\n");
//  Serial.write (" Start parsing head for wav file\n");
//  Serial.write ("==========================================\n");

  // chunk_ID
      read_file (4, "RIFF");
      cnt += 4;

  // chunk_size 
      read_file (4, 0);
      cnt += 4;
      
  // format 
      read_file (4, "WAVE");
      cnt += 4;
      
  // subchunk1_ID
      read_file (4, "fmt ");
      cnt += 4;
      
  // subchunk1_size
      read_file (4, 0);
      cnt += 4;
      subchunk_size = get_u32(wav_samp_buffer);
  //    Serial.write ("subchunk1_size ");
  //    Serial.println(subchunk_size);


  // audio_format
      read_file (2, 0);
      cnt += 2;
  //    Serial.write ("audio format ");
  //    Serial.println(get_u16(wav_samp_buffer));

  // num_of_channels
      read_file (2, 0);
      cnt += 2;
      num_of_channels = get_u16(wav_samp_buffer);
  //    Serial.write ("num_of_channels ");
  //    Serial.println(num_of_channels);

      if (num_of_channels != 1) {
  //      Serial.write ("only mono (single channel) is supported!\n");
        return 0xff;  
      }

  // sample_rate 
      read_file (4, 0);
      cnt += 4;
      sample_rate = get_u32(wav_samp_buffer);
  //    Serial.write ("sample_rate ");
  //    Serial.println(sample_rate);

      if (sample_rate != 8000) {
  //      Serial.write ("not 8KHz sample rate!\n");
        return 0xff;
      }

  // byte_rate
      read_file (4, 0);
      cnt += 4;
  //    Serial.write ("byte_rate ");
  //    Serial.println(get_u32(wav_samp_buffer));

  // block_align
      read_file (2, 0);
      cnt += 2;
      block_align = get_u16(wav_samp_buffer);
  //    Serial.write ("block_align ");
  //    Serial.println(block_align);


  // bits_per_sample
      read_file (2, 0);
      cnt += 2;
  //    Serial.write ("bits_per_sample ");
  //    Serial.println(get_u16(wav_samp_buffer));

  // read extra
      if ((20 + subchunk_size) > cnt) {
        t = 20 + subchunk_size - cnt;
 //       Serial.write ("read extra pad ");
 //       Serial.print(t);
 //       Serial.write (" bytes\n");
        read_file (t, 0);
      }
      
 // data
      t = 4;
      do { 
       res = read_file (4, "data");
       cnt += 4; 
       read_file (4, 0);
       cnt += 4;
       subchunk_size = get_u32(wav_samp_buffer);

       if (res) {
  //        Serial.write ("skip this subtrunk\n");
          read_file (subchunk_size, 0);
          cnt += subchunk_size;
       }

       --t;
       
      } while (res && t);

      if (!t) {
        Serial.write ("can not find data subtrunk\n");
        return 0xff; 
      }

      wav_file_header_size = cnt;

//      Serial.write ("wav_file_header_size ");
//      Serial.println(wav_file_header_size);
      
      num_of_samples = subchunk_size / block_align;
//      Serial.write ("num_of_samples = ");
//      Serial.println(num_of_samples);


//  Serial.write ("==========================================\n");
//  Serial.write (" End parsing the head for wav file\n");
//  Serial.write ("==========================================\n");
  
  return 0;
  
} // End of parse_wav_file_head()


//----------------------------------------------------------------------------
// load_wav_into_SRAM()
//
// Parameters:
//      None
//
// Return Value:
//      None
//
// Remarks:
//      load wav file samples (The first 128KB) into SRAM
//----------------------------------------------------------------------------

void load_wav_into_SRAM() 
{
  uint32_t i, max_num_of_samp, addr;
  uint16_t sample;
  uint8_t t, res;

  if (num_of_samples > 65536) {
    max_num_of_samp = 65536;
  } else {
    max_num_of_samp = num_of_samples;
  }

  addr = 0;
  for (i = 0; i < max_num_of_samp; ++i) {
    res = read_file (block_align, 0);
    if (res) {
      break;
    }

    sample = get_u16(wav_samp_buffer);
    
    t = sample & 0xFF;
    SRAM.write(addr, t);
    
    t = (sample >> 8) & 0xFF;
    ++addr;
          
    SRAM.write(addr, t);
    ++addr;
  }
  
} // End of load_wav_into_SRAM()



//----------------------------------------------------------------------------
// play_wav_from_SRAM()
//
// Parameters:
//      None
//
// Return Value:
//      None
//
// Remarks:
//      play audio samples stored in the SRAM
//----------------------------------------------------------------------------

void play_wav_from_SRAM() 
{
  uint8_t t;
  uint32_t i, max_num_of_samp, addr;
  uint16_t sample;

  uint16_t br = 0, btr = 0;

  
  if (num_of_samples > 65536) {
      max_num_of_samp = 65536;
  } else {
      max_num_of_samp = num_of_samples;
  }

  t = 0;
  addr = 0;
  for (i = 0; i < max_num_of_samp; ++i) {
   
      t = SRAM.read(addr);
      sample = t;

      ++addr;
      t = SRAM.read(addr);
      sample += (uint16_t)t << 8;

      ++addr;
    
      CODEC.sampleWrite(sample);
    
  } // End of for loop

} // End of play_wav_from_SRAM()


//----------------------------------------------------------------------------
// play_wav_file_on_sd()
//
// Parameters:
//      file_name: file name on the microSD card to be played
//
// Return Value:
//      None
//
// Remarks:
//      play audio samples stored in the SRAM
//----------------------------------------------------------------------------

void play_wav_file_on_sd (uint8_t *file_name)
{ 
    uint8_t t;

    if (!SD_available) {
       Serial.write (" microSD is not available\n");
       return;
    }

    Serial.write ("Playing ");
    Serial.write (file_name);
    
    t = SD.fopen(file_name);
    
    if (t) {
      Serial.write("\n SD open fail\n");
    } else {
      // Serial.write("\n SD open success\n");
    }
  
    t = parse_wav_file_head();

  
    if (t) {
        Serial.write("\n wav file open error\n");
        return ;
    } else {
      // Serial.write ("\n Playing ... \n");
    }

    Serial.write (" ...");
    load_wav_into_SRAM();
    play_wav_from_SRAM();

    Serial.write (" Done\n");
    
} // End of play_wav_file_on_sd()

uint8_t* digit_file_name[] = {"ZERO.WAV", 
                              "ONE.WAV", 
                              "TWO.WAV", 
                              "THREE.WAV", 
                              "FOUR.WAV",
                              "FIVE.WAV",
                              "SIX.WAV",
                              "SEVEN.WAV",
                              "EIGHT.WAV",
                              "NINE.WAV"};




//============================================================================
// setup()
//============================================================================

void setup() {
  // get the chip ID

  uint8_t t, i; 
  Serial.begin(115200);
  delay(2000);

  while (Serial.available()) {
      Serial.read();
  }
  Serial.write ("========================================================\n");
  Serial.write ("== BIST V"BIST_VERSION" for PulseRain M10 \n");
  Serial.write ("========================================================\n\n ");

  Serial.write ("Chip ID ==>  ");
  
  CHIP_ID_DATA_CSR = 0xff;
  single_nop_delay();


  //---------------------------------------------------------------
  // CHIP ID
  //---------------------------------------------------------------
      
      for (i = 0; i < 8; ++i) {
        t = CHIP_ID_DATA_CSR;
        chip_id[i] = t;
        if (t < 16) {
          Serial.write("0");
        }
        
        Serial.print(t, HEX);
        if (i < 7) {
          Serial.write ("-");
        }
      } // End of for loop
    
  //---------------------------------------------------------------
  // MCU Revision
  //---------------------------------------------------------------
    
      t = MCU_REVISION;
      Serial.write ("\n MCU Rev ==>  ");
      Serial.println(t, HEX);

  //---------------------------------------------------------------
  // ADC
  //---------------------------------------------------------------
    
      ADC.begin();
      analogRead(0); // setup active ADC channel here
      attachIsrHandler (ADC_INT_INDEX, adc_isr_handler);
       
  //---------------------------------------------------------------
  // CODEC and SRAM
  //---------------------------------------------------------------
      
      CODEC.begin();
      SRAM.begin();
      CODEC.outputVolume(28);

  //---------------------------------------------------------------
  // Pin In/Out setting
  //---------------------------------------------------------------

      pinMode(LED_BUILTIN_GREEN, OUTPUT);
      pinMode(LED_BUILTIN_RED, OUTPUT);
    
      pinMode(PUSH_BUTTON_PIN, INPUT);
    

  //---------------------------------------------------------------
  // microSD
  //---------------------------------------------------------------
    
      t = SD.begin();
    
      if (t) {
          Serial.write("\n microSD init failed!!!!!!!!!!!\n");
          SD_available = 0;
      } else {
          Serial.write("\n microSD init succeeded!!!!!!\n");
          SD_available = 1;
      }

  //---------------------------------------------------------------
  // DTMF
  //---------------------------------------------------------------

      DTMF.begin();

  

  prev_button_status = 1;
  test_step = 0;

} // End of setup()



//============================================================================
// loop()
//============================================================================

void loop() {

  uint8_t  button = prev_button_status, t1, t2;
  int8_t t;
  uint16_t tmp16;
 

 
  switch (test_step) {

      case 1: // Test for LED and ADC
           noInterrupts();
            tmp16 = adc_value;
          interrupts();
          Serial.write (" ADC = "); 
          Serial.println(tmp16);
          tmp16 >>= 2;
          tmp16 += 200;          
          test_led (tmp16);

          button = button_read();
          
          break;


      case 2: // Test for DTMF, CODEC, SRAM and microSD
          t1 = digitalRead(PUSH_BUTTON_PIN);
          t = DTMF.decode();
          if ((t >= 0) && (t <= 9)) {
              Serial.write ("DTMF Key ");
              Serial.print(t);
              Serial.write (", ");
              play_wav_file_on_sd(digit_file_name[t]);
          } else if (t == DTMF_ASTERISK) {
              Serial.write ("DTMF Key *, ");
              play_wav_file_on_sd ("HELLO.WAV");
          } else if (t == DTMF_POUND) {
              Serial.write ("DTMF Key #, ");
              play_wav_file_on_sd ("BYE.WAV");
          }
          
          t2 = digitalRead(PUSH_BUTTON_PIN);

          if (t1 == t2) {
            button = t1;
          }
          
          break;
         
  
      case 5:
          t1 = digitalRead(PUSH_BUTTON_PIN);
          t = DTMF.decode();

      default:
          digitalWrite(LED_BUILTIN_GREEN, LOW);   
          digitalWrite(LED_BUILTIN_RED, LOW);     
          button = button_read();  
   }

   // push button detection
   if ((!button) && (prev_button_status)) {
      test_step = (test_step + 1) % TOTAL_NUM_OF_TEST_STEPS;
      print_test_message ();

   }
   
   prev_button_status = button;

} // End of loop()

