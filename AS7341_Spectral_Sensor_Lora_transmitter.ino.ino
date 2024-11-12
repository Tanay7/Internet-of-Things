#include <Wire.h>
#include <Adafruit_AS7341.h>
#include <Adafruit_SSD1306.h>
#include <SPI.h>
#include <LoRa.h>
// OLED display configuration
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
// LED pin definition
const int ledPin = 25;
// Create an instance of the AS7341 sensor
Adafruit_AS7341 as7341;
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
  Serial.println("Setting up AS7341...");
  if (!as7341.begin()) {
    Serial.println("Failed to find AS7341 chip!");
    while (1) delay(10);
  }
  Serial.println("AS7341 found!");
  Serial.println("Setting up LoRa...");
  LoRa.setPins(18, 14, 26); // CS, RST, IRQ
  if (!LoRa.begin(433E6)) { // Frequency set to 433 MHz
    Serial.println("Starting LoRa failed!");
    for (;;);
  }
  LoRa.setTxPower(20);
  LoRa.setSignalBandwidth(62500);
  LoRa.setSpreadingFactor(12);
  LoRa.setCodingRate4(8);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.print("AS7341 & LoRa");
  display.display();
  // Initialize LED pin
  pinMode(ledPin, OUTPUT);
}
void loop() {
  // Trigger a measurement
  if (!as7341.readAllChannels()) {
    Serial.println("Error reading all channels!");
    return;
  }
  // Print color readings using channel constants defined in your library
  Serial.print("Clear: "); Serial.print(as7341.getChannel(AS7341_CHANNEL_CLEAR));
  Serial.print(" F1 (415nm): "); Serial.print(as7341.getChannel(AS7341_CHANNEL_415nm_F1));
  Serial.print(" F2 (445nm): "); Serial.print(as7341.getChannel(AS7341_CHANNEL_445nm_F2));
  Serial.print(" F3 (480nm): "); Serial.print(as7341.getChannel(AS7341_CHANNEL_480nm_F3));
  Serial.print(" F4 (515nm): "); Serial.print(as7341.getChannel(AS7341_CHANNEL_515nm_F4));
  Serial.print(" F5 (555nm): "); Serial.print(as7341.getChannel(AS7341_CHANNEL_555nm_F5));
  Serial.print(" F6 (590nm): "); Serial.print(as7341.getChannel(AS7341_CHANNEL_590nm_F6));
  Serial.print(" F7 (630nm): "); Serial.print(as7341.getChannel(AS7341_CHANNEL_630nm_F7));
  Serial.print(" F8 (680nm): "); Serial.print(as7341.getChannel(AS7341_CHANNEL_680nm_F8));
  Serial.print(" Infrared: "); Serial.println(as7341.getChannel(AS7341_CHANNEL_NIR));
  // Display on OLED
  display.clearDisplay();
  display.setCursor(0, 0);
  display.print("Clear: "); display.print(as7341.getChannel(AS7341_CHANNEL_CLEAR));
  display.print(" F1 (415nm): "); display.print(as7341.getChannel(AS7341_CHANNEL_415nm_F1));
  display.print(" F2 (445nm): "); display.print(as7341.getChannel(AS7341_CHANNEL_445nm_F2));
  display.print(" F3 (480nm): "); display.print(as7341.getChannel(AS7341_CHANNEL_480nm_F3));
  display.print(" F4 (515nm): "); display.print(as7341.getChannel(AS7341_CHANNEL_515nm_F4));
  display.print(" F5 (555nm): "); display.print(as7341.getChannel(AS7341_CHANNEL_555nm_F5));
  display.print(" F6 (590nm): "); display.print(as7341.getChannel(AS7341_CHANNEL_590nm_F6));
  display.print(" F7 (630nm): "); display.print(as7341.getChannel(AS7341_CHANNEL_630nm_F7));
  display.print(" F8 (680nm): "); display.print(as7341.getChannel(AS7341_CHANNEL_680nm_F8));
  display.print(" Infrared: "); display.print(as7341.getChannel(AS7341_CHANNEL_NIR));
  display.display();
  // Send data over LoRa
  String data = String(as7341.getChannel(AS7341_CHANNEL_CLEAR)) + "," +
                String(as7341.getChannel(AS7341_CHANNEL_415nm_F1)) + "," +
                String(as7341.getChannel(AS7341_CHANNEL_445nm_F2)) + "," +
                String(as7341.getChannel(AS7341_CHANNEL_480nm_F3)) + "," +
                String(as7341.getChannel(AS7341_CHANNEL_515nm_F4)) + "," +
                String(as7341.getChannel(AS7341_CHANNEL_555nm_F5)) + "," +
                String(as7341.getChannel(AS7341_CHANNEL_590nm_F6)) + "," +
                String(as7341.getChannel(AS7341_CHANNEL_630nm_F7)) + "," +
                String(as7341.getChannel(AS7341_CHANNEL_680nm_F8)) + "," +
                String(as7341.getChannel(AS7341_CHANNEL_NIR));
  LoRa.beginPacket();
  LoRa.print(data);
  LoRa.endPacket();
  digitalWrite(ledPin, HIGH);
  delay(100); // Short delay to keep LED on
  digitalWrite(ledPin, LOW);
  delay(1000); // Adjust delay as needed
}