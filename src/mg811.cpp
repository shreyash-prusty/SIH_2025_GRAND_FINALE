#include <Arduino.h>

#define CO2_PIN 34   // Analog pin for MG811

void setup() {
  Serial.begin(115200);
  delay(500);
  
  // For Serial Plotter, we just need to send the number
  // No text headers or labels
}

void loop() {
  // Raw ADC read
  int raw = analogRead(CO2_PIN);
  
  // Convert to voltage (ESP32 ADC = 0–4095)
  float voltage = raw * (3.3 / 4095.0);
  
  // Simple smoothing for clean graph
  static float smooth = 0;
  smooth = (smooth * 0.90) + (voltage * 0.10);
  
  // For SINGLE LINE graph in Serial Plotter:
  // Send ONLY the number (no labels, no spaces, no extra text)
  Serial.println(smooth);
  
  delay(30);   // Good for Serial Plotter
}