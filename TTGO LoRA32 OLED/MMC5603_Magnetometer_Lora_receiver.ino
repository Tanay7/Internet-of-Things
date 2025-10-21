
#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <SPI.h>
#include <LoRa.h>
// OLED display configuration
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
// LoRa configuration
const long frequency = 433E6; // Frequency in Hz
const int bandwidth = 62500; // Bandwidth in Hz (62.5 kHz)
const int spreadingFactor = 12; // Spreading factor
const int codingRate = 8; // Coding rate
// LED pin definition
const int ledPin = 25;
void setup() {
  Serial.begin(115200);
  while (!Serial) delay(10); // Wait for Serial Monitor to open
  Serial.println("Setting up OLED...");
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);
  }
  display.display();
  delay(2000); // Pause for 2 seconds
  Serial.println("Setting up LoRa...");
  LoRa.setPins(18, 14, 26); // CS, RST, IRQ
  if (!LoRa.begin(frequency)) { // Frequency set to 433 MHz
    Serial.println("Starting LoRa failed!");
    for (;;);
  }
  LoRa.setSignalBandwidth(bandwidth);
  LoRa.setSpreadingFactor(spreadingFactor);
  LoRa.setCodingRate4(codingRate);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.print("LoRa Receiver");
  display.display();
  // Initialize LED pin
  pinMode(ledPin, OUTPUT);
}
void loop() {
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    // Read packet
    String receivedData = "";
    while (LoRa.available()) {
      receivedData += (char)LoRa.read();
    }
    // Print received data to Serial Monitor
    Serial.println("Received: " + receivedData);
    // Display received data on OLED
    display.clearDisplay();
    display.setCursor(0, 0);
    display.print("Received:");
    display.setCursor(0, 10);
    display.print(receivedData);
    display.display();
    // Flash LED to indicate new data packet received
    digitalWrite(ledPin, HIGH);
    delay(100); // Short delay to keep LED on
    digitalWrite(ledPin, LOW);
  }
}
