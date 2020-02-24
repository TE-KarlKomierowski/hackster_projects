#include <SPI.h>
#include <LoRa.h>

void setup() {
  Serial.begin(115200);
  while (!Serial);

  Serial.println("LoRa Smart Parking Receiver");
  LoRa.setSPI(SPI5); // SPI of the Spresense main board rather than the extension board.

  LoRa.setPins(18, 26, 25); // Pins connected to the LoRa module.

  if (!LoRa.begin(915E6)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }
}

void loop() {
  // try to parse packet
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    // received a packet
    //Serial.print("Received packet '");

    // read packet
    while (LoRa.available()) {
      Serial.print((char)LoRa.read());
    }
    Serial.println();

    // print RSSI of packet
//    Serial.print("' with RSSI ");
    //Serial.println(LoRa.packetRssi());
  }
}
