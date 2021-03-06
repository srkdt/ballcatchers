#include <Arduino.h>

#include <Adafruit_I2CDevice.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1351.h>
#include <SPI.h>
#include <esp_now.h>
#include <WiFi.h>
#include <ESP32Servo.h>
#include <FastLED.h>

// Screen dimensions
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 128

// Color definitions
#define BLACK 0x0000
#define BLUE 0x001F
#define RED 0xF800
#define GREEN 0x07E0
#define CYAN 0x07FF
#define MAGENTA 0xF81F
#define YELLOW 0xFFE0
#define WHITE 0xFFFF

// OLED pins
#define MOSI_PIN 2 //SI
#define SCLK_PIN 4 //CL
#define DC_PIN 5   //DC
#define RST_PIN 18 //R
#define CS_PIN 19  //SC

// Pins for 7 Segment Display
#define RCLK_PIN 13  //pin 13 (latch pin) 1
#define SER_PIN 14   //pin 12 (datapin) 2
#define SRCLK_PIN 12 //pin 14 (clock pin) 3

// Pins for the UI buttons
#define NEXTBUTTON 23
#define SELECTBUTTON 22

// Pins for limit switches used for Hand detection
#define LEFTHAND 26
#define RIGHTHAND 33

// Pins for Servos used for the release mechanism
#define RIGHTSERVOPIN 25
#define LEFTSERVOPIN 27

// Pins for photo sensors used for ball detection
#define RIGHTBALLPIN 35
#define LEFTBALLPIN 32

// LED settings
#define NUM_LEDS 21
#define LED_PIN 15 // check!
#define BRIGHTNESS 200

// Variables for 7 Segment Display
const int datArray[11] = {63, 6, 91, 79, 102, 109, 125, 7, 127, 111, 128}; //base 10 representations of bits for 0,1,2,3,4,5,6,7,8,9,.
const byte digitArray[4] = {0b1000, 0b0100, 0b0010, 0b0001};               //digit number (first four bits in the bit shifter)
int reactionTime = 0;
int reactionTimeDisplayed[4];
int refreshRate = 100; // Hz
unsigned long processorTime;
unsigned long previousTime = 0;
unsigned long elapsedTime = 0; //elapsed time between digits are displayed

// Variables for Servos
Servo rightServo; //Create servo object to control right servo
Servo leftServo;  //Create servo object to control left servo
int posRightServo = 0;
int posLeftServo = 0;
unsigned long servoUp = 0;

// time variables for Ball drop
uint16_t timeToRelease = 0;
uint16_t dropTime = 0;
uint16_t catchTime = 0;
uint16_t bestTime = 0;

String catchTimeString;

// LED color array:
CRGB leds[NUM_LEDS];

// FOR hsv rainbow:
uint8_t hue = 0;

bool catchMode = false;    // loop condition variable
int difficultyCounter = 0; // variable for difficulty selection

// Functions declararion: definition below
void DifficultySelection();
void displayDifficulty();
void playPrompt();
void timeGenerator(int mode);
void DropBall();
void handsDelay(int timeToDrop);
void lcdTestPattern(void);
void servosback();
void displayScore();
void writeToDigit(int digitNumber, int number);
void rainbow();
void handsLEDs();
void notHandsLEDs();
void betterLEDs();
void worseLEDs();
void oledDisplayCenter(String text, int cursorHeight);
int createDropTime(int min, int max);
bool handsOn();
bool ballsHang();
bool ballsPlaced();
bool fingersOn();

// --- ESP-NOW stuff ---
typedef struct struct_message // Structure to receive data
{
    bool ballSignal;
    bool restart;
} struct_message;

// struct_message called myData
struct_message myData;

// variable affected by ball catching
bool caught = false;

// OLED display
Adafruit_SSD1351 tft = Adafruit_SSD1351(SCREEN_WIDTH, SCREEN_HEIGHT, CS_PIN, DC_PIN, MOSI_PIN, SCLK_PIN, RST_PIN);

// callback function that will be executed when data is received
void OnDataRecv(const uint8_t *mac_addr, const uint8_t *incomingData, int len)
{
    memcpy(&myData, incomingData, sizeof(myData));
    if (myData.ballSignal == false) // set if ball goes to sleep
    {
        tft.fillRect(0, 15, 128, 15, BLACK);
        tft.setTextColor(RED);
        tft.setTextSize(1);
        oledDisplayCenter("BALLS WENT TO SLEEP", 20);
    }
    if (myData.restart == true) // set at esp32_balls.cpp setup()
    {
        tft.fillRect(0, 15, 128, 15, BLACK);
        tft.setTextColor(RED);
        tft.setTextSize(1);
        oledDisplayCenter("BALL ON", 20);
    }
    else
        caught = true;
}

void setup()
{
    ///.begin(115200);

    // Set device as a Wi-Fi Station
    WiFi.mode(WIFI_STA);

    // Init ESP-NOW
    if (esp_now_init() != ESP_OK)
    {
        ///.println("Error initializing ESP-NOW");
        return;
    }

    // Register for recv CB to get recv packer info
    esp_now_register_recv_cb(OnDataRecv);

    // 7 Segment Display stuff
    pinMode(SER_PIN, OUTPUT);
    pinMode(RCLK_PIN, OUTPUT);
    pinMode(SRCLK_PIN, OUTPUT);

    pinMode(SELECTBUTTON, INPUT_PULLUP);
    pinMode(NEXTBUTTON, INPUT_PULLUP);
    pinMode(LEFTHAND, INPUT_PULLUP);
    pinMode(RIGHTHAND, INPUT_PULLUP);
    pinMode(RIGHTBALLPIN, INPUT);
    pinMode(LEFTBALLPIN, INPUT);

    rightServo.attach(RIGHTSERVOPIN);
    leftServo.attach(LEFTSERVOPIN);

    FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS)
        .setCorrection(TypicalLEDStrip)
        .setDither(BRIGHTNESS < 255);
    FastLED.setBrightness(BRIGHTNESS);

    tft.begin();

    servosback();

    lcdTestPattern(); // animation on boot - pride yeh
    delay(1000);

    tft.fillScreen(BLACK);
    oledDisplayCenter("Place balls in", 20);
    oledDisplayCenter("charging position", 30);
    oledDisplayCenter("to activate them!", 40);

    delay(5000);

    handsLEDs();

    tft.fillScreen(BLACK);
    tft.setTextColor(GREEN);
    tft.setTextSize(1);
    oledDisplayCenter("--- BAllCATCHERZ ---", 5);

    for (int j = 1; j < 5; j++)
    {
        writeToDigit(j, 0);
        delay(1000 / (refreshRate * 4));
    }
    writeToDigit(5, 0);

    DifficultySelection(); // go in difficulty selection at least once at boot

    myData.ballSignal = true;
    myData.restart = false;
}

void loop()
{
    if (catchMode == false)
    {
        notHandsLEDs();
        playPrompt();

        while (1)
        {
            if (digitalRead(NEXTBUTTON) == LOW) // option to set difficulty with right button press
            {
                DifficultySelection();
                playPrompt();
                break;
            }
            else if ((handsOn()) && (ballsHang())) // press hands to play if balls hang
            {
                break;
            }
        }

        timeGenerator(difficultyCounter); // select random time *after* difficulty selection
        handsDelay(timeToRelease);        // checks user's hands position for given amt of time
        catchMode = true;                 // go into play mode: don't set a new time, etc
        caught = false;                   // make sure the ball hasn't set the caught variable by mistake
        DropBall();                       // ball drops - also assigns millis() to dropTime
    }

    if (caught == true)
    {
        catchTime = (millis() - dropTime);
        servosback();
        displayScore();
        catchMode = false;
        caught = false;
    }
}

bool handsOn() // returns true if hands are in place
{
    if (!digitalRead(LEFTHAND) && !digitalRead(RIGHTHAND))
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool ballsHang() // returns true if balls hang in place
{
    if ((analogRead(LEFTBALLPIN) < 20) && (analogRead(RIGHTBALLPIN) < 20))
    {
        return true;
    }
    else
        return false;
}

void DifficultySelection()
{
    ///.println("--- Difficutly selection ---");
    tft.fillRect(0, 18, 128, 110, BLACK);
    tft.setCursor(0, 50);
    tft.setTextColor(BLUE);
    tft.setTextSize(1);
    tft.println("<- Change difficulty");
    tft.setCursor(0, 70);
    tft.print("           Confirm ->");

    tft.setCursor(0, 80);
    tft.setTextSize(2);
    displayDifficulty();

    while (digitalRead(SELECTBUTTON)) // click select (left) button to exit loop
    {
        if (digitalRead(NEXTBUTTON) == LOW) // toggle difficulty levels
        {
            difficultyCounter++;

            if (difficultyCounter == 3)
            {
                difficultyCounter = 0;
            }

            tft.fillRect(0, 80, 128, 48, BLACK);
            tft.setCursor(0, 80);
            tft.setTextSize(2);
            displayDifficulty();

            delay(200); // avoid bouncing
        }
        if (handsOn()) // allow user to quit by placing hands
        {
            break;
        }
    }
}

void displayDifficulty()
{
    switch (difficultyCounter) // display difficulty (during game)
    {
    case 0: // Easy Mode
        oledDisplayCenter("EASY", 100);
        break;
    case 1: // Normal Mode
        oledDisplayCenter("NORMAL", 100);
        break;
    case 2: // Hard Mode
        oledDisplayCenter("HARD", 100);
        break;
    }
}

void playPrompt()
{
    tft.fillRect(0, 20, 128, 102, BLACK);
    tft.setCursor(0, 50);
    tft.setTextColor(BLUE);
    tft.setTextSize(1);
    tft.println("<- Change difficulty");
    oledDisplayCenter("Place hands to play", 70);
    tft.setTextSize(2);
    displayDifficulty();
}

void timeGenerator(int mode)
{
    switch (mode)
    {
    case 0: // Easy Mode
        timeToRelease = random(1800, 2200);
        break;
    case 1: // Normal Mode
        timeToRelease = random(1000, 3000);
        break;
    case 2: // Hard Mode
        timeToRelease = random(200, 8000);
        break;
    }
}

void handsDelay(int timeToDrop)
{
    bool ok = false;
    while (!ok)
    {
        uint32_t millisAtStart = millis();
        while (handsOn() && !ok)
        { // both hands to continue
            uint32_t currentMillis = millis();
            if (currentMillis - millisAtStart > timeToDrop)
            {
                ok = true;
            }
            rainbow();
        }
    }
}

void servosback()
{
    rightServo.write(86);
    leftServo.write(114);
}

void DropBall()
{
    bool dropRightBall = random(0, 2);
    if (dropRightBall)
    {
        rightServo.write(110);
        ///.println("\tRight Ball dropped yeh");
    }
    else
    {
        leftServo.write(90);
        ///.println("\tLeft Ball dropped yeh");
    }
    dropTime = millis();
}

void lcdTestPattern(void)
{
    static const uint16_t PROGMEM colors[] =
        {RED, YELLOW, GREEN, CYAN, BLUE, MAGENTA, BLACK, WHITE};

    for (uint8_t c = 0; c < 8; c++)
    {
        tft.fillRect(0, tft.height() * c / 8, tft.width(), tft.height() / 8,
                     pgm_read_word(&colors[c]));
        delay(100);
    }
}

void displayScore()
{
    tft.fillRect(0, 40, 128, 90, BLACK);
    tft.setTextSize(1);
    tft.setCursor(0, 40);
    oledDisplayCenter("Score:", 30);

    if (catchTime < bestTime || bestTime == 0)
    {
        bestTime = catchTime;
        tft.setTextColor(GREEN); // better score is green
    }
    else
        tft.setTextColor(RED); // worse score is red

    tft.setTextSize(2); // score bigger size
    catchTimeString = String(catchTime);
    oledDisplayCenter(catchTimeString, 50);

    tft.setTextColor(WHITE);
    tft.setTextSize(1);
    tft.print("\n\nBest score: ");
    tft.println(bestTime); // best score printed in white
    tft.print("\nPlace hands and balls\nto continue...");

    reactionTimeDisplayed[0] = catchTime / 1000; // array to be pushed to digits
    reactionTimeDisplayed[1] = (catchTime / 100) % 10;
    reactionTimeDisplayed[2] = (catchTime / 10) % 10;
    reactionTimeDisplayed[3] = catchTime % 10;

    while (!((handsOn()) && (ballsHang())))
    {
        if (catchTime == bestTime)
        {
            betterLEDs();
        }
        else if (catchTime > bestTime)
        {
            worseLEDs();
        }
        if (catchTime <= 9999)
        {
            for (int j = 1; j < 5; j++)
            {
                writeToDigit(j, reactionTimeDisplayed[j - 1]);
                delay(1000 / (refreshRate * 4));
            }
        }
    }
    writeToDigit(5, 0);
}

void writeToDigit(int digitNumber, int number) // select which digit to write to and what to write in it
{
    digitNumber = digitNumber - 1;                                   // shift in new bits for number to be written
    digitalWrite(RCLK_PIN, LOW);                                     // ground latchPin and hold low for as long as data is transmitted
    shiftOut(SER_PIN, SRCLK_PIN, MSBFIRST, digitArray[digitNumber]); // shift in which digit to write to
    shiftOut(SER_PIN, SRCLK_PIN, MSBFIRST, datArray[number]);        // shift in the actual number
    digitalWrite(RCLK_PIN, HIGH);                                    // pull the latchPin clockPin to save the data
}

void notHandsLEDs()
{
    FastLED.clear(); // clear all pixel data

    for (int i = 0; i < 6; i++)
    {
        leds[i] = CRGB::OrangeRed;
    }

    for (int i = 15; i < NUM_LEDS; i++)
    {
        leds[i] = CRGB::OrangeRed;
    }
    delay(1);
    FastLED.show();
}

void handsLEDs()
{
    FastLED.clear(); // clear all pixel data

    for (int i = 0; i < 6; i++)
    {
        leds[i] = CRGB::Aqua;
    }

    for (int i = 15; i < NUM_LEDS; i++)
    {
        leds[i] = CRGB::Aqua;
    }
    delay(1);
    FastLED.show();
}

void rainbow()
{
    for (int i = 0; i < NUM_LEDS; i++)
    {
        leds[i] = CHSV(hue + (i * 10), 255, 255);

        EVERY_N_MILLISECONDS(5)
        {
            hue++;
        }
        delay(1);
        FastLED.show();
    }
}

void betterLEDs()
{
    uint8_t sinBeat = beatsin8(30, 0, NUM_LEDS - 1, 0, 0);
    uint8_t sinBeat2 = beatsin8(30, 0, NUM_LEDS - 1, 0, 85);
    uint8_t sinBeat3 = beatsin8(30, 0, NUM_LEDS - 1, 0, 170);

    leds[sinBeat] = CRGB::Green;
    leds[sinBeat2] = CRGB::DarkGreen;
    leds[sinBeat3] = CRGB::LimeGreen;

    fadeToBlackBy(leds, NUM_LEDS, 4);

    delay(1);
    FastLED.show();
}

void worseLEDs()
{
    uint8_t sinBeat = beatsin8(30, 0, NUM_LEDS - 1, 0, 0);
    uint8_t sinBeat2 = beatsin8(30, 0, NUM_LEDS - 1, 0, 85);
    uint8_t sinBeat3 = beatsin8(30, 0, NUM_LEDS - 1, 0, 170);

    leds[sinBeat] = CRGB::Red;
    leds[sinBeat2] = CRGB::OrangeRed;
    leds[sinBeat3] = CRGB::DarkRed;

    fadeToBlackBy(leds, NUM_LEDS, 4);

    delay(1);
    FastLED.show();
}

void oledDisplayCenter(String text, int cursorHeight)
{
    int16_t x1;
    int16_t y1;
    uint16_t width;
    uint16_t height;

    tft.getTextBounds(text, 0, 0, &x1, &y1, &width, &height);

    // display on horizontal and vertical center
    tft.setCursor((SCREEN_WIDTH - width) / 2, cursorHeight);
    tft.println(text); // text to display
}
