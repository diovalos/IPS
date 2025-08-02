# IPS
Wi-Fi-Based Indoor Positioning System Using ESP8266 and RSSI Triangulation
------------------------------------------------------------------------------

🔧 How to Use This Code 
1. Get MAC Addresses of Your 3 Hotspots 

    On Android: Settings → Network → Hotspot → Connected devices (or use Wi-Fi analyzer app)
    On PC/Laptop: Use arp -a or Wi-Fi analyzer tools
    Or check router admin page

3. Calibrate REFERENCE_RSSI 

    Place ESP32 1 meter from a hotspot.
    Upload code and check RSSI value in Serial Monitor.
    Set #define REFERENCE_RSSI to that value (e.g., -38, -42, etc.)
     

4. Upload & Open Serial Monitor 

    Baud: 115200
    You’ll see: RSSI and estimated distance every 2 seconds.


📦 Next Step: Send Data to Computer 

To enable trilateration, send the distances to a Python script on your PC. 
Example: Send via Serial (simple) 

Or via UDP (recommended for wireless): 

Use WiFiUDP.h to send to your PC’s IP and port. 

Let me know if you want the UDP version or the Python trilateration code next! 