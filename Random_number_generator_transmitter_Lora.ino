
#include <SPI.h>
#include <LoRa.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
const int sendInterval = 2000; // 2 seconds (not used anymore)
unsigned long previousMillis = 0;
unsigned long transmitCount = 0;
// LoRa configuration parameters
const long frequency = 433E6; // Frequency in Hz
const int bandwidth = 62500; // Bandwidth in Hz (62.5 kHz)
const int spreadingFactor = 12; // Spreading factor
const int codingRate = 8; // Coding rate
// Timing for screen rotation
unsigned long lastDisplayChangeTime = 0;
unsigned long displayChangeInterval = 5000; // 5 seconds for each screen
int screen = 0;
void setup() {
    Serial.begin(9600);
    while (!Serial);
    
    Serial.println("Setting up OLED...");
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Change 0x3C to your I2C address
        Serial.println(F("SSD1306 allocation failed"));
        for (;;);
    }
    display.display();
    delay(2000); // Pause for 2 seconds
    Serial.println("Setting up LoRa...");
    LoRa.setPins(18, 14, 26); // CS, RST, IRQ
    if (!LoRa.begin(frequency)) { // Initialize LoRa with the defined frequency
        Serial.println("Starting LoRa failed!");
        for (;;); // Stay in loop if LoRa setup fails
    }
    LoRa.setSignalBandwidth(bandwidth);
    LoRa.setSpreadingFactor(spreadingFactor);
    LoRa.setCodingRate4(codingRate);
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.print("LoRa Sender");
    display.display();
    Serial.println("LoRa setup successful");
    // Seed the random number generator with a value from analog pin (e.g., A0)
    randomSeed(analogRead(0));
}
void loop() {
    unsigned long currentMillis = millis();
    // Check if it's time to switch the display screen
    if (currentMillis - lastDisplayChangeTime >= displayChangeInterval) {
        lastDisplayChangeTime = currentMillis;
        screen = (screen + 1) % 2; // Toggle between two screens
    }
    if (currentMillis - previousMillis >= sendInterval) {
        previousMillis = currentMillis;
        transmitCount++;
        // Generate a random number between a specified range (e.g., 0 to 9999)
        int randomNumber = random(10000); // Generates a random number from 0 to 9999
        Serial.print("Sending message: ");
        Serial.println(randomNumber);
        LoRa.beginPacket();
        LoRa.print(randomNumber); // Sending the random number as a message
        LoRa.endPacket();
        Serial.println("Message sent");
        // Clear only part of the display for new information while keeping the sending message at the top
        display.clearDisplay(); // Clear the entire display first
        // Always show the sending message at the top
        display.setCursor(0, 0);
        display.print("Sending: ");
        display.println(randomNumber); // Display the random number being sent
        
        if (screen == 0) {
            display.setCursor(0, 20);
            display.print("Message sent");
            display.setCursor(0, 30);
            display.print("Tx Power (dBm): ");
            display.println(LoRa.packetRssi()); // Display transmit power
            display.setCursor(0, 40);
            display.print("Freq (MHz): ");
            display.println(frequency / 1E6); // Convert Hz to MHz
            
        } else {
            display.setCursor(0, 20);
            display.print("Sp. Factor: ");
            display.println(spreadingFactor); // Display spreading factor
            
            display.setCursor(0, 30);
            display.print("Coding Rate: ");
            display.println(codingRate); // Display coding rate
            
            display.setCursor(0, 40);
            display.print("BW (kHz): ");
            display.println(bandwidth / 1000); // Convert Hz to kHz
            
            display.setCursor(0, 50);
            display.print("Freq Error: ");
            display.println(LoRa.packetFrequencyError()); // Display frequency error from last received packet
        }
        // Update the OLED with new information
        display.display();
        
        delay(3000); // Pause for **3 seconds** before sending new data
        
        screen = (screen + 1) % 2; // Toggle between two screens after sending message.
    }
}
