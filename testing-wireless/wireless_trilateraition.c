#include "WiFi.h"
#include <WiFiUdp.h>
#include <cmath>

// --- Configuration ---
const char* ssid = "YourWiFiSSID";       // Replace with your Wi-Fi name
const char* password = "YourWiFiPassword"; // Replace with your Wi-Fi password

const char* COMPUTER_IP = "192.168.1.105";  // Replace with your computer’s IP
const int UDP_PORT = 4210;

const char* AP_SSIDS[] = {"narzo", "Y_GIRIBABU3 4620", "A"};
const int NUM_APS = 3;

const double TX_POWER = -45.0;
const double PATH_LOSS_EXPONENT = 2.5;

// --- Kalman Filter ---
double kalman_estimate[NUM_APS] = {0.0};
double kalman_error_estimate[NUM_APS] = {1.0};
const double kalman_process_noise = 0.125;
const double kalman_measurement_noise = 4.0;

// --- UDP object ---
WiFiUDP udp;

// --- Function declarations ---
double kalman_update(int ap_index, double measurement);
double calculate_distance(double rssi);
void send_data_udp(String data);

void setup() {
  Serial.begin(115200);
  Serial.println("ESP32 WiFi Trilateration (UDP Version)");

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected!");
  Serial.print("ESP32 IP: ");
  Serial.println(WiFi.localIP());

  udp.begin(UDP_PORT);

  for (int i = 0; i < NUM_APS; i++) {
    kalman_estimate[i] = -60.0;
    kalman_error_estimate[i] = 1.0;
  }
}

void loop() {
  Serial.println("\nScanning...");
  int n = WiFi.scanNetworks();
  bool ap_found[NUM_APS] = {false};

  if (n > 0) {
    for (int i = 0; i < n; ++i) {
      String ssid = WiFi.SSID(i);
      int rssi = WiFi.RSSI(i);

      for (int j = 0; j < NUM_APS; j++) {
        if (ssid.equals(AP_SSIDS[j])) {
          kalman_estimate[j] = kalman_update(j, rssi);
          ap_found[j] = true;
          break;
        }
      }
    }
  }

  for (int i = 0; i < NUM_APS; i++) {
    if (!ap_found[i]) {
      Serial.printf("AP '%s' not found.\n", AP_SSIDS[i]);
    }
  }

  // Prepare data string
  String data = "DATA:";
  for (int i = 0; i < NUM_APS; i++) {
    double dist = calculate_distance(kalman_estimate[i]);
    if (dist < 0) dist = 0.0;
    data += String(dist, 3);
    if (i < NUM_APS - 1) data += ",";
  }

  // Send to both serial and UDP
  Serial.println(data);
  send_data_udp(data);

  WiFi.scanDelete();
  delay(500);
}

double kalman_update(int ap_index, double measurement) {
  kalman_error_estimate[ap_index] += kalman_process_noise;
  double gain = kalman_error_estimate[ap_index] /
                (kalman_error_estimate[ap_index] + kalman_measurement_noise);
  kalman_estimate[ap_index] += gain * (measurement - kalman_estimate[ap_index]);
  kalman_error_estimate[ap_index] *= (1.0 - gain);
  return kalman_estimate[ap_index];
}

double calculate_distance(double rssi) {
  if (rssi == 0.0) return -1.0;
  double exponent = (TX_POWER - rssi) / (10.0 * PATH_LOSS_EXPONENT);
  return pow(10.0, exponent);
}

void send_data_udp(String data) {
  udp.beginPacket(COMPUTER_IP, UDP_PORT);
  udp.print(data);
  udp.endPacket();
}
