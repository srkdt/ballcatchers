#include <Arduino.h>

#include <Adafruit_I2CDevice.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1351.h>
#include <SPI.h>
#include <esp_now.h>
#include <WiFi.h>
#include <ESP32Servo.h>

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
#define SER_PIN 12   //pin 12 (datapin) 2
#define SRCLK_PIN 14 //pin 14 (clock pin) 3

// Pins for the UI buttons
#define NEXTBUTTON 23
#define SELECTBUTTON 22

// Pins for limit switches used for Hand detection
#define LEFTHAND 26
#define RIGHTHAND 33

// Pins for Servos used for the release mechanism
#define RIGHTSERVOPIN 25
#define LEFTSERVOPIN 27

// Pins for Hall sensors used for ball detection
#define RIGHTHALLSENSOR 35
#define LEFTHALLSENSOR 32

// TODO: Pin for LED Strip
//#define LEDSTRIP 15 //alternativly 21 and 34 are still free

// Variables for 7 Segment Display
const int datArray[11] = {63, 6, 91, 79, 102, 109, 125, 7, 127, 111, 128}; //base 10 representations of bits for 0,1,2,3,4,5,6,7,8,9,.
const byte digitArray[4] = {0b1000, 0b0100, 0b0010, 0b0001};               //digit number (first four bits in the bit shifter)
int reactionTime = 0;
int reactionTimeDisplayed[4];
int refreshRate = 100; //Hz
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
int createDropTime(int min, int max);
bool handsOn();
bool ballsPlaced();
bool fingersOn();

// --- ESP-NOW stuff ---
typedef struct struct_message // Structure to receive data
{
    bool ballSignal;
} struct_message;

// struct_message called myData
struct_message myData;

// variable affected by ball catching
bool caught = false;

// callback function that will be executed when data is received
void OnDataRecv(const uint8_t *mac_addr, const uint8_t *incomingData, int len)
{
    memcpy(&myData, incomingData, sizeof(myData));
    myData.ballSignal = true;
    caught = true;
}

// OLED display
Adafruit_SSD1351 tft = Adafruit_SSD1351(SCREEN_WIDTH, SCREEN_HEIGHT, CS_PIN, DC_PIN, MOSI_PIN, SCLK_PIN, RST_PIN);

void setup()
{
    Serial.begin(115200);

    // Set device as a Wi-Fi Station
    WiFi.mode(WIFI_STA);

    // Init ESP-NOW
    if (esp_now_init() != ESP_OK)
    {
        Serial.println("Error initializing ESP-NOW");
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
    pinMode(RIGHTHALLSENSOR, INPUT);
    pinMode(LEFTHALLSENSOR, INPUT);

    rightServo.attach(RIGHTSERVOPIN);
    leftServo.attach(LEFTSERVOPIN);

    tft.begin();

    rightServo.write(90);
    leftServo.write(90);

    lcdTestPattern(); // animation on boot - pride yeh
    delay(1000);

    tft.setCursor(0, 5);
    tft.fillScreen(BLACK);
    tft.setTextColor(GREEN);
    tft.setTextSize(1);
    tft.println("--- BAllCATCHERZ ---"); // live from the dripzone

    DifficultySelection(); // go in difficulty selection at least once at boot
}

void loop()
{
    if (catchMode == false)
    {
        playPrompt();

        while (1)
        {
            if (digitalRead(NEXTBUTTON) == LOW) // option to set difficulty with right button press
            {
                DifficultySelection();
                playPrompt();
                break;
            }
            else if (handsOn()) // press hands to play
            {
                break;
            }
        }

        timeGenerator(difficultyCounter); // select random time *after* difficulty selection
        catchMode = true;                 // go into play mode: don't set a new time, etc
        caught = false;                   // make sure the ball hasn't set the caught variable by mistake
        handsDelay(timeToRelease);        // checks user's hands position for given amt of time
        DropBall();                       // ball drops - also assigns millis() to dropTime
    }

    if (caught == true)
    {
        catchTime = (millis() - dropTime);
        // Serial.print("\tCatch time: \t");
        // Serial.println(catchTime);
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
        //TODO: Implement LED Strip signalizing that game is ready
        //TODO: Delete wehen nolonger needed
        //digitalWrite(REDLED, HIGH); // Red light signals both hands are placed
        return true;
    }
    else
    {
        //TODO: Implement LED Strip signalizing that game is NOT ready
        //TODO: Delete wehen nolonger needed
        // digitalWrite(REDLED, LOW);
        // digitalWrite(BLUELED, LOW);
        // digitalWrite(GREENLED, LOW);
        return false;
    }
}

// TODO: bool ballsPlaced() // returns true if balls are in place

void DifficultySelection()
{
    //Serial.println("--- Difficutly selection ---");
    tft.fillRect(0, 18, 128, 110, BLACK);
    tft.setCursor(0, 30);
    tft.setTextColor(BLUE);
    tft.setTextSize(1);
    tft.println("Choose difficulty");

    tft.setCursor(0, 50);
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

            tft.fillRect(0, 38, 128, 90, BLACK);
            tft.setCursor(0, 50);
            tft.setTextSize(2);

            displayDifficulty();
            // Serial.print("Difficulty: ");
            // Serial.println(difficultyCounter);

            delay(200); // avoid bouncing
        }
        if (handsOn()) // allow user to quit by placing hands
        {
            break;
        }
    }

    tft.fillRect(0, 18, 128, 90, BLACK);
    tft.setCursor(0, 30);
    tft.setTextSize(1);
    tft.print("Difficulty: ");

    displayDifficulty();
}

void displayDifficulty()
{
    switch (difficultyCounter) // display difficulty (during game)
    {
    case 0: // Easy Mode
        tft.println("EASY");
        // Serial.println("*** Easy mode selected ***");
        break;
    case 1: // Normal Mode
        tft.println("NORMAL");
        // Serial.println("*** Normal mode selected ***");
        break;
    case 2: // Hard Mode
        tft.println("HARD");
        // Serial.println("*** Hard mode selected ***");
        break;
    }
}

void playPrompt()
{
    tft.fillRect(0, 38, 128, 90, BLACK);
    tft.setCursor(0, 40);
    tft.setTextColor(BLUE);
    tft.setTextSize(1);
    tft.println("\nPlace hands to play");
    tft.println("\nPress NEXT to change difficulty");
}

void timeGenerator(int mode)
{
    switch (mode)
    {
    case 0: // Easy Mode
        timeToRelease = random(800, 1000);
        // Serial.print("Time to release - easy: \t");
        // Serial.println(timeToRelease);
        break;
    case 1: // Normal Mode
        timeToRelease = random(500, 1500);
        // Serial.print("Time to release - normal: \t");
        // Serial.println(timeToRelease);
        break;
    case 2: // Hard Mode
        timeToRelease = random(200, 2000);
        // Serial.print("Time to release - hard: \t");
        // Serial.println(timeToRelease);
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
                // Serial.print("ok - out of the loop after \t");
                // Serial.print(timeToDrop);
            }
        }
    }
}

void servosback()
{
    rightServo.write(90);
    leftServo.write(90);
}

void DropBall()
{
    bool dropRightBall = random(0, 2);
    if (dropRightBall)
    {
        //TODO: Implement rightServo release
        rightServo.write(60);

        //TODO: Delete wehen nolonger needed
        // digitalWrite(BLUELED, HIGH); // Right Ball drops
        // digitalWrite(GREENLED, LOW);
        Serial.println("\tRight Ball dropped yeh");
    }
    else
    {
        //TODO: Implement leftServo release
        leftServo.write(120);

        //TODO: Delete wehen nolonger needed
        // digitalWrite(GREENLED, HIGH); // Left Ball drops
        // digitalWrite(BLUELED, LOW);
        Serial.println("\tLeft Ball dropped yeh");
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
    tft.setCursor(0, 40);
    tft.print("\nScore: ");

    if (catchTime < bestTime || bestTime == 0)
    {
        bestTime = catchTime;
        tft.setTextColor(GREEN); // better score is green
    }
    else
        tft.setTextColor(RED); // worse score is red

    tft.setTextSize(2); // score bigger size
    tft.println(catchTime);
    //TODO: print to 7Seg display

    tft.setTextColor(WHITE);
    tft.setTextSize(1);
    tft.print("\n\nBest score: ");
    tft.println(bestTime); // best score printed in white
    tft.print("Place hands to continue...");

    reactionTimeDisplayed[0] = catchTime / 1000;
    reactionTimeDisplayed[1] = (catchTime / 100) % 10;
    reactionTimeDisplayed[2] = (catchTime / 10) % 10;
    reactionTimeDisplayed[3] = catchTime % 10; // push it to digits

    while (!handsOn())
    {
        for (int j = 1; j < 5; j++)
        {
            writeToDigit(j, reactionTimeDisplayed[j - 1]);
            delay(1000 / (refreshRate * 4));
        }
    }
    writeToDigit(5, 0);
}

void writeToDigit(int digitNumber, int number) //select which digit to write to and what to write in it
{
    digitNumber = digitNumber - 1;                                   // shift in new bits for number to be written
    digitalWrite(RCLK_PIN, LOW);                                     // ground latchPin and hold low for as long as data is transmitted
    shiftOut(SER_PIN, SRCLK_PIN, MSBFIRST, digitArray[digitNumber]); // shift in which digit to write to
    shiftOut(SER_PIN, SRCLK_PIN, MSBFIRST, datArray[number]);        // shift in the actual number
    digitalWrite(RCLK_PIN, HIGH);                                    // pull the latchPin clockPin to save the data
}