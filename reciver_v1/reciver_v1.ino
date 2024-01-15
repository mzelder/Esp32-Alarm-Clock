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

// [Twój dotychczasowy kod...]
// Wi-Fi credentials - change it for your WIFI
const char* ssid = "iPhone";
const char* password = "12345678";

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

typedef struct {
  bool activateModule;
} esp_now_message_t;

esp_now_message_t alarmMessage;
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

void setup() {
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
  
  // Wifi setup
  WiFi.mode(WIFI_STA);  
  connectWifi(); 
  
  // esp-now setup
  if (esp_now_init() != ESP_OK) {
    Serial.println("Błąd inicjalizacji ESP-NOW");
    return;
  }

  esp_now_register_send_cb(OnDataSent);
  esp_now_register_recv_cb(OnDataRecv);

  peerInfo.channel = 6;  
  peerInfo.encrypt = false;
  memcpy(peerInfo.peer_addr, remoteMac, 6);

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Błąd dodawania pary ESP-NOW");
    return;
  }
}


void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    connectWifi();
  }

  if (digitalRead(BTN_MIDDLE) == LOW) {
    alarm_mode = true; 
  }

  if (digitalRead(BTN_OFF) == LOW) {
    alarm_mode = false; 
  }
  
  if (!alarm_mode) {
    // displayDateTime();
    checkAndTriggerAlarm();
  } else {
    alarmSetter();
  }
}

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print(" send status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  
}

void checkAndTriggerAlarm() {
  // timeClient.update();
  // int currentHour = timeClient.getHours();
  // int currentMinute = timeClient.getMinutes();

  // Compare current time with alarm time
  // if (currentHour == alarmHour && currentMinute == alarmMinute) {

  alarmMessage.activateModule = true;
  esp_err_t result = esp_now_send(remoteMac, (uint8_t *)&alarmMessage, sizeof(alarmMessage));
  Serial.println(result == ESP_OK ? "Sent with success" : "Error sendint the data");
}

void connectWifi() {
  tft.fillScreen(ST77XX_BLACK);
  
  while (WiFi.status() != WL_CONNECTED) {  
    WiFi.begin(ssid, password);
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

  configTime(0, 3600, "pool.ntp.org", "time.nist.gov"); // Set timezone and NTP server
  timeClient.begin(); // Start NTP client
}

void displayDateTime() {
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return;
  }

  char dateBuffer[20];
  strftime(dateBuffer, sizeof(dateBuffer), "%Y/%m/%d", &timeinfo); // Format the date
  String currentDate = String(dateBuffer);

  String currentTime = timeClient.getFormattedTime();

  tft.fillScreen(ST77XX_BLACK);

  // Set cursor for time and print
  tft.setCursor(30, 60); // Adjust cursor position as needed
  tft.print(currentTime);

  // Set cursor for date and print
  tft.setCursor(25, 20); // Adjust cursor position as needed
  tft.print(currentDate);
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
