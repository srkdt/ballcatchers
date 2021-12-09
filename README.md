# ballcatchers

## Abstract

As part of the Mechatronics Trinational curriculum, our team has developed a reaction-based ball catching game as 5th semester project. The device consists of a main station retaining two magnetically connected balls, which are realeased at a random interval. As the user lifts his hands from the station to catch the balls, a timer starts and stops when the balls are caught. This reaction time is then displayed to the user. ESP32s are used for data processing as well as communicating between the station and the balls.  
**This repo contains and details the code used in the device (mainly C++).**

<p align="center">
  <img src="https://github.com/srkdt/ballcatchers/tree/dev/media/Intro_Film_lowRes.gif"/>
</p>

## Acknowledgements
We would like to thank Mr Silvan Wirth for his support and advice all along the process of designing, manufacturing and programming the device.
*A special thanks also goes to Carlos the cat, without whom the project would never have seen the light of day.*

## Table of contents

1. [Programming the station](https://github.com/srkdt/ballcatchers#1-Programming-the-station)
  - [1.1 User interface](https://github.com/srkdt/ballcatchers#1-Programming-the-station/#11-Programming-the-interface)
    - [1.1.1 Mode selection](https://github.com/srkdt/ballcatchers#1-Programming-the-station/#111-Mode-selection)
    - [1.1.2 Reaction Time display](https://github.com/srkdt/ballcatchers#1-Programming-the-station/#112-Reaction-Time-display)
    - [1.1.3 LED feedback](https://github.com/srkdt/ballcatchers#1-Programming-the-station/#113-LED-feedback)
  - [1.2 Communication](https://github.com/srkdt/ballcatchers#1-Programming-the-station#12-Communication)
  - [1.3 Physical inputs (sensors)](https://github.com/srkdt/ballcatchers#1-Programming-the-station#13Physical-inputs-(sensors))
2. [Programming the balls](https://github.com/srkdt/ballcatchers#1-Programming-the-station#2-Programming-the-balls)
3. [Final design](https://github.com/srkdt/ballcatchers#1-Programming-the-station#3-Final-design)


# 1. Programming the station
## 1.1 User Interface
### 1.1.1 Mode selection
While playing the game, the user has the possibility of choosing a difficulty level. This difficulty level influences the time at which the balls are released and whether they are released simultaneously or not. For that purpose, an OLED display and two pushbuttons are built inside the station for the user to be able to modify the difficulty level. 

### 1.1.2 Reaction Time display
Given that the reaction time is the most important information to be displayed to the user whilst he is playing, it needs to be clearly communicated to the user. To that extent, an array of four 7-segments digits is used in the middle of the station to display the time. The following code snippets describe how to show the reaction time.

```C++
#define SER_Pin 14 //pin 14 (datapin) 2
#define RCLK_Pin 32 //pin 12 (latch pin) 1
#define SRCLK_Pin 15 //pin 11 (clock pin) 3
const int datArray[11] = {63, 6, 91, 79, 102, 109, 125, 7, 127, 111, 128};//base 10 representations of bits for 0,1,2,3,4,5,6,7,8,9,.
const byte digitArray[4] = {0b1000, 0b0100, 0b0010, 0b0001}; //digit number (first four bits in the bit shifter)
int reactionTime = 0;
int reactionTimeDisplayed[4];
int refreshRate = 60; //Hz
int counter = 1; // for refreshing the display

unsigned long processorTime;
unsigned long previousTime = 0;
unsigned long elapsedTime = 0; //elapsed time between digits are displayed


void setup() {
  pinMode(SER_Pin, OUTPUT);
  pinMode(RCLK_Pin, OUTPUT);
  pinMode(SRCLK_Pin, OUTPUT);
}

void loop() {
  //create new random number every 2sec
  processorTime = millis();
  //fetch reaction time here

  //split number into individual digits
  reactionTimeDisplayed[0] = reactionTime / 1000;
  reactionTimeDisplayed[1] = (reactionTime / 100) % 10;
  reactionTimeDisplayed[2] = (reactionTime / 10) % 10;
  reactionTimeDisplayed[3] = reactionTime % 10;

  elapsedTime = processorTime - previousTime;
  if (elapsedTime >= 1000 / (refreshRate * 4)) { //=4.1ms @ 60Hz
    if (counter < 5) {
      counter++; //jump to next display
      writeToDigit(counter, reactionTimeDisplayed[counter - 1]); //update display
      previousTime = processorTime;
    }
    else {
      counter = 1;
      previousTime = processorTime;
    }
  }

//select which digit to write to and what to write in it
void writeToDigit(int digitNumber, int number) {
  digitNumber = digitNumber - 1;

  //shift in new bits for number to be written
  digitalWrite(RCLK_Pin, LOW); //ground latchPin and hold low for as long as data is transmitted
  shiftOut(SER_Pin, SRCLK_Pin, MSBFIRST, digitArray[digitNumber]); //shift in which digit to write to
  shiftOut(SER_Pin, SRCLK_Pin, MSBFIRST, datArray[number]); //shift in the actual number
  digitalWrite(RCLK_Pin, HIGH); //pull the latchPin clockPin to save the data
}
```

### 1.1.3 LED feedback

## 1.2 Communication

## 1.3 Physical inputs

# 2. Programming the balls

# 3. Final design
-> add photos

