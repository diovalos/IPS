#include <WiFi.h>

#include <stdbool.h>

// --- CONFIGURATION ---

// Define the MAC addresses or SSIDs of the 3 known Access Points (Hotspots)

// You can use either MAC or SSID. Using MAC is more reliable if SSID is not unique.



// Option 1: Use MAC addresses (recommended)

//uint8_t AP1_MAC[] = {0xA6, 0x8B, 0x48, 0xBC, 0x06, 0xBA}; // Replace with actual MAC of Hotspot 1

//uint8_t AP2_MAC[] = {0x92, 0x0f, 0x69, 0xbf, 0x58, 0xf7}; // Hotspot 2

//uint8_t AP3_MAC[] = {0x3A, 0x4B, 0x5C, 0x6D, 0x7E, 0x8F}; // Hotspot 3



// Option 2: Use SSIDs (less reliable if multiple networks have same name)

const char* AP1_SSID = "hello";

const char* AP2_SSID = "narzo";

const char* AP3_SSID = "A";



// Calibration values (MUST be measured experimentally!)

#define REFERENCE_RSSI -40    // RSSI at 1 meter (dBm). Measure this!

#define PATH_LOSS_EXPONENT 3.0  // Typical indoor value: 2.5–4.0



// Scan settings

#define SCAN_DELAY 50  // Delay between scans (ms)



void setup() {

  Serial.begin(115200);

  WiFi.mode(WIFI_STA);

  WiFi.disconnect();

  delay(50);



  Serial.println("ESP32 Wi-Fi RSSI Distance Estimator");

  Serial.println("Scanning for 3 Access Points...");

}



void loop() {

  int16_t rssi1 = 100, rssi2 = 100, rssi3 = 100; // Initialize with invalid high value

  bool found1 = false, found2 = false, found3 = false;



  // Start Wi-Fi scan

  int n = WiFi.scanNetworks();

  Serial.printf("Scanned %d networks\n", n);



//  for (int i = 0; i < n; i++) {

//    // Get MAC address of scanned network

//    uint8_t* mac = WiFi.BSSID(i);

//

//    // Compare MAC addresses (use this for accuracy)

//    if (!found1 && compareMAC(mac, AP1_MAC)) {

//      rssi1 = WiFi.RSSI(i);

//      found1 = true;

//    }

//    if (!found2 && compareMAC(mac, AP2_MAC)) {

//      rssi2 = WiFi.RSSI(i);

//      found2 = true;

//    }

//    if (!found3 && compareMAC(mac, AP3_MAC)) {

//      rssi3 = WiFi.RSSI(i);

//      found3 = true;

//    }





 // In loop():



//for (int i = 0; i < n; i++) {

//  String ssid = WiFi.SSID(i);

//  if (!found1 && ssid == String(AP1_SSID)) {

//    rssi1 = WiFi.RSSI(i);

//    found1 = true;

//  }

//  if (!found2 && ssid == String(AP2_SSID)) {

//    rssi2 = WiFi.RSSI(i);

//    found2 = true;

//  }

//  if (!found3 && ssid == String(AP3_SSID)) {

//    rssi3 = WiFi.RSSI(i);

//    found3 = true;

//  }

//}





  for (int i = 0; i < n; i++) {

    // Get MAC address of scanned network

//    uint8_t* mac = WiFi.BSSID(i);



    // Compare MAC addresses (use this for accuracy)

    if (!found1 && strcmp(WiFi.SSID(i).c_str(), AP1_SSID) == 0) {

      rssi1 = WiFi.RSSI(i);

      found1 = true;

    }

     if (!found2 && strcmp(WiFi.SSID(i).c_str(), AP2_SSID) == 0) {

      rssi2 = WiFi.RSSI(i);

      found2 = true;

    } if (!found3 && strcmp(WiFi.SSID(i).c_str(), AP3_SSID) == 0) {

      rssi3 = WiFi.RSSI(i);

      found3 = true;

    }

  }

    Serial.printf("%d %d %d device found \n",(int)found1,(int)found2,(int)found3);

    // Optional: Use SSID matching instead

    /*

    if (!found1 && strcmp(WiFi.SSID(i).c_str(), AP1_SSID) == 0) {

      rssi1 = WiFi.RSSI(i);

      found1 = true;

    }

    */

//}



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

}
