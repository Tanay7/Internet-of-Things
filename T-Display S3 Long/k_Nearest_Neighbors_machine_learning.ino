

/*
 * @file    T_Display_S3_Long_ML_kNN.ino
 * @author  Tanay C
 * @brief   A visualization of the k-Nearest Neighbors (k-NN) machine learning
 * algorithm on the Lilygo T-Display-S3-Long using synthetic data.
 * @version 1.0
 * @date    2025-10-20
 */
// --- LIBRARIES ---
#include "AXS15231B.h"
#include <Arduino.h>
#include <TFT_eSPI.h>
#include <math.h>
#include <stdlib.h> // For qsort
// --- DISPLAY & UI ---
TFT_eSPI tft = TFT_eSPI(180, 640);
TFT_eSprite sprite = TFT_eSprite(&tft);
// --- ML & DATA DEFINITIONS ---
#define NUM_CLASSES 3
#define POINTS_PER_CLASS 40
#define TOTAL_POINTS (NUM_CLASSES * POINTS_PER_CLASS)
#define K_NEIGHBORS 7 // The 'k' in k-NN
// Data structure for a single classified point
struct DataPoint {
    int16_t x;
    int16_t y;
    int8_t classID; // 0, 1, 2, etc.
};
// Helper structure for sorting neighbors by distance
struct Neighbor {
    int index;
    float distance;
};
DataPoint trainingData[TOTAL_POINTS];
DataPoint unknownPoint;
Neighbor neighbors[TOTAL_POINTS];
int k_nearest_indices[K_NEIGHBORS];
// UI Colors
uint16_t classColors[NUM_CLASSES] = {TFT_RED, TFT_GREEN, TFT_BLUE};
#define UNKNOWN_COLOR TFT_WHITE
#define LINE_COLOR    TFT_YELLOW
// --- FUNCTION PROTOTYPES ---
void generateTrainingData();
void classifyPoint();
int compareNeighbors(const void *a, const void *b);
void drawScene(bool showAnalysis, int classifiedID);
void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("--- k-NN Machine Learning Visualization ---");
    // Initialize Display
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, LOW);
    axs15231_init();
    sprite.createSprite(640, 180);
    sprite.setSwapBytes(1);
    lcd_fill(0, 0, 180, 640, 0x00);
    digitalWrite(TFT_BL, HIGH);
    // Seed the random number generator
    randomSeed(analogRead(0));
    // Create our synthetic dataset
    generateTrainingData();
    Serial.println("Synthetic training data generated.");
}
void loop() {
    // 1. Generate a new "unknown" point at a random location
    unknownPoint.x = random(20, 620);
    unknownPoint.y = random(20, 160);
    unknownPoint.classID = -1; // -1 signifies "unknown"
    // 2. Draw the initial scene with the unknown point
    drawScene(false, -1);
    delay(1500);
    // 3. Run the k-NN algorithm to classify the point
    classifyPoint();
    // 4. Draw the "analysis" view, showing lines to the nearest neighbors
    drawScene(true, -1);
    delay(2500);
    // 5. Draw the final result, coloring the point and showing the text
    drawScene(true, unknownPoint.classID);
    delay(4000);
}
// =========================================================================
// --- MACHINE LEARNING FUNCTIONS ---
// =========================================================================
/**
 * @brief Creates clusters of colored data points to act as our training set.
 */
void generateTrainingData() {
    // Define cluster centers
    int centersX[NUM_CLASSES] = {120, 320, 520};
    int centersY[NUM_CLASSES] = {90, 45, 135};
    int spread = 60; // How spread out the clusters are
    for (int c = 0; c < NUM_CLASSES; c++) {
        for (int i = 0; i < POINTS_PER_CLASS; i++) {
            int index = c * POINTS_PER_CLASS + i;
            trainingData[index].x = centersX[c] + random(-spread, spread);
            trainingData[index].y = centersY[c] + random(-spread, spread);
            trainingData[index].classID = c;
        }
    }
}
/**
 * @brief Implements the k-NN algorithm.
 */
void classifyPoint() {
    // 1. Calculate distance to all training points
    for (int i = 0; i < TOTAL_POINTS; i++) {
        float dx = trainingData[i].x - unknownPoint.x;
        float dy = trainingData[i].y - unknownPoint.y;
        neighbors[i].index = i;
        neighbors[i].distance = sqrt(dx * dx + dy * dy);
    }
    // 2. Sort neighbors by distance (ascending)
    qsort(neighbors, TOTAL_POINTS, sizeof(Neighbor), compareNeighbors);
    // 3. Tally the votes from the top 'k' neighbors
    int votes[NUM_CLASSES] = {0};
    for (int i = 0; i < K_NEIGHBORS; i++) {
        k_nearest_indices[i] = neighbors[i].index; // Store for visualization
        int neighborClass = trainingData[neighbors[i].index].classID;
        votes[neighborClass]++;
    }
    // 4. Find the class with the most votes
    int maxVotes = -1;
    int winningClass = -1;
    for (int i = 0; i < NUM_CLASSES; i++) {
        if (votes[i] > maxVotes) {
            maxVotes = votes[i];
            winningClass = i;
        }
    }
    unknownPoint.classID = winningClass;
}
/**
 * @brief A comparison function for qsort() to sort neighbors by distance.
 */
int compareNeighbors(const void *a, const void *b) {
    Neighbor *nA = (Neighbor *)a;
    Neighbor *nB = (Neighbor *)b;
    if (nA->distance < nB->distance) return -1;
    if (nA->distance > nB->distance) return 1;
    return 0;
}

// =========================================================================
// --- UI DRAWING FUNCTIONS ---
// =========================================================================
/**
 * @brief Draws the entire visualization scene onto the sprite.
 * @param showAnalysis If true, draws lines to the nearest neighbors.
 * @param classifiedID The final class ID, or -1 if not yet classified.
 */
void drawScene(bool showAnalysis, int classifiedID) {
    sprite.fillSprite(TFT_BLACK);
    // Draw all the known training data points
    for (int i = 0; i < TOTAL_POINTS; i++) {
        sprite.fillCircle(trainingData[i].x, trainingData[i].y, 3, classColors[trainingData[i].classID]);
    }
    // Determine the color of the unknown point
    uint16_t unknownColor = UNKNOWN_COLOR;
    if (classifiedID != -1) {
        unknownColor = classColors[classifiedID];
    }
    
    // Draw the unknown point
    sprite.fillRect(unknownPoint.x - 5, unknownPoint.y - 5, 11, 11, unknownColor);
    sprite.drawRect(unknownPoint.x - 5, unknownPoint.y - 5, 11, 11, TFT_DARKGREY);
    // If in the analysis phase, draw lines to neighbors
    if (showAnalysis) {
        for (int i = 0; i < K_NEIGHBORS; i++) {
            int neighborIndex = k_nearest_indices[i];
            sprite.drawLine(unknownPoint.x, unknownPoint.y,
                            trainingData[neighborIndex].x, trainingData[neighborIndex].y, LINE_COLOR);
        }
    }
    // Display the status text
    sprite.setTextDatum(TC_DATUM);
    sprite.setTextSize(2);
    sprite.setTextColor(TFT_WHITE);
    if (classifiedID == -1 && !showAnalysis) {
        sprite.drawString("New point to classify...", 320, 5);
    } else if (classifiedID == -1 && showAnalysis) {
        sprite.drawString("Finding 7 nearest neighbors...", 320, 5);
    } else {
        String className = (classifiedID == 0) ? "RED" : (classifiedID == 1) ? "GREEN" : "BLUE";
        sprite.setTextColor(classColors[classifiedID]);
        sprite.drawString("Classified as: " + className, 320, 5);
    }
    // Push the final image to the display
    lcd_PushColors_rotated_90(0, 0, 640, 180, (uint16_t*)sprite.getPointer());
}
