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

typedef struct test_struct {
  int x;
  int y;
} test_struct;

//Create a struct_message called myData
test_struct myData;

// Initialize the TFT display
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);

// Adres MAC drugiego modułu ESP32 - ZMIEŃ NA WŁAŚCIWY
uint8_t remoteMac[] = {0xD4, 0x8A, 0xFC, 0x9D, 0xCF, 0xA4};
esp_now_peer_info_t peerInfo;

void setup() {
  Serial.begin(115200);
  
  WiFi.mode(WIFI_STA);  
  WiFi.begin(ssid, password);
  
  connectWifi(); // Initialize WIFI and ESP-NOW
  
  // [Inicjalizacja TFT i WiFi...]
  tft.initR(INITR_BLACKTAB);
  tft.setFont(&FreeMono9pt7b);
  tft.setRotation(1);
  tft.setTextSize(1);
  tft.setTextColor(ST77XX_WHITE);


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
    connectWifi(); // Reconnect to Wi-Fi if connection is lost
  }

  timeClient.update();
  displayDateTime();

  delay(1000); // Update time every second
}

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("Dane wysłane do: ");
  Serial.println(macAddressToString(mac_addr));
}

void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  memcpy(&myData, incomingData, sizeof(myData));
  Serial.print("Bytes received: ");
  Serial.println(len);
  Serial.print("x: ");
  Serial.println(myData.x);
  Serial.print("y: ");
  Serial.println(myData.y);
  Serial.println();
}

String macAddressToString(const uint8_t* macAddr) {
  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
           macAddr[0], macAddr[1], macAddr[2], macAddr[3], macAddr[4], macAddr[5]);
  return String(macStr);
}


void connectWifi() {
  tft.fillScreen(ST77XX_BLACK);
  
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

  configTime(0, 3600, "pool.ntp.org", "time.nist.gov"); // Set timezone and NTP server
  timeClient.begin(); // Start NTP client
}

void displayDateTime() {
  struct tm timeinfo;
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