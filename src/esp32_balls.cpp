// ESP-Now settings
#include <esp_now.h>
#include <WiFi.h>

#define THRESHOLD 40
#define SLEEPTIME 300000 // delay in ms before uC goes to sleep

// Accelerometer pins:
#define XPIN 34
#define YPIN 35
#define ZPIN 32

touch_pad_t touchPin;

// RECEIVER'S MAC Address
uint8_t broadcastAddress[] = {0xa4, 0xcf, 0x12, 0x25, 0x7d, 0xe0};

// Structure to receive data
typedef struct struct_message
{
  bool ballSignal;
  bool restart;

} struct_message;

// Create a struct_message called myData
struct_message myData;

// Create peer interface
esp_now_peer_info_t peerInfo;

// callback when data is sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status)
{
  // Serial.print("\r\nLast Packet Send Status:\t");
  // Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

int x = 0, y = 0, z = 0;
float g, g_prev = 0;
bool okToSend = true;
int lastTime = 0;
uint32_t sleepTrigger = 0;

void callback()
{ // callback function: executed if touch on GPIO 15 wire
} // leave empty!

void setup()
{

  Serial.begin(115200);

  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);

  // Init ESP-NOW
  if (esp_now_init() != ESP_OK)
  {
    // Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Register for Send CB to get status of Trasnmitted packet
  esp_now_register_send_cb(OnDataSent);

  // Register peer
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  // Add peer
  if (esp_now_add_peer(&peerInfo) != ESP_OK)
  {
    // Serial.println("Failed to add peer");
    return;
  }

  // Interrupt on GPIO15 for sleep wake-up
  touchAttachInterrupt(T3, callback, THRESHOLD);

  //Configure Touchpad as wakeup source
  esp_sleep_enable_touchpad_wakeup();

  myData.ballSignal = true;
  myData.restart = true;
  esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *)&myData, sizeof(myData));
  myData.restart = false;
}

void loop()
{

  x = 0;
  y = 0;
  z = 0;

  for (int i = 0; i < 10; i++)
  {
    x += analogRead(XPIN);
    y += analogRead(YPIN);
    z += analogRead(ZPIN);
    delayMicroseconds(1);
  }
  x /= 10;
  y /= 10;
  z /= 10;

  g_prev = g;

  g = sqrt(x * x + y * y + z * z);

  float difference = abs(g - g_prev);

  if (difference > 250 && millis() - lastTime > 3000) // 250 low enough for both balls
  {
    sleepTrigger = millis();
    // Send message via ESP-NOW
    esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *)&myData, sizeof(myData));

    if (result == ESP_OK)
    {
      Serial.println("Sent with success");
    }
    else
    {
      Serial.println("Error sending the data");
    }

    lastTime = millis();
  }
  if (millis() - sleepTrigger > SLEEPTIME) // 300000 ms: if no transmission for 5min
  {
    myData.ballSignal = false; // lets the station know the ball sleeps
    esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *)&myData, sizeof(myData));
    Serial.println("Sleeping...");
    esp_deep_sleep_start(); // go to sleep until pin 15 is touched
  }
}