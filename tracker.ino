#include <WiFi.h>

// --- CONFIGURATION ---
// Define the MAC addresses or SSIDs of the 3 known Access Points (Hotspots)
// You can use either MAC or SSID. Using MAC is more reliable if SSID is not unique.

// Option 1: Use MAC addresses (recommended)
uint8_t AP1_MAC[] = {0x1A, 0x2B, 0x3C, 0x4D, 0x5E, 0x6F}; // Replace with actual MAC of Hotspot 1
uint8_t AP2_MAC[] = {0x2A, 0x3B, 0x4C, 0x5D, 0x6E, 0x7G}; // Hotspot 2
uint8_t AP3_MAC[] = {0x3A, 0x4B, 0x5C, 0x6D, 0x7E, 0x8H}; // Hotspot 3

// Option 2: Use SSIDs (less reliable if multiple networks have same name)
// const char* AP1_SSID = "Hotspot-Alpha";
// const char* AP2_SSID = "Hotspot-Beta";
// const char* AP3_SSID = "Hotspot-Gamma";

// Calibration values (MUST be measured experimentally!)
#define REFERENCE_RSSI -40    // RSSI at 1 meter (dBm). Measure this!
#define PATH_LOSS_EXPONENT 3.0  // Typical indoor value: 2.5–4.0

// Scan settings
#define SCAN_DELAY 2000  // Delay between scans (ms)

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

  Serial.println("ESP32 Wi-Fi RSSI Distance Estimator");
  Serial.println("Scanning for 3 Access Points...");
}

void loop() {
  int16_t rssi1 = 100, rssi2 = 100, rssi3 = 100; // Initialize with invalid high value
  bool found1 = false, found2 = false, found3 = false;

  // Start Wi-Fi scan
  int n = WiFi.scanNetworks();
  Serial.printf("Scanned %d networks\n", n);

  for (int i = 0; i < n; i++) {
    // Get MAC address of scanned network
    uint8_t* mac = WiFi.BSSID(i);

    // Compare MAC addresses (use this for accuracy)
    if (!found1 && compareMAC(mac, AP1_MAC)) {
      rssi1 = WiFi.RSSI(i);
      found1 = true;
    }
    if (!found2 && compareMAC(mac, AP2_MAC)) {
      rssi2 = WiFi.RSSI(i);
      found2 = true;
    }
    if (!found3 && compareMAC(mac, AP3_MAC)) {
      rssi3 = WiFi.RSSI(i);
      found3 = true;
    }

    // Optional: Use SSID matching instead
    /*
    if (!found1 && strcmp(WiFi.SSID(i).c_str(), AP1_SSID) == 0) {
      rssi1 = WiFi.RSSI(i);
      found1 = true;
    }
    */
  }

  // Estimate distance for each AP
  float d1 = estimateDistance(rssi1);
  float d2 = estimateDistance(rssi2);
  float d3 = estimateDistance(rssi3);

  // Print results
  Serial.println("\n--- RSSI & Distance ---");
  Serial.printf("AP1 - RSSI: %d dBm | Distance: %.2f m\n", rssi1, d1);
  Serial.printf("AP2 - RSSI: %d dBm | Distance: %.2f m\n", rssi2, d2);
  Serial.printf("AP3 - RSSI: %d dBm | Distance: %.2f m\n", rssi3, d3);

  // You can now send this data via UDP/TCP/HTTP to your computer!
  // Example: Serial.println("DATA:" + String(d1) + "," + String(d2) + "," + String(d3));

  // Wait before next scan
  WiFi.scanDelete(); // Free memory
  delay(SCAN_DELAY);
}

// Function to estimate distance from RSSI
float estimateDistance(int rssi) {
  if (rssi == 100 || rssi >= 0) return 0.0; // Invalid or no signal
  float ratio = (REFERENCE_RSSI - rssi) / (10 * PATH_LOSS_EXPONENT);
  return pow(10, ratio);
}

// Helper: Compare two MAC addresses
bool compareMAC(uint8_t* a, uint8_t* b) {
  for (int i = 0; i < 6; i++) {
    if (a[i] != b[i]) return false;
  }
  return true;
}git 