/* #include <Arduino.h>

// ESP-Now settings
#include <esp_now.h>
#include <WiFi.h>

// RECEIVER'S MAC Address
uint8_t broadcastAddress[] = {0xa4, 0xcf, 0x12, 0x25, 0x7d, 0xe0};

// Must match the receiver structure
typedef struct struct_message
{
    int id;          // must be unique for each sender board
    float magnitude; //******
} struct_message;

// Create a struct_message called myData
struct_message myData;

// Create peer interface
esp_now_peer_info_t peerInfo;

// callback fn when data is sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status)
{
    // Serial.print("\r\nLast Packet Send Status:\t");
    // Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

// Accelerometer pins:
#define xPin 34
#define yPin 35
#define zPin 32

int x = 0, y = 0, z = 0;

float gx = 0, gy = 0, gz = 0;
float gx_prev = 0, gy_prev = 0, gz_prev = 0;

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

    // Register for Send CB to get status of Trasnmitted packet
    esp_now_register_send_cb(OnDataSent);

    // Register peer
    memcpy(peerInfo.peer_addr, broadcastAddress, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;

    // Add peer
    if (esp_now_add_peer(&peerInfo) != ESP_OK)
    {
        Serial.println("Failed to add peer");
        return;
    }
}

void loop()
{
    x = 0;
    y = 0;
    z = 0;

    for (int i = 0; i < 10; i++)
    {
        x += analogRead(xPin);
        y += analogRead(yPin);
        z += analogRead(zPin);
        delayMicroseconds(1);
    }
    x /= 10;
    y /= 10;
    z /= 10;

    gx_prev = gx;
    gy_prev = gy;
    gz_prev = gz;

    gx = (x - 2048) / 330.0;
    gy = (x - 2048) / 330.0;
    gz = (x - 2048) / 330.0;

    float g = sqrt(gx * gx + gy * gy + gz * gz);
    float g_prev = sqrt(gx_prev * gx_prev + gy_prev * gy_prev + gz_prev * gz_prev);
    float difference = abs(g - g_prev);

    // testing:
    // Serial.print(x);
    // Serial.print("\t");
    // Serial.print(y);
    // Serial.print("\t");
    // Serial.println(z);

    // Serial.println(g);
    // Serial.println(g_prev);

    //Serial.println(difference);

    // Set values to send
    myData.id = 0;
    myData.magnitude = difference;

    if (difference > 2)
    {
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
        // delay(1000);
    }
} */