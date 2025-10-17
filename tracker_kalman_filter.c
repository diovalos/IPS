//using kalman filter
#include "WiFi.h"
#include <cmath>

// --- Configuration ---
const char* AP_SSIDS[] = {"narzo", "Y_GIRIBABU3 4620", "A"};
const int NUM_APS = 3;

const double TX_POWER = -45.0;           // Calibrate at 1 meter
const double PATH_LOSS_EXPONENT = 2.5;   // Environment factor (2–4 typical)

// --- Kalman Filter Parameters ---
double kalman_estimate[NUM_APS] = {0.0};
double kalman_error_estimate[NUM_APS] = {1.0};
const double kalman_process_noise = 0.125;     // Small process noise (stability)
const double kalman_measurement_noise = 4.0;   // Measurement noise (RSSI variance)

// --- Function Declarations ---
double kalman_update(int ap_index, double measurement);
double calculate_distance(double rssi);
void print_status();
void send_data_to_serial();

void setup() {
    Serial.begin(115200);
    Serial.println("ESP32 WiFi Distance Calculator - Kalman Filter Version");

    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(100);

    // Initialize filter estimates
    for (int i = 0; i < NUM_APS; i++) {
        kalman_estimate[i] = -60.0;  // start with typical RSSI value
        kalman_error_estimate[i] = 1.0;
    }

    Serial.println("Setup complete. Starting scan loop...");
}

void loop() {
    Serial.println("\nStarting WiFi scan...");
    int n = WiFi.scanNetworks();
    Serial.print(n); Serial.println(" networks found.");

    bool ap_found[NUM_APS] = {false};

    if (n > 0) {
        for (int i = 0; i < n; ++i) {
            String ssid = WiFi.SSID(i);
            int rssi = WiFi.RSSI(i);

            for (int j = 0; j < NUM_APS; j++) {
                if (ssid.equals(AP_SSIDS[j])) {
                    // Update Kalman filter with new measurement
                    kalman_estimate[j] = kalman_update(j, rssi);
                    ap_found[j] = true;
                    break;
                }
            }
        }
    } else {
        Serial.println("No networks found.");
    }

    // Handle missing APs
    for (int i = 0; i < NUM_APS; i++) {
        if (!ap_found[i]) {
            Serial.print("AP '"); Serial.print(AP_SSIDS[i]);
            Serial.println("' not found. Retaining previous estimate.");
        }
    }

    print_status();
    send_data_to_serial();

    WiFi.scanDelete();
    delay(500); // Slight delay between scans (adjust as needed)
}

// --- Kalman Filter Update Function ---
double kalman_update(int ap_index, double measurement) {
    // Prediction step
    kalman_error_estimate[ap_index] += kalman_process_noise;

    // Kalman Gain
    double kalman_gain = kalman_error_estimate[ap_index] /
                         (kalman_error_estimate[ap_index] + kalman_measurement_noise);

    // Correction step
    kalman_estimate[ap_index] += kalman_gain * (measurement - kalman_estimate[ap_index]);
    kalman_error_estimate[ap_index] *= (1.0 - kalman_gain);

    return kalman_estimate[ap_index];
}

// --- Distance Calculation ---
double calculate_distance(double rssi) {
    if (rssi == 0.0) return -1.0;
    double exponent = (TX_POWER - rssi) / (10.0 * PATH_LOSS_EXPONENT);
    return pow(10.0, exponent);
}

// --- Display Current Status ---
void print_status() {
    Serial.println("\n--- Current Status (Kalman Filter) ---");
    for (int i = 0; i < NUM_APS; i++) {
        double dist = calculate_distance(kalman_estimate[i]);
        Serial.print("AP: "); Serial.print(AP_SSIDS[i]);
        Serial.print("\tFiltered RSSI: "); Serial.print(kalman_estimate[i], 2);
        Serial.print(" dBm\tDistance: ");
        if (dist < 0) Serial.println("N/A");
        else Serial.println(dist, 3);
    }
    Serial.println("--------------------------------------");
}

// --- Send Data for External Processing ---
void send_data_to_serial() {
    Serial.print("DATA:");
    for (int i = 0; i < NUM_APS; i++) {
        double dist = calculate_distance(kalman_estimate[i]);
        if (dist < 0) Serial.print("0.000");
        else Serial.print(dist, 3);
        if (i < NUM_APS - 1) Serial.print(",");
    }
    Serial.println();
}
