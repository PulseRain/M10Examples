/*
###############################################################################
# Copyright (c) 2016, PulseRain Technology LLC 
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


//----------------------------------------------------------------------------
// isDigit()
//
// Parameters:
//      c : char value input
//
// Return Value:
//      1 if the value input is in [0-9]
//
// Remarks:
//      determine if the input char is a digit
//----------------------------------------------------------------------------

uint8_t isDigit(uint8_t c)
{
  if ((c >= '0') && (c <= '9')) {
    return 1;
  } 

  return 0;
  
} // End of isDigit()


//----------------------------------------------------------------------------
// is_valid_expression_char()
//
// Parameters:
//      c : char value input
//
// Return Value:
//      1 if the value input is a valid character
//
// Remarks:
//      determine if the input char is a digit
//----------------------------------------------------------------------------

uint8_t is_valid_expression_char(uint8_t c)
{
  if (isDigit(c) || (c == '(') || (c == ')') || (c == '-') ||
    (c == '+') || (c == '*') || (c == '/') || (c == '\r')) {
    return 1; 
  } 

  return 0;
  
} // End of is_valid_expression_char()

#define DIGIT_BUFFER_SIZE 16

uint8_t digit_buffer[DIGIT_BUFFER_SIZE];
uint8_t digit_buffer_index = 0;

uint8_t current_operator_level = 0;
uint8_t negative_sign = 0;


//----------------------------------------------------------------------------
// push_digit_in()
//
// Parameters:
//      c : char value input
//
// Return Value:
//      None
//
// Remarks:
//      push input char into the buffer
//----------------------------------------------------------------------------

void push_digit_in(uint8_t c)
{
  if (!isDigit(c)) {
    return;
  }
    
  if (current_operator_level == 0) {
    current_operator_level = 1;
  }

  if (digit_buffer_index < (DIGIT_BUFFER_SIZE - 1)) {
    digit_buffer[digit_buffer_index++] = c;
  }
  
} // End of push_digit_in()

#define OPERATOR_BUFFER_SIZE 128
uint8_t operator_buffer[OPERATOR_BUFFER_SIZE];
uint8_t operator_buffer_index = 0;

//----------------------------------------------------------------------------
// operator_level()
//
// Parameters:
//      c : char value input
//
// Return Value:
//      operator level for the input char
//
// Remarks:
//      determine the operator level of the input character
//----------------------------------------------------------------------------
uint8_t operator_level(uint8_t c)
{
  uint8_t level = 0;

  switch (c) {
      case '+':
      case '-':
        level = 3;
        break;

      case '*':
      case '/':
        level = 2;
        break;

      case '(':
        level = 0;
        break;

      case ')':
        level = 127;
        break;

      case '\r':
        level = 128;
        break;

      default:
        level = 0xff;
  } // End of switch

  return level;
  
} // End of operator_level()

//----------------------------------------------------------------------------
// push_operator_in()
//
// Parameters:
//      c : char value input
//
// Return Value:
//      None
//
// Remarks:
//      push operator into the operator buffer
//----------------------------------------------------------------------------

void push_operator_in(uint8_t c)
{
  uint8_t valid_operator = 0;
  uint8_t level;
  
  if ((current_operator_level == 0) && (c == '-')) {
    negative_sign = 1;
  } else {

    level = operator_level(c);

    if (level != 0xff) {
      valid_operator = 1;
      current_operator_level = level;
    }
    
  }

  if ((operator_buffer_index < OPERATOR_BUFFER_SIZE) && (valid_operator)) {
    operator_buffer[operator_buffer_index++] = c;
  }
  
} // End of push_operator_in()

//----------------------------------------------------------------------------
// extract_number()
//
// Parameters:
//      None
//
// Return Value:
//      number extracted
//
// Remarks:
//      extract number from the input buffer
//----------------------------------------------------------------------------

int32_t extract_number()
{
  int32_t temp_val = 0;
  uint8_t index = 0;

  while (index < digit_buffer_index) {
    temp_val = temp_val * 10 + (digit_buffer[index] - '0');
    ++index;
  } // End of while loop

  digit_buffer_index = 0;

  //printf("\n=================> %d\n", temp_val);

  if (negative_sign) {
    temp_val = -temp_val;
  }

  negative_sign = 0;

  return temp_val;

} // End of extract_number()

int32_t num_stack[256];
uint8_t num_stack_index = 0;


//----------------------------------------------------------------------------
// push_num()
//
// Parameters:
//      num : data to be pushed onto stack
//
// Return Value:
//      None
//
// Remarks:
//      push number onto the stack
//----------------------------------------------------------------------------

void push_num(int32_t num)
{
  if (num_stack_index != 255) {
    num_stack[num_stack_index++] = num;
  }
} // End of push_num()

//----------------------------------------------------------------------------
// pop_num()
//
// Parameters:
//      None
//
// Return Value:
//      number popped from the stack 
//
// Remarks:
//      pop number out of the stack
//----------------------------------------------------------------------------

int32_t pop_num()
{
  if (!num_stack_index) {
    return 0;
  }

  return (num_stack[--num_stack_index]);
  
} // End of pop_num()


uint8_t input_buffer[256];
uint8_t input_buffer_index = 0;

//----------------------------------------------------------------------------
// get_expression()
//
// Parameters:
//      None
//
// Return Value:
//      None
//
// Remarks:
//      function to handle input line      
//----------------------------------------------------------------------------

void get_expression()
{
  uint8_t c;
    
  input_buffer_index = 0;
  num_stack_index = 0;
  digit_buffer_index = 0;
  current_operator_level = 0;
  negative_sign = 0;
  operator_buffer_index = 0;

  do {
        while (!Serial.available()); 
        c = Serial.read();
        //Serial.readBytes(&c, 1);
        
        if (c == 'H') { // up arrow
            Serial.write ("\b", 1);
            Serial.write (" ", 1);
            Serial.write ("\b", 1);
            
            while (input_buffer[input_buffer_index] && (input_buffer[input_buffer_index] != '\r') && (input_buffer_index < 255)) {
                Serial.write (&input_buffer[input_buffer_index++], 1);
            } // End of while loop                
            
            
        } else if ((c == 8) && (input_buffer_index)) {
            //serial.writeByte('\b');
            Serial.write(" ", 1);
            Serial.write("\b", 1);
            --input_buffer_index;
        } else if (input_buffer_index < 255) {
            input_buffer[input_buffer_index++] = c;
        }
  } while (c != '\r');
  
  input_buffer[input_buffer_index] = 0;

} // End of get_expression()

//----------------------------------------------------------------------------
// print_prompt()
//
// Parameters:
//      None
//
// Return Value:
//      None
//
// Remarks:
//      function to print prompt for input
//----------------------------------------------------------------------------

void print_prompt()
{
    Serial.write ("Please input an expression to evaluate:\r\n", 41);
} // End of print_prompt()

//----------------------------------------------------------------------------
// error_message()
//
// Parameters:
//      None
//
// Return Value:
//      None
//
// Remarks:
//      function to print error message
//----------------------------------------------------------------------------

void error_message()
{
    Serial.write("Expression error!\r\n", 19);
} // End of error_message()


//----------------------------------------------------------------------------
// calc_process()
//
// Parameters:
//      None
//
// Return Value:
//      0      if the input expression is valid
//      0xFF   if expression evaluation fails
//
// Remarks:
//      function to evaluate the expression input
//----------------------------------------------------------------------------

uint8_t calc_process()
{
  uint8_t c, p;
  int32_t a, b;

  if (operator_buffer_index < 2) {
    return 0;
  }

  c = operator_buffer[--operator_buffer_index];
    
  while (operator_buffer_index) {
    p = operator_buffer[operator_buffer_index - 1];

    if (operator_level(p) <= operator_level(c)) {
      switch (p) {
        case '+':
        case '-':
          if (num_stack_index < 2) {
            return 0xFF;
          }

          b = pop_num();
          a = pop_num();
          //printf("\n====>  %d %c %d \n", a, p, b);
          if (p == '+') {
            push_num(a + b);
          } else {
            push_num(a - b);
          }

          --operator_buffer_index;
          break;

        case '*':
        case '/':
          if (num_stack_index < 2) {
            return 0xFF;
          }

          b = pop_num();
          a = pop_num();
          
          //printf("\n====>  %d %c %d \n", a, p, b);

          if (p == '*') {
            push_num(a * b);
          }
          else {
            push_num(a / b);
          }

          --operator_buffer_index;
          break;

        case '(':
          //printf("\n====> ((( \n");
          if (c == ')') {
            --operator_buffer_index;
          } else {
            operator_buffer[operator_buffer_index++] = c;
            
          }

          return 0;

        default:
          return 0xff;
      } // End of switch

    } else {
      operator_buffer[operator_buffer_index++] = c;
      break;
    }

  } // End of while loop

  if ((!operator_buffer_index) && (c != '\r')) {
     operator_buffer[operator_buffer_index++] = c;
  }
  
  return 0;

} // End of calc_process()


//----------------------------------------------------------------------------
// setup()
//
// Parameters:
//      None
//
// Return Value:
//     None
//
// Remarks:
//      setup()
//----------------------------------------------------------------------------

void setup() {
  Serial.begin(115200);
  delay(1000);
} // End of setup()

//----------------------------------------------------------------------------
// loop()
//
// Parameters:
//      None
//
// Return Value:
//     None
//
// Remarks:
//      loop()
//----------------------------------------------------------------------------

void loop()
{
    uint8_t i, c;

    print_prompt();
    get_expression();
    Serial.write ("\r", 1);
    Serial.write ("\n", 1);
    Serial.write ("\r", 1);
    Serial.write ("\n", 1);
    
    for (i = 0; i < input_buffer_index; ++i) {

        c = input_buffer[i];
        
        if (is_valid_expression_char(c)) {
            push_digit_in(c);
            push_operator_in(c);

            if (!isDigit(c)) {
              if (digit_buffer_index) {
                push_num(extract_number());
              }

              if (calc_process()) {
                error_message();
              }
            }
        }
    } // End of for
    
    if (operator_buffer_index && (operator_buffer[0] != '\r')) {
        error_message();
    } else {
      Serial.write("ans = ", 6);
      Serial.println(num_stack[0], DEC);
      Serial.write ("\r", 1);
      Serial.write ("\n", 1);
    }
    
} // End of loop()
