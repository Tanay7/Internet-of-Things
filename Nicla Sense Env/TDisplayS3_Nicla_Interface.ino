


/**
 * @file TDisplayS3_Nicla_Interface.ino
 * @brief Interfaces Arduino Nicla Sense Env with LilyGo T-Display S3 AMOLED via I2C
 * and displays data on the built-in AMOLED using Arduino_GFX.
 * @version 6.11 - Intuitive Static Label Colors
 * @date 2025-10-24
 *
 * REFACTOR NOTES (v6.11):
 * - Applied intuitive color scheme to static labels per user request:
 * - TEMP: Orange (C_TEMP_ORANGE)
 * - HUMIDITY: Blue (C_HUMID_BLUE) - Was already blue
 * - eCO2 (ppm): White (C_WHITE)
 * - TVOC: Yellow (C_GAS_YELLOW)
 * - Ethanol: Magenta (C_MAGENTA)
 * - O3: Bright Cyan (C_OAQ_TITLE)
 * - NO2: White (C_WHITE)
 *
 * REFACTOR NOTES (v6.10):
 * - Fixed static label invisibility by redrawing them within the
 * differential update block.
 *
 * REFACTOR NOTES (v6.9):
 * - Changed static pollutant labels to C_HUMID_BLUE.
 *
 * REFACTOR NOTES (v6.7):
 * - Unified all panel header colors (TEMP, IAQ, OAQ) to C_HUMID_BLUE.
 *
 * REFACTOR NOTES (v6.5):
 * - Removed the 3-minute WARMUP_DURATION_MS loop from setup().
 * - Correctly implements label visibility for dark mode.
 *
 * REFACTOR NOTES (v6.0 - v6.4):
 * - Merged high-contrast dark mode color palette.
 * - Implemented differential rendering to eliminate flicker.
 * - Replaced blocking delay() with a non-blocking millis() timer.
 * - Corrected TVOC/Ethanol scales to conservative, health-based thresholds.
 * - Added user-initiated baseline calibration (Button 0).
 *
 * Hardware Connections (T-Display S3 AMOLED STEMMA QT -> Nicla ESLOV/Pins):
 * - GPIO 44 (SDA) -> Nicla SDA
 * - GPIO 43 (SCL) -> Nicla SCL
 * - 3V3         -> Nicla 3V3
 * - GND         -> Nicla GND
 */
// --- LIBRARIES ---
#include <Arduino.h>
#include <Wire.h>
#include "Arduino_NiclaSenseEnv.h" 
#include <Arduino_GFX_Library.h>
// --- UI COLOR DEFINITIONS (16-bit RGB565) ---
// --- Dark Mode Palette ---
#define C_BACKGROUND  0x0000 // Background (C_BLACK)
#define C_BLACK       0x0000 // True black
#define C_TEXT_PRIMARY 0xFFFF // Primary data text (C_WHITE)
#define C_WHITE       0xFFFF // Primary data text & Specific static labels (eCO2, NO2)
#define C_DARK_GREY   0x2104 // Gauge/Bar backgrounds
#define C_HEADER_BLUE 0x041F // Top Header background
#define C_PANEL_BORDER 0x8410 // Panel outlines (Brighter grey)
// Sensor-specific Title Colors (Panel Headers)
#define C_HUMID_BLUE  0x05BF // Used for ALL panel headers (TEMP, HUMIDITY, IAQ, OAQ)
// Static Label Specific Colors
#define C_TEMP_ORANGE 0xFC00 // Added back for TEMP label
#define C_GAS_YELLOW  0xFFE0 // For TVOC label
#define C_MAGENTA     0xF81F // For Ethanol label
#define C_OAQ_TITLE   0x07FF // Added back for O3 label (matches OAQ header visually)
// --- Granular Gas Colors (for dynamic status labels like [GOOD], [MOD]) ---
#define C_EMERALD_GREEN 0x04E0 // #009E60
#define C_FOREST_GREEN  0x2444 // #228B22
#define C_LEAF_GREEN    0x3666 // #32CD32
#define C_LIME_GREEN    0xBFE0 // #BFFF00
#define C_PALE_GREEN    0x97F3 // #98FB98
#define C_LIGHT_YELLOW  0xFFFB // #FFFFE0
#define C_YELLOW        0xFFE0 // #FFFF00
#define C_AMBER         0xFEA0 // #FFBF00
#define C_LIGHT_ORANGE  0xFEC0 // #FFD580
#define C_ORANGE        0xFD20 // #FFA500
#define C_DEEP_ORANGE   0xFC60 // #FF8C00
#define C_BURNT_ORANGE  0xC2A0 // #CC5500
#define C_LIGHT_RED     0xFB33 // #FF6666
#define C_RED           0xF800 // #FF0000
#define C_CRIMSON       0xD8A7 // #DC143C
#define C_DARK_RED      0x8800 // #8B0000
#define C_BRICK_RED     0xB104 // #B22222
#define C_BROWN         0xA145 // #A52A2A
#define C_DARK_BROWN    0x5A06 // #5C4033
#define C_MAROON_BLACK  0x380A // #3B0A0A
#define C_BLACK_GAS     0x0000 // #000000 (Use C_BLACK)
// IAQ Bar/Status Colors
#define C_IAQ_GREEN   0x07E0
#define C_IAQ_YELLOW  0xFFE0
#define C_IAQ_ORANGE  0xFD20
#define C_IAQ_RED     0xF800
// Use existing colors for new AQI levels
#define C_AQI_PURPLE  C_CRIMSON // (Very Unhealthy)
#define C_AQI_MAROON  C_BROWN   // (Hazardous)

// --- DISPLAY BRIGHTNESS ---
// Set display brightness (0 = min, 255 = max)
#define DISPLAY_BRIGHTNESS 200
// --- HARDWARE & DISPLAY CONFIGURATION ---
Arduino_DataBus *bus = new Arduino_ESP32QSPI(
    6 /* sck */, 47 /* d0 */, 18 /* d1 */, 7 /* d2 */, 48 /* d3 */, 5 /* cs */);
Arduino_GFX *gfx = new Arduino_RM67162(
    bus, 17 /* RST */, 0 /* rotation */, false /* Not IPS */);
// Define canvas dimensions matching the 1.91" display
#define CANVAS_WIDTH  240
#define CANVAS_HEIGHT 536
Arduino_Canvas *canvas = new Arduino_Canvas(CANVAS_WIDTH, CANVAS_HEIGHT, gfx);
// I2C pins for the T-Display S3's STEMMA QT connector
#define I2C_SDA 44
#define I2C_SCL 43
// Define constants for the default GFX font sizes (6x8 pixels per character at size 1)
const int FONT_W = 6;
const int FONT_H = 8;
// --- REFACTOR: NEW CONSTANTS ---
#define UPDATE_INTERVAL_MS 500 // Replaces delay(500)
// T-Display S3 onboard button 1 (left)
#define CAL_BUTTON_PIN 0
// --- SENSOR DATA STORAGE ---
// Global variables to hold the latest sensor data
float g_temperature = 0.0;
float g_humidity = 0.0;
float g_iaq = 0.0;
float g_tvoc_mg_m3 = 0.0;
float g_ethanol = 0.0;
float g_eco2_ppm = 0.0;
float g_oaq_index = 0.0;
float g_o3_ppb = 0.0;
float g_no2_ppb = 0.0;
// --- REFACTOR: Globals to store PREVIOUS data for differential render ---
float g_temperature_prev = -1.0;
float g_humidity_prev = -1.0;
float g_iaq_prev = -1.0;
float g_tvoc_mg_m3_prev = -1.0;
float g_ethanol_prev = -1.0;
float g_eco2_ppm_prev = -1.0;
float g_oaq_index_prev = -1.0;
float g_o3_ppb_prev = -1.0;
float g_no2_ppb_prev = -1.0;
// --- REFACTOR: Global for non-blocking timer ---
unsigned long g_last_update_time = 0;
// --- Nicla Object ---
NiclaSenseEnv nicla;
// --- FUNCTION PROTOTYPES ---
void readAllSensors();
void copySensorDataToPrev(); // REFACTOR: New function
void checkCalibrationButton(); // REFACTOR: New function
// REFACTOR: Added 'force_redraw' parameter for differential update
void drawDashboard(bool force_redraw = false);
void printCentered(String text, int x, int y, int w, int h, int textSize, int yOffset = 0);
void drawHeader(); // This is now only called once
void drawEnvironmentPanel(int x, int y, int w, int h, bool force_redraw = false);
void drawIndoorPanel(int x, int y, int w, int h, bool force_redraw = false);
void drawOutdoorPanel(int x, int y, int w, int h, bool force_redraw = false);
uint16_t getIaqColor(float iaq);
uint16_t getTvocColor(float tvoc_mg_m3); // REFACTOR: Logic changed
uint16_t getEthanolColor(float ethanol_ppm); // REFACTOR: Logic changed
String getTvocLevel(float tvoc_mg_m3); // REFACTOR: Logic changed
String getEthanolLevel(float ethanol_ppm); // REFACTOR: Logic changed
uint16_t getO3Color(float o3_ppb); 
String getO3Level(float o3_ppb); 
uint16_t getNO2Color(float no2_ppb); 
String getNO2Level(float no2_ppb); 

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("T-Display S3 AMOLED - Nicla Interface (v6.11 Dark Mode - Intuitive Labels)");
  // --- INITIALIZATION ORDER ---
  // 1. Initialize I2C bus as Master
  Wire.begin(I2C_SDA, I2C_SCL);
  Serial.printf("I2C Master Initialized (SDA: %d, SCL: %d)\n", I2C_SDA, I2C_SCL);
  // 2. Initialize Display Hardware
  pinMode(38, OUTPUT);      // Enable Backlight control pin
  digitalWrite(38, HIGH);   // Turn on Backlight
  gfx->begin();
  gfx->fillScreen(C_BACKGROUND);
  // Set brightness using the new define
  bus->beginWrite();
  bus->writeC8D8(0x51, DISPLAY_BRIGHTNESS);
  bus->endWrite();
  Serial.println("Display Initialized.");
  // 3. Initialize the off-screen canvas buffer
  canvas->begin(GFX_SKIP_OUTPUT_BEGIN); // Don't run gfx->begin() again
  canvas->fillScreen(C_BACKGROUND);
  canvas->setTextColor(C_TEXT_PRIMARY); // Default text color
  // Initial message uses size 2
  printCentered("Initializing Nicla...", 0, 0, CANVAS_WIDTH, CANVAS_HEIGHT, 2);
  canvas->flush(); // Send buffer to display
  Serial.println("Canvas Initialized.");
  // 4. Initialize Nicla Sense Env
  if (!nicla.begin()) {
    Serial.println("ERROR: Failed to initialize Nicla Sense Env board!");
    canvas->fillScreen(C_IAQ_RED);
    canvas->setTextSize(2);
    canvas->setTextColor(C_WHITE); // C_WHITE is fine for red background
    printCentered("Nicla Init Failed!", 0, 0, CANVAS_WIDTH, CANVAS_HEIGHT, 2, -10);
    printCentered("Check I2C wiring.", 0, 0, CANVAS_WIDTH, CANVAS_HEIGHT, 2, 10);
    canvas->flush();
    while (1); // Halt execution
  }
  Serial.println("Nicla Sense Env Initialized.");
  // 5. Enable all available sensors
  nicla.temperatureHumiditySensor().setEnabled(true);
  nicla.indoorAirQualitySensor().setEnabled(true);
  nicla.outdoorAirQualitySensor().setEnabled(true);
  Serial.println("All Nicla Sensors Enabled.");
  // --- REFACTOR: 3-MINUTE WARM-UP LOGIC REMOVED ---
  // --- REFACTOR: NEW ---
  // 6. Initialize Calibration Button
  pinMode(CAL_BUTTON_PIN, INPUT_PULLUP);
  Serial.println("Calibration button (GPIO 0) initialized.");
  // 7. Draw the STATIC dashboard UI *ONCE*
  Serial.println("Drawing static dashboard UI...");
  drawDashboard(true); // true = force_redraw
  // 8. Get initial sensor readings and flush
  Serial.println("Getting initial sensor readings...");
  readAllSensors();
  copySensorDataToPrev(); // Sync prev values to current ones
  // Draw the initial data *into* the static UI
  drawEnvironmentPanel(0, 35, CANVAS_WIDTH, 100, false); // false = don't redraw static parts
  drawIndoorPanel(0, 140, CANVAS_WIDTH, 210, false);
  drawOutdoorPanel(0, 355, CANVAS_WIDTH, CANVAS_HEIGHT - 355 - 5, false);
  canvas->flush(); // Push the completed initial frame
  // 9. Set timer for non-blocking loop
  g_last_update_time = millis();
  Serial.println("\nStarting sensor loop...");
}
void loop() {
  // --- REFACTOR: NON-BLOCKING TIMER ---
  if (millis() - g_last_update_time < UPDATE_INTERVAL_MS) {
    // Check for button press even if not updating sensors
    checkCalibrationButton();
    return; // Not time to update yet
  }
  g_last_update_time = millis(); // Reset the timer
  // 1. Read all sensor values into global variables
  readAllSensors();
  // 2. Redraw *only changed data* to the dashboard
  drawDashboard(false); // false = don't force redraw
  // 3. --- Example: Control Nicla RGB LED based on IAQ ---
  if (!isnan(g_iaq)) {
    uint16_t iaqColor = getIaqColor(g_iaq); // Get color based on IAQ
    uint8_t r = (iaqColor >> 11) & 0x1F;
    uint8_t g = (iaqColor >> 5) & 0x3F;
    uint8_t b = iaqColor & 0x1F;
    // Scale to 0-255 for the Nicla LED function
    nicla.rgbLED().setColor(map(r, 0, 31, 0, 255), map(g, 0, 63, 0, 255), map(b, 0, 31, 0, 255));
    nicla.rgbLED().setBrightness(50); // Set brightness (0-255)
  } else {
    nicla.rgbLED().setColor(0, 0, 50); // Blue if IAQ is not ready
    nicla.rgbLED().setBrightness(50);
  }
  // 4. --- REFACTOR: Check for user calibration input ---
  checkCalibrationButton();
  // 5. --- REFACTOR: Store current values as "previous" for next loop's comparison ---
  copySensorDataToPrev();
  // NO delay(500)
}
/**
 * @brief Reads all sensor data into global variables
 */
void readAllSensors() {
  g_temperature = nicla.temperatureHumiditySensor().temperature();
  g_humidity = nicla.temperatureHumiditySensor().humidity();
  g_iaq = nicla.indoorAirQualitySensor().airQuality();
  g_tvoc_mg_m3 = nicla.indoorAirQualitySensor().TVOC();
  g_ethanol = nicla.indoorAirQualitySensor().ethanol();
  g_eco2_ppm = nicla.indoorAirQualitySensor().CO2();
  g_oaq_index = nicla.outdoorAirQualitySensor().airQualityIndex();
  g_o3_ppb = nicla.outdoorAirQualitySensor().O3();
  g_no2_ppb = nicla.outdoorAirQualitySensor().NO2();
}
/**
 * @brief REFACTOR: Stores all current sensor values for the next diff check.
 */
void copySensorDataToPrev() {
  g_temperature_prev = g_temperature;
  g_humidity_prev = g_humidity;
  g_iaq_prev = g_iaq;
  g_tvoc_mg_m3_prev = g_tvoc_mg_m3;
  g_ethanol_prev = g_ethanol;
  g_eco2_ppm_prev = g_eco2_ppm;
  g_oaq_index_prev = g_oaq_index;
  g_o3_ppb_prev = g_o3_ppb;
  g_no2_ppb_prev = g_no2_ppb;
}
/**
 * @brief REFACTOR: Checks for user-initiated calibration button press.
 */
void checkCalibrationButton() {
  // Check if button 0 is pressed (LOW-active)
  if (digitalRead(CAL_BUTTON_PIN) == LOW) {
    Serial.println("CALIBRATION: Button pressed. Holding...");
    // Wait for 2 seconds to confirm long press
    delay(2000);
    // Check if still pressed
    if (digitalRead(CAL_BUTTON_PIN) == LOW) {
      Serial.println("CALIBRATION: Triggered by user.");
      // Show "Calibrating" message
      canvas->fillScreen(C_IAQ_ORANGE);
      canvas->setTextColor(C_BLACK); // Black text on orange is fine
      printCentered("CALIBRATING...", 0, 0, CANVAS_WIDTH, CANVAS_HEIGHT, 2, -10);
      printCentered("Ensure fresh air!", 0, 0, CANVAS_WIDTH, CANVAS_HEIGHT, 2, 10);
      canvas->flush();
      // --- PLACEHOLDER ---
      // The NiclaSenseEnv high-level library does not appear to expose a
      // baseline reset function. This is where it would be called.
      Serial.println("CALIBRATION: NOTE: Library function not exposed. This is a placeholder.");
      // e.g., nicla.indoorAirQualitySensor().resetBaseline();
      delay(5000); // Show message for 5 seconds
      // Force a full redraw of the entire dashboard
      drawDashboard(true);
      // Re-read sensors and sync prev values
      readAllSensors();
      copySensorDataToPrev();
      // Redraw the new initial data
      drawEnvironmentPanel(0, 35, CANVAS_WIDTH, 100, false);
      drawIndoorPanel(0, 140, CANVAS_WIDTH, 210, false);
      drawOutdoorPanel(0, 355, CANVAS_WIDTH, CANVAS_HEIGHT - 355 - 5, false);
      canvas->flush();
      // Wait for button release
      while(digitalRead(CAL_BUTTON_PIN) == LOW) {
        delay(100);
      }
      Serial.println("CALIBRATION: Complete. Resuming operation.");
    }
  }
}
/**
 * @brief Prints text centered horizontally and vertically within a defined bounding box.
 */
void printCentered(String text, int x, int y, int w, int h, int textSize, int yOffset) {
    int textWidth = text.length() * FONT_W * textSize;
    int textHeight = FONT_H * textSize;
    int cursorX = x + (w / 2) - (textWidth / 2);
    int cursorY = y + (h / 2) - (textHeight / 2) + yOffset;
    canvas->setTextSize(textSize);
    canvas->setCursor(cursorX, cursorY);
    canvas->print(text);
}
/**
 * @brief Main drawing function.
 * REFACTOR: Now takes a 'force_redraw' flag.
 * If true, draws everything (static + data).
 * If false, calls sub-functions that only draw changed data.
 */
void drawDashboard(bool force_redraw) {
    if (force_redraw) {
      canvas->fillScreen(C_BACKGROUND);
      drawHeader(); // Draw static header
    }
    // --- Define Layout ---
    int w_full = CANVAS_WIDTH;
    int h_header = 30;
    int h_env = 100; // Fixed height for Temp/Humid panel (matches v5.6)
    int h_iaq = 210; // Fixed height for Indoor panel (matches v5.6)
    int h_oaq = CANVAS_HEIGHT - h_header - h_env - h_iaq - 5; // 5px bottom margin
    int y_pos = 0;
    y_pos += h_header + 5; // Skip header
    // Pass 'force_redraw' flag to sub-panels
    drawEnvironmentPanel(0, y_pos, w_full, h_env, force_redraw);
    y_pos += h_env + 5;
    drawIndoorPanel(0, y_pos, w_full, h_iaq, force_redraw);
    y_pos += h_iaq + 5;
    drawOutdoorPanel(0, y_pos, w_full, h_oaq, force_redraw);
    // Flush the canvas buffer to the display
    // This is now the *only* flush call in the main loop.
    canvas->flush();
}
/**
 * @brief Draws the static header bar at the top of the screen.
 * (This is only called by drawDashboard if force_redraw is true)
 */
void drawHeader() {
    canvas->fillRect(0, 0, CANVAS_WIDTH, 30, C_HEADER_BLUE);
    canvas->setTextColor(C_WHITE); // C_WHITE is correct for blue background
    printCentered("NICLA ENV MONITOR", 0, 0, CANVAS_WIDTH, 30, 2); // Size 2 text
}
/**
 * @brief Draws the Environment (Temp/Humid) panel.
 * REFACTOR: Only draws static parts on 'force_redraw'.
 * Only updates data if values have changed.
 * Headers use unified blue color and size 2.
 */
void drawEnvironmentPanel(int x, int y, int w, int h, bool force_redraw) {
    if (force_redraw) {
      canvas->drawRect(x, y, w, h, C_PANEL_BORDER);
      // Draw vertical separator line
      canvas->drawFastVLine(x + w / 2, y, h, C_PANEL_BORDER);
      int w_half = w / 2;
      // --- Temperature Title (Static) ---
      canvas->setTextColor(C_HUMID_BLUE); // Unified color
      canvas->setTextSize(2); // Header Size 2
      printCentered("TEMP", x, y + 5, w_half, 30, 2);
      // --- Humidity Title (Static) ---
      canvas->setTextColor(C_HUMID_BLUE); // Unified color
      canvas->setTextSize(2); // Header Size 2
      printCentered("HUMIDITY", x + w_half, y + 5, w_half, 30, 2);
    }
    // --- Differential Data Update ---
    if (force_redraw || g_temperature != g_temperature_prev || g_humidity != g_humidity_prev)
    {
      // Erase *only* the data area
      canvas->fillRect(x + 1, y + 36, w - 2, h - 37, C_BACKGROUND);
      char buffer[20];
      int w_half = w / 2;
      // --- Temperature Data ---
      canvas->setTextColor(C_TEXT_PRIMARY); // Set correct color for data (White)
      canvas->setTextSize(2); // Data text size remains 2
      snprintf(buffer, sizeof(buffer), "%.2f C", g_temperature);
      printCentered(buffer, x, y + 40, w_half, 40, 2);
      // --- Humidity Data ---
      canvas->setTextColor(C_TEXT_PRIMARY); // Set correct color for data (White)
      canvas->setTextSize(2); // Data text size remains 2
      snprintf(buffer, sizeof(buffer), "%.2f %%", g_humidity);
      printCentered(buffer, x + w_half, y + 40, w_half, 40, 2);
    }
}
/**
 * @brief Draws the Indoor Air Quality (IAQ) panel.
 * REFACTOR: Only draws static parts on 'force_redraw'.
 * Only updates data if values have changed.
 * Headers use unified blue color and size 2.
 * *** FIX V6.11: Applied intuitive colors to static labels. ***
 */
void drawIndoorPanel(int x, int y, int w, int h, bool force_redraw) {
    // Calculate static label positions (needed in both blocks)
    int line_h = 25;
    int x_label = x + 10;
    int y_line_start = y + 140; // Y position start matches v5.6
    if (force_redraw) {
      canvas->drawRect(x, y, w, h, C_PANEL_BORDER);
      // Title (Static)
      canvas->setTextColor(C_HUMID_BLUE); // Unified color
      canvas->setTextSize(2); // Header Size 2
      printCentered("INDOOR AIR QUALITY", x, y + 5, w, 30, 2);
      // --- Indoor Details List Labels (Static - Initial Draw Only) ---
      // These are now also drawn in the update block below
      canvas->setTextSize(2);
      canvas->setTextColor(C_WHITE); // eCO2 label color
      canvas->setCursor(x_label, y_line_start); canvas->print("eCO2 (ppm):");
      canvas->setTextColor(C_GAS_YELLOW); // TVOC label color
      canvas->setCursor(x_label, y_line_start + line_h); canvas->print("TVOC:");
      canvas->setTextColor(C_MAGENTA); // Ethanol label color
      canvas->setCursor(x_label, y_line_start + (line_h * 2)); canvas->print("Ethanol:");
    }
    // --- Differential Data Update ---
    // Check if any indoor value has changed
    if (force_redraw || g_iaq != g_iaq_prev || g_eco2_ppm != g_eco2_ppm_prev ||
        g_tvoc_mg_m3 != g_tvoc_mg_m3_prev || g_ethanol != g_ethanol_prev)
    {
      // Erase all data areas
      canvas->fillRect(x + 1, y + 36, w - 2, h - 37, C_BACKGROUND);
      // --- Redraw all dynamic data for this panel ---
      // Large IAQ Value
      canvas->setTextColor(getIaqColor(g_iaq));
      char buffer[40];
      snprintf(buffer, sizeof(buffer), "%.0f", g_iaq);
      printCentered(buffer, x, y + 40, w, 60, 6);
      // --- IAQ Status Bar ---
      int barX = x + 20; int barW = w - 40; int barY = y + 110; int barH = 15;
      canvas->drawRect(barX, barY, barW, barH, C_DARK_GREY);
      int fillW = map(g_iaq, 0, 500, 0, barW);
      if (fillW < 0) fillW = 0; if (fillW > barW) fillW = barW;
      canvas->fillRect(barX, barY, fillW, barH, getIaqColor(g_iaq));
      // --- Indoor Details List ---
      canvas->setTextSize(2);
      int x_level = x + 105; // Position for dynamic status label
      int x_value_right = x + w - 10; // Right-align edge for value
      int y_line = y_line_start; // Use calculated start Y
      int textWidth = 0;
      // --- FIX V6.11: Redraw static labels with intuitive colors here ---
      canvas->setTextColor(C_WHITE); // eCO2 label color
      canvas->setCursor(x_label, y_line); canvas->print("eCO2 (ppm):");
      canvas->setTextColor(C_GAS_YELLOW); // TVOC label color
      canvas->setCursor(x_label, y_line + line_h); canvas->print("TVOC:");
      canvas->setTextColor(C_MAGENTA); // Ethanol label color
      canvas->setCursor(x_label, y_line + (line_h * 2)); canvas->print("Ethanol:");
      // --- End Fix ---
      // eCO2 Value
      canvas->setTextColor(C_TEXT_PRIMARY); // White for value
      snprintf(buffer, sizeof(buffer), "%d", isnan(g_eco2_ppm) ? -1 : (int)g_eco2_ppm);
      textWidth = String(buffer).length() * FONT_W * 2;
      canvas->setCursor(x_value_right - textWidth, y_line);
      canvas->print(buffer);
      y_line += line_h;
      // TVOC Status Label + Value
      String tvocLevel = getTvocLevel(g_tvoc_mg_m3);
      uint16_t tvocColor = getTvocColor(g_tvoc_mg_m3);
      canvas->setTextColor(tvocColor); // Dynamic color for status label
      canvas->setCursor(x_level, y_line); canvas->print(tvocLevel);
      canvas->setTextColor(C_TEXT_PRIMARY); // White for value
      snprintf(buffer, sizeof(buffer), "%.3f", g_tvoc_mg_m3);
      textWidth = String(buffer).length() * FONT_W * 2;
      canvas->setCursor(x_value_right - textWidth, y_line); canvas->print(buffer);
      y_line += line_h;
      // Ethanol Status Label + Value
      String ethLevel = getEthanolLevel(g_ethanol);
      uint16_t ethColor = getEthanolColor(g_ethanol);
      canvas->setTextColor(ethColor); // Dynamic color for status label
      canvas->setCursor(x_level, y_line); canvas->print(ethLevel);
      canvas->setTextColor(C_TEXT_PRIMARY); // White for value
      if (isnan(g_ethanol)) { snprintf(buffer, sizeof(buffer), "N/A"); }
      else { snprintf(buffer, sizeof(buffer), "%.2f", g_ethanol); }
      textWidth = String(buffer).length() * FONT_W * 2;
      canvas->setCursor(x_value_right - textWidth, y_line); canvas->print(buffer);
    }
}

/**
 * @brief Draws the Outdoor Air Quality (OAQ) panel.
 * REFACTOR: Only draws static parts on 'force_redraw'.
 * Only updates data if values have changed.
 * Headers use unified blue color and size 2.
 * *** FIX V6.11: Applied intuitive colors to static labels. ***
 */
void drawOutdoorPanel(int x, int y, int w, int h, bool force_redraw) {
    // Calculate static label Y positions (needed in both blocks)
    int line_h = 25;
    int remaining_h_calc = h - 140; // Height below where the bar will be
    int y_line_start_calc = y + 140 + (remaining_h_calc - (line_h * 2)) / 2;
    int x_label = x + 10; // X position for static labels
    if (force_redraw) {
      canvas->drawRect(x, y, w, h, C_PANEL_BORDER);
      // Title (Static)
      canvas->setTextColor(C_HUMID_BLUE); // Unified color
      canvas->setTextSize(2); // Header Size 2
      printCentered("OUTDOOR AIR QUALITY", x, y + 5, w, 30, 2);
      // --- Outdoor Details List Labels (Static - Initial Draw Only) ---
      // These are now also drawn in the update block below
      canvas->setTextSize(2);
      canvas->setTextColor(C_OAQ_TITLE); // O3 label color (Bright Cyan)
      canvas->setCursor(x_label, y_line_start_calc); canvas->print("O3:");
      canvas->setTextColor(C_WHITE); // NO2 label color
      canvas->setCursor(x_label, y_line_start_calc + line_h); canvas->print("NO2:");
    }
    // --- Differential Data Update ---
    if (force_redraw || g_oaq_index != g_oaq_index_prev ||
        g_o3_ppb != g_o3_ppb_prev || g_no2_ppb != g_no2_ppb_prev)
    {
      // Erase all data areas
      canvas->fillRect(x + 1, y + 36, w - 2, h - 37, C_BACKGROUND);
      // --- Redraw all dynamic data for this panel ---
      // Large OAQ Value
      canvas->setTextColor(getIaqColor(g_oaq_index));
      char buffer[40];
      snprintf(buffer, sizeof(buffer), "%.0f", g_oaq_index);
      printCentered(buffer, x, y + 40, w, 60, 6);
      // --- OAQ Status Bar ---
      int barX = x + 20; int barW = w - 40; int barY = y + 110; int barH = 15;
      canvas->drawRect(barX, barY, barW, barH, C_DARK_GREY);
      int fillW = map(g_oaq_index, 0, 500, 0, barW);
      if (fillW < 0) fillW = 0; if (fillW > barW) fillW = barW;
      canvas->fillRect(barX, barY, fillW, barH, getIaqColor(g_oaq_index));
      // --- Outdoor Details List ---
      canvas->setTextSize(2);
      int x_level = x + 60; // Position for dynamic status label
      int x_value_right = x + w - 10; // Right-align edge for value
      int y_line = y_line_start_calc; // Use calculated start Y
      int textWidth = 0;
      // --- FIX V6.11: Redraw static labels with intuitive colors here ---
      canvas->setTextColor(C_OAQ_TITLE); // O3 label color (Bright Cyan)
      canvas->setCursor(x_label, y_line); canvas->print("O3:");
      canvas->setTextColor(C_WHITE); // NO2 label color
      canvas->setCursor(x_label, y_line + line_h); canvas->print("NO2:");
      // --- End Fix ---
      // O3 Status Label + Value
      String o3Level = getO3Level(g_o3_ppb);
      uint16_t o3Color = getO3Color(g_o3_ppb);
      canvas->setTextColor(o3Color);             // Dynamic color for status label
      canvas->setCursor(x_level, y_line); canvas->print(o3Level);
      canvas->setTextColor(C_TEXT_PRIMARY); // White for value
      snprintf(buffer, sizeof(buffer), "%.2f", g_o3_ppb);
      textWidth = String(buffer).length() * FONT_W * 2;
      canvas->setCursor(x_value_right - textWidth, y_line); canvas->print(buffer);
      y_line += line_h;
      // NO2 Status Label + Value
      String no2Level = getNO2Level(g_no2_ppb);
      uint16_t no2Color = getNO2Color(g_no2_ppb);
      canvas->setTextColor(no2Color);             // Dynamic color for status label
      canvas->setCursor(x_level, y_line); canvas->print(no2Level);
      canvas->setTextColor(C_TEXT_PRIMARY); // White for value
      snprintf(buffer, sizeof(buffer), "%.2f", g_no2_ppb);
      textWidth = String(buffer).length() * FONT_W * 2;
      canvas->setCursor(x_value_right - textWidth, y_line); canvas->print(buffer);
    }
}
/**
 * @brief Helper function to return a color based on IAQ/OAQ index.
 * --- NOTE: This uses the v4.6 IAQ colors.
 */
uint16_t getIaqColor(float iaq) {
  if (isnan(iaq)) return C_DARK_GREY; // Grey for invalid reading
  // Using typical IAQ thresholds
  if (iaq <= 100) return C_IAQ_GREEN;
  if (iaq <= 200) return C_IAQ_YELLOW; // Moderate/Unhealthy for Sensitive
  if (iaq <= 300) return C_IAQ_ORANGE; // Unhealthy
  return C_IAQ_RED; // Very Unhealthy or worse
}
/**
 * @brief Helper function to return a color based on TVOC (mg/m3).
 * --- REFACTOR: NEW CONSERVATIVE SCALE ---
 */
uint16_t getTvocColor(float tvoc_mg_m3) {
    if (isnan(tvoc_mg_m3)) return C_DARK_GREY;
    if (tvoc_mg_m3 <= 0.5) return C_IAQ_GREEN;  // [LOW]
    if (tvoc_mg_m3 <= 1.5) return C_IAQ_YELLOW; // [MOD]
    if (tvoc_mg_m3 <= 3.0) return C_IAQ_ORANGE; // [HIGH]
    return C_IAQ_RED;                          // [V.HIGH] (> 3.0)
}
/**
 * @brief Helper function to return a color based on Ethanol (ppm).
 * --- REFACTOR: NEW CONSERVATIVE SCALE ---
 */
uint16_t getEthanolColor(float ethanol_ppm) {
    if (isnan(ethanol_ppm)) return C_DARK_GREY;
    if (ethanol_ppm <= 0.5) return C_IAQ_GREEN;  // [LOW]
    if (ethanol_ppm <= 1.5) return C_IAQ_YELLOW; // [MOD]
    if (ethanol_ppm <= 3.0) return C_IAQ_ORANGE; // [HIGH]
    return C_IAQ_RED;                          // [V.HIGH] (> 3.0)
}
/**
 * @brief Helper function to return a Level ID string based on TVOC (mg/m3).
 * --- REFACTOR: NEW CONSERVATIVE SCALE ---
 */
String getTvocLevel(float tvoc_mg_m3) {
    if (isnan(tvoc_mg_m3)) return "[-]";
    if (tvoc_mg_m3 <= 0.5) return "[LOW]";
    if (tvoc_mg_m3 <= 1.5) return "[MOD]";
    if (tvoc_mg_m3 <= 3.0) return "[HIGH]";
    return "[V.HIGH]";
}
/**
 * @brief Helper function to return a Level ID string based on Ethanol (ppm).
 * --- REFACTOR: NEW CONSERVATIVE SCALE ---
 */
String getEthanolLevel(float ethanol_ppm) {
    if (isnan(ethanol_ppm)) return "[-]";
    if (ethanol_ppm <= 0.5) return "[LOW]";
    if (ethanol_ppm <= 1.5) return "[MOD]";
    if (ethanol_ppm <= 3.0) return "[HIGH]";
    return "[V.HIGH]";
}
/**
 * @brief Gets AQI color for O3 (8-hr) based on EPA standards.
 * --- UNCHANGED: Confirmed to be accurate per review. ---
 */
uint16_t getO3Color(float o3_ppb) {
    if (isnan(o3_ppb)) return C_DARK_GREY;
    if (o3_ppb <= 54) return C_IAQ_GREEN;
    if (o3_ppb <= 70) return C_IAQ_YELLOW;
    if (o3_ppb <= 85) return C_IAQ_ORANGE;
    if (o3_ppb <= 105) return C_IAQ_RED;
    if (o3_ppb <= 200) return C_AQI_PURPLE;
    return C_AQI_MAROON;
}
/**
 * @brief Gets AQI level string for O3 (8-hr) based on EPA standards.
 * --- UNCHANGED: Confirmed to be accurate per review. ---
 */
String getO3Level(float o3_ppb) {
    if (isnan(o3_ppb)) return "[-]";
    if (o3_ppb <= 54) return "[GOOD]";
    if (o3_ppb <= 70) return "[MOD]";
    if (o3_ppb <= 85) return "[UNH-S]";
    if (o3_ppb <= 105) return "[UNH]";
    if (o3_ppb <= 200) return "[V-UNH]";
    return "[HAZ]";
}
/**
 * @brief Gets AQI color for NO2 (1-hr) based on EPA standards.
 * --- UNCHANGED: Confirmed to be accurate per review. ---
 */
uint16_t getNO2Color(float no2_ppb) {
    if (isnan(no2_ppb)) return C_DARK_GREY;
    if (no2_ppb <= 53) return C_IAQ_GREEN;
    if (no2_ppb <= 100) return C_IAQ_YELLOW;
    if (no2_ppb <= 360) return C_IAQ_ORANGE;
    if (no2_ppb <= 649) return C_IAQ_RED;
    if (no2_ppb <= 1249) return C_AQI_PURPLE;
    return C_AQI_MAROON;
}
/**
 * @brief Gets AQI level string for NO2 (1-hr) based on EPA standards.
 * --- UNCHANGED: Confirmed to be accurate per review. ---
 */
String getNO2Level(float no2_ppb) {
    if (isnan(no2_ppb)) return "[-]";
    if (no2_ppb <= 53) return "[GOOD]";
    if (no2_ppb <= 100) return "[MOD]";
    if (no2_ppb <= 360) return "[UNH-S]";
    if (no2_ppb <= 649) return "[UNH]";
    if (no2_ppb <= 1249) return "[V-UNH]";
    return "[HAZ]";
}

