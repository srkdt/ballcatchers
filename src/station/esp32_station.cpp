#include <Arduino.h>

#include <Adafruit_I2CDevice.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1351.h>
#include <SPI.h>

//This version of the code uses the screen and 4 buttons.
//2 for the Next/Select functionality
//2 to simulate the Hand pads

// Screen dimensions
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 128 // Change this to 96 for 1.27" OLED.

// You can use any (4 or) 5 pins
#define SCLK_PIN 2
#define MOSI_PIN 4
#define DC_PIN 5
#define CS_PIN 18
#define RST_PIN 19

//Pins for the UI buttons
#define NextButton 23
#define SelectButton 22

//Pins for "Hand" buttons
#define LeftHand 33
#define RightHand 32

//Pins for LEDs for simulation purposes
#define redLED 13   //when both hands are place
#define blueLED 12  //when right ball drops
#define greenLED 14 //when left ball drops

//Variables for Ball drop
uint32_t timeToBallDrop = 0;
uint32_t processorTime = 0;
uint32_t previousTime = 0;
uint32_t elapsedTime = 0;

//Button State
int StateNext = 0;
int LastStateNext = 0;
int CounterNext = 0; //used for difficulty settings
int StateSelect = 0;
int LastStateSelect = 0;
int CounterSelect = 0;
int StateLeftHand = 0;
int StateRightHand = 0;

// Color definitions
#define BLACK 0x0000
#define BLUE 0x001F
#define RED 0xF800
#define GREEN 0x07E0
#define CYAN 0x07FF
#define MAGENTA 0xF81F
#define YELLOW 0xFFE0
#define WHITE 0xFFFF

// Option 1: use any pins but a little slower
Adafruit_SSD1351 tft = Adafruit_SSD1351(SCREEN_WIDTH, SCREEN_HEIGHT, CS_PIN, DC_PIN, MOSI_PIN, SCLK_PIN, RST_PIN);

//Function Declaration
int DifficultySelection();
void DropBall(uint32_t time);
int createDropTime(int min, int max);

void setup()
{
    pinMode(SelectButton, INPUT_PULLUP);
    pinMode(NextButton, INPUT_PULLUP);
    pinMode(LeftHand, INPUT_PULLUP);
    pinMode(RightHand, INPUT_PULLUP);
    pinMode(redLED, OUTPUT);
    pinMode(blueLED, OUTPUT);
    pinMode(greenLED, OUTPUT);

    tft.begin();

    tft.setCursor(0, 5);
    tft.fillScreen(BLACK);
    tft.setTextColor(GREEN);
    tft.setTextSize(1);
    tft.println("--- GRAVITY BALLS ---");

    Serial.begin(9601);
}

void loop()
{
    processorTime = millis();

    DifficultySelection();
    
    //Readout of the handsensors (here simulated as PullUp Buttons)
    StateLeftHand = digitalRead(LeftHand);
    StateRightHand = digitalRead(RightHand);

    //When both hands are in place, drop ball according to difficulty mode
    if (StateLeftHand == LOW && StateRightHand == LOW)
    {
        digitalWrite(redLED, HIGH); //Red light signals both hands are placed
        //tft.println("Get ready!");
        switch (CounterNext)
        {
        case 0: // Easy Mode
            //timeToBallDrop = random(800, 1000);
            timeToBallDrop = createDropTime(800, 1000);
            break;
        case 1: // Normal Mode
            //timeToBallDrop = random(500, 1500);
            timeToBallDrop = createDropTime(500, 1500);
            break;
        case 2: // Hard Mode
            //timeToBallDrop = random(200, 2000);
            timeToBallDrop = createDropTime(200, 2000);
            break;
        }
        elapsedTime = processorTime - previousTime;
        DropBall(timeToBallDrop);
    }
    else
    {
        digitalWrite(redLED, LOW);
        digitalWrite(blueLED, LOW);
        digitalWrite(greenLED, LOW);
        previousTime = processorTime;
    }
}

int DifficultySelection()
{
    StateNext = digitalRead(NextButton);
    if (CounterSelect == 0) //If the select Button has never been pressed, then allow user to choose difficulty
    {        
        tft.setCursor(0, 30);
        tft.setTextColor(BLUE);
        tft.setTextSize(1);
        tft.println("Choose difficulty");
        
        if (StateNext != LastStateNext) //If State of the Next Button changes
        {
            if (StateNext == LOW)
            {
                CounterNext++; //The current value +1
                tft.fillRect(0, 38, 128, 8, BLACK);
            }
        }
        LastStateNext = StateNext; //Set Last state to current state, since the change has happened and it needs to be logged

        switch (CounterNext) //CounterNext used as variable for difficulty selection
        {
        case 0: //Easy Mode
            tft.println("Easy");
            break;
        case 1: //Normal Mode
            tft.println("Normal");
            break;
        case 2: //Hard Mode
            tft.println("Hard");
            break;
        case 3: //Go back to Easy after Hard by setting CounterNext to 0
            CounterNext = 0;
            break;
        }
    }

    //State change recognition of the Select Button
    StateSelect = digitalRead(SelectButton);
    if (StateSelect != LastStateSelect)
    {
        if (StateSelect == LOW)
        {
            CounterSelect++;
        }
    }
    LastStateSelect = StateSelect;

    //After Select Button was pressed, stop difficulty selection with Next Button and display selected dificulty on screen
    while (CounterSelect == 1)
    {
        tft.println("Playing in");
        switch (CounterNext)
        {
        case 0: // Easy Mode
            tft.println("Easy mode");
            Serial.println("***Easy mode selected***");
            break;
        case 1: // Normal Mode
            tft.println("Normal mode");
            Serial.println("***Normal mode selected***");
            break;
        case 2: // Hard Mode
            tft.println("Hard mode");
            Serial.println("***Hard mode selected***");
            break;
        }
        CounterSelect++;
    }
    if (digitalRead(SelectButton) == LOW && digitalRead(NextButton) == LOW)
    {
        tft.fillRect(0, 38, 128, 30, BLACK);
        CounterSelect = 0;
        CounterNext = 0;
        delay(100);
    }
    return CounterNext;
    return timeToBallDrop;
}

int createDropTime(int min, int max)
{
    int randomDroptimer = random(min, max);
    /* Serial.print("randomDroptimer ");
    Serial.println(randomDroptimer); */
    return randomDroptimer;
}

void DropBall(uint32_t time)
{
    uint32_t DropTimer = time;
    Serial.print("Drop timer: ");
    Serial.print(DropTimer);
    Serial.print("   ||   elapsed time: ");
    Serial.println(elapsedTime);
    if (elapsedTime > DropTimer)
    {
        bool dropRightBall = random(0, 2);
        if (dropRightBall == true)
        {
            digitalWrite(blueLED, HIGH); // Right Ball drops
            digitalWrite(greenLED, LOW);
            Serial.print("Right Ball dropped at ");
            Serial.println(DropTimer);
        }
        if (dropRightBall != true)
        {
            digitalWrite(greenLED, HIGH); // Left Ball drops
            digitalWrite(blueLED, LOW);
            Serial.print("Left Ball dropped at ");
            Serial.println(DropTimer);
        }
        previousTime = processorTime;
    }
}
