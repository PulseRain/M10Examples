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

#include "M10ADC.h"

//=============================================================================
// Example to read analog channel A0 and output the result to serial port
//
// On M10 board, set the IO voltage jumper (JP1) to 5V, and
// use a wire (both ends are male) to connect IOREF to A0.
// And use a screw driver to adjust the onboard potentiometer.
// The output value should change accordingly on the Serial Monitor
//
// Adapted from a similar example on Arduino board
//=============================================================================

#define A0 0

// the setup routine runs once when you press reset:
void setup() {
  // initialize serial communication at 115200bps
  Serial.begin(115200);
}

// the loop routine runs over and over again forever:
void loop() {
  // read the input on analog channel A0:
  uint16_t sensorValue = analogRead(A0);
  
  // print out the value you read:
  Serial.println(sensorValue);
  delay(1000);        // delay in between reads for stability
}

