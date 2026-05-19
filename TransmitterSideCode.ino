#include <TinyGPSPlus.h>
#include <HardwareSerial.h>

TinyGPSPlus gps;

// GPS UART
HardwareSerial GPSserial(0);

// LoRa UART
HardwareSerial LoRaSerial(1);

unsigned long lastSend = 0;

void setup() {

  Serial.begin(115200);

  // GPS RX, TX
  GPSserial.begin(9600, SERIAL_8N1, 6, 7);

  // LoRa RX, TX
  LoRaSerial.begin(115200, SERIAL_8N1, 4, 5);

  Serial.println("GPS LoRa Sender Ready");
}

void loop() {

  // Read GPS data
  while (GPSserial.available()) {
    gps.encode(GPSserial.read());
  }

  // Valid GPS Fix
  if (gps.location.isValid()) {

    // Send every 5 sec
    if (millis() - lastSend > 5000) {

      String gpsData =
        String(gps.location.lat(), 6) +
        "," +
        String(gps.location.lng(), 6);

      // Create LoRa command
      String cmd =
        "AT+SEND=2," +
        String(gpsData.length()) +
        "," +
        gpsData;

      // Send LoRa
      LoRaSerial.println(cmd);

      Serial.println("GPS Sent:");
      Serial.println(gpsData);

      lastSend = millis();
    }
  }

  // LoRa responses
  while (LoRaSerial.available()) {
    Serial.write(LoRaSerial.read());
  }
}