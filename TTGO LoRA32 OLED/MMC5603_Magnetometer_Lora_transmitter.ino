
#include <Wire.h>
#include <Adafruit_MMC56x3.h>
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
// Create an instance of the MMC5603 sensor
Adafruit_MMC5603 mmc = Adafruit_MMC5603();
unsigned long previousDisplayMillis = 0; // Stores the last time display was updated
unsigned long previousSendMillis = 0; // Stores the last time data was sent
const long displayInterval = 2000; // Interval at which to refresh display (2 seconds)
const long sendInterval = 5000; // Interval at which to send data (5 seconds)
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
  Serial.println("Setting up MMC5603...");
  if (!mmc.begin(MMC56X3_DEFAULT_ADDRESS, &Wire)) {
    Serial.println("Failed to find MMC5603 chip!");
    while (1) delay(10);
  }
  Serial.println("MMC5603 found!");
  Serial.println("Setting up LoRa...");
  LoRa.setPins(18, 14, 26); // CS, RST, IRQ
  if (!LoRa.begin(frequency)) { // Frequency set to 433 MHz
    Serial.println("Starting LoRa failed!");
    for (;;);
  }
  LoRa.setTxPower(20);
  LoRa.setSignalBandwidth(bandwidth);
  LoRa.setSpreadingFactor(spreadingFactor);
  LoRa.setCodingRate4(codingRate);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.print("MMC5603");
  display.display();
  // Initialize LED pin
  pinMode(ledPin, OUTPUT);
}
void loop() {
  unsigned long currentMillis = millis();
  // Update and display sensor readings every 2 seconds
  if (currentMillis - previousDisplayMillis >= displayInterval) {
    previousDisplayMillis = currentMillis;
    sensors_event_t event;
    mmc.getEvent(&event);
    display.clearDisplay();
    display.setCursor(0, 0);
    display.print("X: "); display.print(event.magnetic.x); display.println(" uT");
    display.print("Y: "); display.print(event.magnetic.y); display.println(" uT");
    display.print("Z: "); display.print(event.magnetic.z); display.println(" uT");
    display.display();
  }
  // Send data every 5 seconds
  if (currentMillis - previousSendMillis >= sendInterval) {
    previousSendMillis = currentMillis;
    sensors_event_t event;
    mmc.getEvent(&event);
    String data = "X: " + String(event.magnetic.x) + " uT, " +
                  "Y: " + String(event.magnetic.y) + " uT, " +
                  "Z: " + String(event.magnetic.z) + " uT";
    LoRa.beginPacket();
    LoRa.print(data);
    LoRa.endPacket();
    Serial.println("Data sent: " + data);
    // Flash LED to indicate data packet sent
    digitalWrite(ledPin, HIGH);
    delay(100); // Short delay to keep LED on
    digitalWrite(ledPin, LOW);
  }
  delay(150); // Adjust delay as needed to balance refresh rate
}
