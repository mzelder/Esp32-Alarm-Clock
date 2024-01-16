#include <WiFi.h>
#include <esp_now.h>

// Piny - ZMIEŃ JEŻELI POTRZEBNE
#define BUTTON_PIN 14
#define LED_PIN 2

// Adres MAC pierwszego modułu ESP32 - ZMIEŃ NA WŁAŚCIWY
uint8_t remoteMac[] = {0xD4, 0x8A, 0xFC, 0x9F, 0x2F, 0x88};
esp_now_peer_info_t peerInfo;

const char* ssid = "iPhone";
const char* password = "12345678";

typedef struct struct_message {
  int b;
} struct_message;

struct_message myData;

void setup() {
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);

  Serial.begin(115200);

  // Inicjalizacja WiFi w trybie STA
  WiFi.mode(WIFI_STA);
  // WiFi.begin(ssid, password);

  // Inicjalizacja ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Błąd inicjalizacji ESP-NOW");
    return;
  }

  esp_now_register_send_cb(OnDataSent);
  esp_now_register_recv_cb(OnDataRecv);

  peerInfo.channel = 0;  
  peerInfo.encrypt = false;
  memcpy(peerInfo.peer_addr, remoteMac, 6);

  // register peer 
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Błąd dodawania pary ESP-NOW");
    return;
  }
}

void loop() {
    if (myData.b == 1) {
      Serial.println("Dioda!");
      digitalWrite(LED_PIN, HIGH);
      delay(500);
      digitalWrite(LED_PIN, LOW);
      delay(500);
      
      if (digitalRead(BUTTON_PIN) == LOW) {
        myData.b = 0;
        esp_err_t result = esp_now_send(remoteMac, (uint8_t *) &myData, sizeof(myData));    
        if (result == ESP_OK) {
          Serial.println("Sent with success");
        } else {
          Serial.println("Error sendint the data");
        }
      }
    }
}

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print(" send status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  Serial.println("ALARM!");
  memcpy(&myData, incomingData, sizeof(myData));
  Serial.println(myData.b);
}