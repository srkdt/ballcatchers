#include <Arduino.h>

#define SER_Pin 14                                                         //pin 26 (datapin) 2
#define RCLK_Pin 32                                                        //pin 27 (latch pin) 1
#define SRCLK_Pin 15                                                       //pin 25 (clock pin) 3
const int datArray[11] = {63, 6, 91, 79, 102, 109, 125, 7, 127, 111, 128}; //base 10 representations of bits for 0,1,2,3,4,5,6,7,8,9,.
const byte digitArray[4] = {0b1000, 0b0100, 0b0010, 0b0001};               //digit number (first four bits in the bit shifter)
int reactionTime = 0;
int reactionTimeDisplayed[4];
int refreshRate = 60; //Hz
int counter = 1;      // for refreshing the display
void writeToDigit(int digitNumber, int number);

unsigned long processorTime; // in order to avoid using delay()
unsigned long previousTime = 0;
unsigned long elapsedTime = 0; //elapsed time between digits are displayed

void setup()
{
  pinMode(SER_Pin, OUTPUT);
  pinMode(RCLK_Pin, OUTPUT);
  pinMode(SRCLK_Pin, OUTPUT);
}

void loop()
{
  //create new random number every 2sec
  processorTime = millis();
  //fetch reaction time here

  //split number into individual digits
  reactionTimeDisplayed[0] = reactionTime / 1000;
  reactionTimeDisplayed[1] = (reactionTime / 100) % 10;
  reactionTimeDisplayed[2] = (reactionTime / 10) % 10;
  reactionTimeDisplayed[3] = reactionTime % 10;

  elapsedTime = processorTime - previousTime;
  if (elapsedTime >= 1000 / (refreshRate * 4))
  { //=4.1ms @ 60Hz
    if (counter < 5)
    {
      counter++;                                                 //jump to next display
      writeToDigit(counter, reactionTimeDisplayed[counter - 1]); //update display
      previousTime = processorTime;
    }
    else
    {
      counter = 1;
      previousTime = processorTime;
    }
  }
}
//select which digit to write to and what to write in it
void writeToDigit(int digitNumber, int number)
{
  digitNumber = digitNumber - 1;

  //shift in new bits for number to be written
  digitalWrite(RCLK_Pin, LOW);                                     //ground latchPin and hold low for as long as data is transmitted
  shiftOut(SER_Pin, SRCLK_Pin, MSBFIRST, digitArray[digitNumber]); //shift in which digit to write to
  shiftOut(SER_Pin, SRCLK_Pin, MSBFIRST, datArray[number]);        //shift in the actual number
  digitalWrite(RCLK_Pin, HIGH);                                    //pull the latchPin clockPin to save the data
}
