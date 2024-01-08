#include <Adafruit_GFX.h>
#include <Fonts/FreeSans12pt7b.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>
#include <WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

// Wi-Fi credentials - change it for your WIFI
const char* ssid = "iPhone";
const char* password = "12345678";

// NTP Client
WiFiUDP ntpUDP;
// Adjust the '3600 * yourTimezoneOffset' to your timezone in seconds
NTPClient timeClient(ntpUDP, "pool.ntp.org", 3600, 60000);

// TFT display pin definitions
#define TFT_CS     5
#define TFT_RST    4
#define TFT_DC     2
#define TFT_MOSI  23
#define TFT_SCLK  18

// Initialize the TFT display
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);

void setup() {
  // Initialize serial communication (optional, for debugging)
  Serial.begin(115200); 

  tft.initR(INITR_BLACKTAB);
  tft.setFont(&FreeSans12pt7b);
  tft.setRotation(1);
  tft.setTextSize(1);
   tft.setTextColor(ST77XX_WHITE);

  // Initalize WIFI
  connectWifi();
}

void loop() {
  // Check Wi-Fi connection
  if (WiFi.status() != WL_CONNECTED) {
    connectWifi(); // Reconnect to Wi-Fi if connection is lost
  }

  timeClient.update();
  String currentTime = timeClient.getFormattedTime();

  // Clear the previous time
  tft.fillScreen(ST77XX_BLACK);
  
  // Set cursor to top-left corner and print current time
  tft.setCursor(30, 50);
  tft.print(currentTime);

  delay(1000); // Update time every second
}


void connectWifi() {
  // Initialize Wi-Fi
  WiFi.begin(ssid, password);
  
  tft.fillScreen(ST77XX_BLACK);
  
  while (WiFi.status() != WL_CONNECTED) {  
    // Connecting 
    Serial.println("Connecting to WiFi...");
    
    tft.fillScreen(ST77XX_BLACK);
    tft.setCursor(30, 50);
    tft.print("Connecting to WiFi...");

    delay(500);    
  }
  
  // Connected
  Serial.println("Connected to WiFi!");
    
  tft.fillScreen(ST77XX_BLACK);
  tft.setCursor(30, 50);
  tft.print("Connected to WiFi!");

  // Start NTP client
  timeClient.begin();
}
