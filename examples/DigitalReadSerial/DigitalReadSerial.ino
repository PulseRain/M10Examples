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
// DigitalReadSerial
//============================================================================
//  This example will print out the current MCU version and the chip ID.
// Then it will keep reading the push button status
//============================================================================



// total number of test steps
#define TOTAL_NUM_OF_TEST_STEPS 3

// Pin number for LEDs
#define LED_BUILTIN_GREEN 13
#define LED_BUILTIN_RED   15

// Pin for the push button (The button next to the DC power jack)
#define PUSH_BUTTON_PIN   11


// 64 bit chip id
uint8_t chip_id [8];


// for push button debouncer
uint8_t prev_button_status;

// led value
uint8_t led_value = 0;
 


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
  // Pin In/Out setting
  //---------------------------------------------------------------

      pinMode(LED_BUILTIN_GREEN, OUTPUT);
      pinMode(LED_BUILTIN_RED, OUTPUT);
    
      pinMode(PUSH_BUTTON_PIN, INPUT);
    
      digitalWrite(LED_BUILTIN_GREEN, LOW);   
      digitalWrite(LED_BUILTIN_RED, LOW);     
   

} // End of setup()



//============================================================================
// loop()
//============================================================================

void loop() {

  uint8_t  button = prev_button_status;
  
  button = button_read();

  if ((!button) && (prev_button_status)) {
      ++led_value;
      Serial.write("Button Pressed ");
      Serial.println(led_value);
      digitalWrite(LED_BUILTIN_GREEN, led_value & 1);   
      digitalWrite(LED_BUILTIN_RED, (led_value >> 1) & 1);     
  } 

   
   prev_button_status = button;

} // End of loop()

