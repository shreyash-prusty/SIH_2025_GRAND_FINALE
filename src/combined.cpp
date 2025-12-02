#include <Arduino.h>
#include <driver/i2s.h>

// Pin Definitions
#define CO2_PIN 34         // Analog pin for MG811
#define I2S_WS 15          // I2S Word Select
#define I2S_SCK 14         // I2S Serial Clock
#define I2S_SD 32          // I2S Serial Data
#define RADAR_PIN 23       // HLK-LD2410 OUT pin

// Vibration Sensor Parameters
float sensitivity = 0.07;  // Lower = less sensitivity
int noiseGate = 50;        // Ignore anything below this amplitude
float smooth1 = 0.04;      // First smoothing filter
float smooth2 = 0.03;      // Second smoothing filter

// Filter states for vibration
int32_t f1 = 0, f2 = 0;

// Smoothing for CO2
float co2_smooth = 0;

// Presence detection state
int presence_state = 0;
unsigned long last_presence_change = 0;
const unsigned long debounce_delay = 300;

// I2S Configuration for vibration sensor
const i2s_config_t i2s_config = {
  .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
  .sample_rate = 44100,
  .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
  .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
  .communication_format = I2S_COMM_FORMAT_I2S_MSB,
  .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
  .dma_buf_count = 4,
  .dma_buf_len = 256,
  .use_apll = true,
  .tx_desc_auto_clear = false,
  .fixed_mclk = 0
};

const i2s_pin_config_t pin_config = {
  .bck_io_num = I2S_SCK,
  .ws_io_num = I2S_WS,
  .data_out_num = I2S_PIN_NO_CHANGE,
  .data_in_num = I2S_SD
};

void setup() {
  Serial.begin(115200);
  
  // Setup pins
  pinMode(CO2_PIN, INPUT);
  pinMode(RADAR_PIN, INPUT_PULLDOWN);
  
  // Initialize I2S for vibration sensor
  i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
  i2s_set_pin(I2S_NUM_0, &pin_config);
  
  delay(500);
  
  // Print header for Serial Plotter (optional)
  // Serial.println("CO2_Voltage,Vibration,Presence");
}

void loop() {
  unsigned long current_time = millis();
  
  // --- CO2 SENSOR READING ---
  int raw_co2 = analogRead(CO2_PIN);
  float voltage = raw_co2 * (3.3 / 4095.0);
  co2_smooth = (co2_smooth * 0.90) + (voltage * 0.10);
  float co2_value = co2_smooth;
  
  // --- VIBRATION SENSOR READING ---
  int32_t vibration_value = 0;
  int32_t raw_vib = 0;
  size_t readBytes;
  
  i2s_read(I2S_NUM_0, &raw_vib, sizeof(raw_vib), &readBytes, 0);
  
  if (readBytes > 0) {
    // Convert & reduce amplitude
    int32_t sample = raw_vib >> 14;
    sample = sample * sensitivity;
    
    // Apply noise gate
    if (abs(sample) < noiseGate) sample = 0;
    
    // Smooth filter 1
    f1 = f1 + smooth1 * (sample - f1);
    
    // Smooth filter 2 (refinement)
    f2 = f2 + smooth2 * (f1 - f2);
    
    vibration_value = abs(f2);  // Use absolute value for visualization
  }
  
  // --- RADAR PRESENCE DETECTION ---
  int radar_val = digitalRead(RADAR_PIN);
  static int last_radar_state = HIGH;
  
  // Debounce and detect state change
  if (radar_val != last_radar_state) {
    last_presence_change = current_time;
    last_radar_state = radar_val;
  }
  
  // Update presence state after debounce period
  if ((current_time - last_presence_change) > debounce_delay) {
    presence_state = (last_radar_state == LOW) ? 1 : 0;
  }
  
  // Scale presence to a visible range (0-1)
  float presence_value = presence_state * 1.0;
  
  // --- OUTPUT FOR SERIAL PLOTTER ---
  // Format: CO2_Voltage,Vibration,Presence
  // Each will appear as a different colored line
  Serial.print(co2_value);
  Serial.print(",");
  Serial.print(vibration_value);
  Serial.print(",");
  Serial.println(presence_value);
  
  // Adjust delay based on your needs
  delay(30);
}