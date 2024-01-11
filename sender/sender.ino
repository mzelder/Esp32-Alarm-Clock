#include <WiFi.h>
#include <esp_now.h>

// Piny - ZMIEŃ JEŻELI POTRZEBNE
#define BUTTON_PIN 4
#define LED_PIN 2

// Adres MAC pierwszego modułu ESP32 - ZMIEŃ NA WŁAŚCIWY
uint8_t remoteMac[] = {0xD4, 0x8A, 0xFC, 0x9F, 0x2F, 0x88};
esp_now_peer_info_t peerInfo;

const char* ssid = "iPhone";
const char* password = "12345678";

typedef struct test_struct {
  int x;
} test_struct;

test_struct test;

void setup() {
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);

  Serial.begin(115200);

  // Inicjalizacja WiFi w trybie STA
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  // Inicjalizacja ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Błąd inicjalizacji ESP-NOW");
    return;
  }

  esp_now_register_send_cb(OnDataSent);
  esp_now_register_recv_cb(OnDataRecv);

  peerInfo.channel = 6;  
  peerInfo.encrypt = false;
  memcpy(peerInfo.peer_addr, remoteMac, 6);

  // register peer 
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Błąd dodawania pary ESP-NOW");
    return;
  }
}

void loop() {
  test.x = random(0,20);

  esp_err_t result = esp_now_send(0, (uint8_t *) &test, sizeof(test_struct));
  Serial.println(result == ESP_OK ? "Sent with success" : "Error sendint the data");
    
  delay(500);
}

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  char macStr[18];
  Serial.print("Packet to: ");
  // Copies the sender mac address to a string
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  Serial.print(macStr);
  Serial.print(" send status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  Serial.println("Dane otrzymane via ESP-NOW");
  // Tutaj możesz dodać logikę do przetwarzania otrzymanych danych
}

String macAddressToString(const uint8_t* macAddr) {
  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
           macAddr[0], macAddr[1], macAddr[2], macAddr[3], macAddr[4], macAddr[5]);
  return String(macStr);
}