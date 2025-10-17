//moving average
#include "WiFi.h"
#include <cmath>

// --- Configuration ---
const char* AP_SSIDS[] = {"narzo", "Y_GIRIBABU3 4620", "A"}; // updated order
const int NUM_APS = 3;

const double TX_POWER = -45.0; // Calibrate this at exactly 1 meter!
const double PATH_LOSS_EXPONENT = 2.5;

// --- Noise Filter (Moving Average) Configuration ---
const int FILTER_SAMPLES = 10;
int rssi_readings[NUM_APS][FILTER_SAMPLES];
int reading_index[NUM_APS] = {0};
long rssi_totals[NUM_APS] = {0};
int sample_counts[NUM_APS] = {0};
double filtered_rssi[NUM_APS] = {0.0};

void update_filter(int ap_index, int new_rssi);
double calculate_distance(double rssi);
void print_status();
void send_data_to_serial();

void setup() {
    Serial.begin(115200);
    Serial.println("ESP32 WiFi Distance Calculator - Updated");

    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(100);

    for (int i = 0; i < NUM_APS; i++) {
        for (int j = 0; j < FILTER_SAMPLES; j++) rssi_readings[i][j] = 0;
        reading_index[i] = 0;
        rssi_totals[i] = 0;
        sample_counts[i] = 0;
        filtered_rssi[i] = 0.0;
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
                    update_filter(j, rssi);
                    ap_found[j] = true;
                    break;
                }
            }
        }
    } else {
        Serial.println("No networks found.");
    }

    for (int i = 0; i < NUM_APS; i++) {
        if (!ap_found[i]) {
            Serial.print("AP '"); Serial.print(AP_SSIDS[i]); Serial.println("' not found. Retaining previous average.");
        }
    }

    print_status();
    send_data_to_serial();
    delay(2000);
}

void update_filter(int ap_index, int new_rssi) {
    if (sample_counts[ap_index] < FILTER_SAMPLES) sample_counts[ap_index]++;
    else rssi_totals[ap_index] -= rssi_readings[ap_index][reading_index[ap_index]];

    rssi_readings[ap_index][reading_index[ap_index]] = new_rssi;
    rssi_totals[ap_index] += new_rssi;

    reading_index[ap_index]++;
    if (reading_index[ap_index] >= FILTER_SAMPLES) reading_index[ap_index] = 0;

    filtered_rssi[ap_index] = (sample_counts[ap_index] > 0) ? ((double)rssi_totals[ap_index]/sample_counts[ap_index]) : 0.0;
}

double calculate_distance(double rssi) {
    if (rssi == 0.0) return -1.0;
    double exponent = (TX_POWER - rssi)/(10.0 * PATH_LOSS_EXPONENT);
    return pow(10.0, exponent);
}

void print_status() {
    Serial.println("\n--- Current Status ---");
    for (int i = 0; i < NUM_APS; i++) {
        double dist = calculate_distance(filtered_rssi[i]);
        Serial.print("AP: "); Serial.print(AP_SSIDS[i]);
        Serial.print("\tRSSI: "); Serial.print(filtered_rssi[i],2);
        Serial.print(" dBm\tDistance: ");
        if (dist<0) Serial.println("N/A");
        else Serial.println(dist,3);
    }
    Serial.println("----------------------");
}

void send_data_to_serial() {
    Serial.print("DATA:");
    for (int i = 0; i < NUM_APS; i++) {
        double dist = calculate_distance(filtered_rssi[i]);
        if (dist < 0) Serial.print("0.000");
        else Serial.print(dist,3);
        if (i < NUM_APS-1) Serial.print(",");
    }
    Serial.println();
}
