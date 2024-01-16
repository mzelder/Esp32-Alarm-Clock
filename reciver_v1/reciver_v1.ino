// BUG -> sender can't send data in the same moment when
// reciver try to get data from wifi for hour management

#include <Adafruit_GFX.h>
#include <Fonts/FreeMono9pt7b.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>
#include <WiFi.h>
#include <esp_now.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <time.h>
#include "SoftwareSerial.h"
#include "DFRobotDFPlayerMini.h"

// Wi-Fi credentials - change it for your WIFI
const char* ssid = "iPhone";
const char* password = "12345678";

// Use pins 2 and 3 to communicate with DFPlayer Mini
static const uint8_t PIN_MP3_TX = 32; // Connects to module's RX 
static const uint8_t PIN_MP3_RX = 33; // Connects to module's TX 
SoftwareSerial softwareSerial(PIN_MP3_RX, PIN_MP3_TX);

// Create the Player object
DFRobotDFPlayerMini player;


// NTP Client
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 3600, 60000); // Adjust for your timezone

// TFT display pin definitions
#define TFT_CS     5
#define TFT_RST    4
#define TFT_DC     2
#define TFT_MOSI  23
#define TFT_SCLK  18

// Buttons pin definitions
#define BTN_UP 27
#define BTN_MIDDLE 26
#define BTN_DOWN 25
#define BTN_OFF 14

// DFD Player pins
#define RX 16
#define TX 17

typedef struct struct_message {
  int b;
} struct_message;

struct_message myData;

struct tm timeinfo;

// Initialize the TFT display
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);

// Adres MAC drugiego modułu ESP32 - ZMIEŃ NA WŁAŚCIWY
uint8_t remoteMac[] = {0xD4, 0x8A, 0xFC, 0x9D, 0xCF, 0xA4};
esp_now_peer_info_t peerInfo;

bool alarm_mode = false;
int alarmHour = 0;
int alarmMinute = 0;
bool settingHour = false;

unsigned long previousMillis = 0; // Will store last time the display was updated
const long interval = 1000; // Interval at which to update the display (milliseconds)
bool timeFetched = false; // Flag to check if time has been fetched

void setup() {
  // initalize dfplayer mini
  softwareSerial.begin(9600);
  // Start communication with DFPlayer Mini
  if (player.begin(softwareSerial)) {
   Serial.println("OK");
   player.volume(30);
  } else {
    Serial.println("Connecting to DFPlayer Mini failed!");
  }
  
  myData.b = 0;
  // pin mode setup
  pinMode(BTN_UP, INPUT_PULLUP);
  pinMode(BTN_MIDDLE, INPUT_PULLUP);
  pinMode(BTN_DOWN, INPUT_PULLUP);
  pinMode(BTN_OFF, INPUT_PULLUP);
  
  Serial.begin(115200);
  
  // Initalize screen
  tft.initR(INITR_BLACKTAB);
  tft.setFont(&FreeMono9pt7b);
  tft.setRotation(1);
  tft.setTextSize(1);
  tft.setTextColor(ST77XX_WHITE);
  
  // Initialize and connect to WiFi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.println("Connecting to WiFi...");
    tft.fillScreen(ST77XX_BLACK);
    tft.setCursor(10, 50);
    tft.print("Connecting to WiFi...");
    delay(500);    
  }
  Serial.println("Connected to WiFi!");    
  tft.fillScreen(ST77XX_BLACK);
  tft.setCursor(10, 50);
  tft.print("Connected to WiFi!");

  // Fetch time and date
  configTime(0, 3600, "pool.ntp.org", "time.nist.gov"); // Set timezone and NTP server
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
  } else {
    timeFetched = true; // Set flag as true after fetching time
    displayDateTime(); // Display time immediately after fetching
  }

  timeinfo.tm_hour++;

  // Disconnect from WiFi
  WiFi.disconnect(true);
  WiFi.mode(WIFI_STA);
  Serial.println("Disconnected from WiFi"); 
  
  // esp-now setup
  if (esp_now_init() != ESP_OK) {
    Serial.println("Błąd inicjalizacji ESP-NOW");
    return;
  }

  esp_now_register_send_cb(OnDataSent);
  esp_now_register_recv_cb(OnDataRecv);

  peerInfo.channel = 0;  
  peerInfo.encrypt = false;
  memcpy(peerInfo.peer_addr, remoteMac, 6);

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Błąd dodawania pary ESP-NOW");
    return;
  }
}

void loop() {
  unsigned long currentMillis = millis();
  if (timeFetched && currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis; // Save the last time you updated the display
    timeinfo.tm_sec += 1; // Increment the second
    mktime(&timeinfo); // Normalize the time structure
    
    if (!alarm_mode) {
      if (alarmHour == timeinfo.tm_hour && alarmMinute == timeinfo.tm_min && myData.b == 0) {
        myData.b = 1;
      }
      if (myData.b == 1) {
        player.play(1);
        esp_err_t result = esp_now_send(remoteMac, (uint8_t *) &myData, sizeof(myData));
        if (result == ESP_OK) {
          Serial.println("Sent with success");
          myData.b = 2;
        } else {
          Serial.println("Error sendint the data");
        }
        
      }
      displayDateTime();
    } else {
      alarmSetter();
    }
  }
  
  if (digitalRead(BTN_MIDDLE) == LOW) {
    alarm_mode = true; 
  }

  if (digitalRead(BTN_OFF) == LOW) {
    alarm_mode = false; 
  }
}

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print(" send status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  player.stop();
  delay(60000);
  memcpy(&myData, incomingData, sizeof(myData));
  Serial.println(myData.b);
}

void displayDateTime() {
    char timeBuffer[9];
  sprintf(timeBuffer, "%02d:%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
  
  tft.fillScreen(ST77XX_BLACK);
  tft.setCursor(30, 60);
  tft.print(timeBuffer);

  char dateBuffer[11];
  sprintf(dateBuffer, "%04d/%02d/%02d", (timeinfo.tm_year + 1900), (timeinfo.tm_mon + 1), timeinfo.tm_mday);
  tft.setCursor(25, 20);
  tft.print(dateBuffer);
}

// Setting alarm clock 

// Function prototypes (if not already defined)
void addTimeToAlarm();
void toggleTimeSettingMode();
void displayAlarmTime();

void alarmSetter() {
  static unsigned long lastPressTime = 0;
  unsigned long pressInterval = 1; // Extremely short time between increments

  // Check if the UP button is pressed
  if (digitalRead(BTN_UP) == LOW) {
    if (millis() - lastPressTime > pressInterval) {
      addTimeToAlarm();
      lastPressTime = millis();
    }
  }

  // Check if the DOWN button is pressed
  if (digitalRead(BTN_DOWN) == LOW) {
    if (millis() - lastPressTime > pressInterval) {
      decreaseTimeFromAlarm();
      lastPressTime = millis();
    }
  }

  // Toggle between hour and minute setting with the middle button
  if (digitalRead(BTN_MIDDLE) == LOW) {
    toggleTimeSettingMode();
    delay(200); // Delay to prevent rapid toggling
  }

  

  // Display the current alarm time
  displayAlarmTime();
}

void addTimeToAlarm() {
  if (settingHour) {
    alarmHour = (alarmHour + 1) % 24; // Increment hour
  } else {
    alarmMinute++;
    if (alarmMinute >= 60) {
      alarmMinute = 0;
    }
  }
}

void decreaseTimeFromAlarm() {
  if (settingHour) {
    alarmHour = (alarmHour == 0) ? 23 : alarmHour - 1;
  } else {
    alarmMinute = (alarmMinute == 0) ? 59 : alarmMinute - 1;
  }
}

void toggleTimeSettingMode() {
  settingHour = !settingHour; // Toggle between hour and minute setting
}

// Global variables for the last displayed time
int lastDisplayedHour = -1;
int lastDisplayedMinute = -1;

void displayAlarmTime() {
  // Check if the time has changed since the last update
  if (alarmHour != lastDisplayedHour || alarmMinute != lastDisplayedMinute) {
    // Update the last displayed time
    lastDisplayedHour = alarmHour;
    lastDisplayedMinute = alarmMinute;

    // Format and display the new time
    char buffer[6];
    sprintf(buffer, "%02d:%02d", alarmHour, alarmMinute);

    tft.fillScreen(ST77XX_BLACK);
    tft.setCursor(40, 60); // Adjust the cursor position as needed
    tft.print(buffer);
    tft.setCursor(40, 30);
    tft.print("Set time: ");
  }
}