#include <Arduino.h>

#include <Adafruit_I2CDevice.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1351.h>
#include <SPI.h>

// --- ESP-NOW stuff ---
#include <esp_now.h>
#include <WiFi.h>

// Structure to receive data
typedef struct struct_message
{
    bool ballSignal;
    //float magnitude; //******
} struct_message;

// Create a struct_message called myData
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

//This version of the code uses the screen and 4 buttons.
//2 for the Next/Select functionality
//2 to simulate the Hand pads

// Screen dimensions
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 128 // Change this to 96 for 1.27" OLED.

// Color definitions
#define BLACK 0x0000
#define BLUE 0x001F
#define RED 0xF800
#define GREEN 0x07E0
#define CYAN 0x07FF
#define MAGENTA 0xF81F
#define YELLOW 0xFFE0
#define WHITE 0xFFFF

void lcdTestPattern(void);

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
uint16_t timeToRelease = 0;
uint32_t processorTime = 0;
uint32_t previousTime = 0;
uint32_t elapsedTime = 0;

uint32_t dropTime = 0;
uint16_t catchTime = 0;
uint16_t bestTime = 0;

// catch variable for random time generation
bool catchMode = false;
bool dropped = false;

//Button State
int difficultyCounter = 0; // variable for difficulty selection
int StateNext = 0;
int LastStateNext = 0;
int CounterNext = 0; // used for difficulty settings
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
void DifficultySelection();
void DropBall();
int createDropTime(int min, int max);
void timeGenerator(int mode);
bool handsOn();
bool fingersOn();
void handsDelay(int timeToDrop);

void setup()
{

    Serial.begin(115200);

    // ESP-NOW stuff
    //Set device as a Wi-Fi Station
    WiFi.mode(WIFI_STA);

    //Init ESP-NOW
    if (esp_now_init() != ESP_OK)
    {
        Serial.println("Error initializing ESP-NOW");
        return;
    }

    // Register for recv CB to get recv packer info
    esp_now_register_recv_cb(OnDataRecv);

    pinMode(SelectButton, INPUT_PULLUP);
    pinMode(NextButton, INPUT_PULLUP);
    pinMode(LeftHand, INPUT_PULLUP);
    pinMode(RightHand, INPUT_PULLUP);
    pinMode(redLED, OUTPUT);
    pinMode(blueLED, OUTPUT);
    pinMode(greenLED, OUTPUT);

    tft.begin();

    lcdTestPattern();
    delay(1000);

    tft.setCursor(0, 5);
    tft.fillScreen(BLACK);
    tft.setTextColor(GREEN);
    tft.setTextSize(1);
    tft.println("--- BAllCATCHERZ ---");

    DifficultySelection();
}

void loop()
{
    if (catchMode == false)
    {
        tft.fillRect(0, 38, 128, 90, BLACK);
        tft.setCursor(0, 40);
        tft.setTextColor(BLUE);
        tft.setTextSize(1);
        tft.println("\nPress hands to play");
        tft.println("\nPress next to change difficulty");
        while (1)
        {
            if (digitalRead(NextButton) == LOW)
            {
                DifficultySelection();
                break;
            }
            else if (handsOn())
            {
                break;
            }
        }
        timeGenerator(CounterNext);
        catchMode = true;
        caught = false;
        Serial.println("\n\t--- Waiting for hands... ---");
        while (!handsOn())
        {
            if (digitalRead(NextButton) == LOW)
            {
                CounterSelect = 0;
                break;
            }
        }

        handsDelay(timeToRelease); // checks user's hands position for given amt of time
        DropBall();                // also assigns millis() to dropTime
        dropped = true;
    }

    if (caught == true)
    {
        catchTime = (millis() - dropTime);
        Serial.print("\tCatch time: \t");
        Serial.println(catchTime);
        tft.fillRect(0, 40, 128, 90, BLACK);
        tft.setCursor(0, 40);
        tft.print("\nScore: ");
        if (catchTime < bestTime || bestTime == 0)
        {
            bestTime = catchTime;
            tft.setTextColor(GREEN);
        }
        else
            tft.setTextColor(RED);

        tft.setTextSize(2);
        tft.println(catchTime);

        tft.setTextColor(WHITE);
        tft.setTextSize(1);
        tft.print("\n\nBest score: ");
        tft.println(bestTime);

        while (!handsOn())
        {
        }
        catchMode = false;
        myData.ballSignal = false;
        caught = false;
        dropped = false;
        int caughtDelay = millis();
        while (millis() - caughtDelay < 1000)
        {
        }
    }
}

bool handsOn()
{
    if (!digitalRead(LeftHand) && !digitalRead(RightHand))
    {
        digitalWrite(redLED, HIGH); //Red light signals both hands are placed
        return true;
    }
    else
    {
        digitalWrite(redLED, LOW);
        digitalWrite(blueLED, LOW);
        digitalWrite(greenLED, LOW);
        return false;
    }
}

bool fingersOn()
{
    if (!digitalRead(NextButton) && !digitalRead(SelectButton))
    {
        return true;
    }
    else
    {
        return false;
    }
}

void DifficultySelection()
{

    Serial.println("--- Difficutly selection ---");
    tft.fillRect(0, 18, 128, 110, BLACK);
    tft.setCursor(0, 30);
    tft.setTextColor(BLUE);
    tft.setTextSize(1);
    tft.println("Choose difficulty");

    while (digitalRead(SelectButton))
    {
        if (digitalRead(NextButton) == LOW)
        {
            tft.fillRect(0, 38, 128, 90, BLACK);
            tft.setCursor(0, 50);
            tft.setTextSize(2);
            difficultyCounter++;
            if (difficultyCounter == 3)
            {
                difficultyCounter = 0;
            }
            switch (difficultyCounter)
            {
            case 0: // Easy Mode
                tft.println("Easy");
                break;
            case 1: // Normal Mode
                tft.println("Normal");
                break;
            case 2: // Hard Mode
                tft.println("Hard");
                break;
            }
            Serial.print("Difficulty: ");
            Serial.println(difficultyCounter);
            delay(200);
        }
    }
    tft.fillRect(0, 18, 128, 90, BLACK);
    tft.setCursor(0, 30);
    tft.setTextSize(1);
    tft.print("Difficulty: ");
    switch (difficultyCounter)
    {
    case 0: // Easy Mode
        tft.println("EASY");
        Serial.println("*** Easy mode selected ***");
        break;
    case 1: // Normal Mode
        tft.println("NORMAL");
        Serial.println("*** Normal mode selected ***");
        break;
    case 2: // Hard Mode
        tft.println("HARD");
        Serial.println("*** Hard mode selected ***");
        break;
    }
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

void DropBall()
{
    bool dropRightBall = random(0, 2);
    if (dropRightBall)
    {
        digitalWrite(blueLED, HIGH); // Right Ball drops
        digitalWrite(greenLED, LOW);
        Serial.println("\tRight Ball dropped yeh");
    }
    else
    {
        digitalWrite(greenLED, HIGH); // Left Ball drops
        digitalWrite(blueLED, LOW);
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