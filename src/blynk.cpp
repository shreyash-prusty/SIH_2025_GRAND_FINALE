#define BLYNK_TEMPLATE_ID "TMPL3Gvdu2oj4"
#define BLYNK_TEMPLATE_NAME "human presence"
#define BLYNK_AUTH_TOKEN "o6cBMIzBTKM_t1O3gMw51vC9lU0erBQZX"

#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
#include <driver/i2s.h>

const char* ssid = "Omm's A35";
const char* pass = "Megamind";

#define CO2_PIN    34   
#define RADAR_PIN  23   

#define I2S_WS   15
#define I2S_SCK  14
#define I2S_SD   32

float sensitivity = 0.07;
int noiseGate = 50;
float smooth1 = 0.04;
float smooth2 = 0.03;
int32_t f1 = 0, f2 = 0;

unsigned long lastDetectTime = 0;
bool presenceFlag = false;

void setup() {
  Serial.begin(115200);

  pinMode(RADAR_PIN, INPUT_PULLDOWN);

  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

  const i2s_config_t i2s_config = {
    mode: (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    sample_rate: 44100,
    bits_per_sample: I2S_BITS_PER_SAMPLE_32BIT,
    channel_format: I2S_CHANNEL_FMT_ONLY_LEFT,
    communication_format: I2S_COMM_FORMAT_I2S_MSB,
    dma_buf_count: 4,
    dma_buf_len: 256,
    use_apll: true
  };

  const i2s_pin_config_t pin_config = {
    bck_io_num: I2S_SCK,
    ws_io_num: I2S_WS,
    data_out_num: I2S_PIN_NO_CHANGE,
    data_in_num: I2S_SD
  };

  i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
  i2s_set_pin(I2S_NUM_0, &pin_config);

  Serial.println("System Ready...");
}

void loop() {
  Blynk.run();

  readCO2();
  readVibration();
  readPresence();
}

void readCO2() {
  int raw = analogRead(CO2_PIN);
  float voltage = raw * (3.3 / 4095.0);

  static float co2Smooth = 0;
  co2Smooth = (co2Smooth * 0.90) + (voltage * 0.10);

  Blynk.virtualWrite(V0, co2Smooth);

  Serial.print("CO2 Voltage: ");
  Serial.println(co2Smooth);
}

void readVibration() {
  int32_t raw = 0;
  size_t bytesRead;

  i2s_read(I2S_NUM_0, &raw, sizeof(raw), &bytesRead, portMAX_DELAY);

  if (bytesRead > 0) {
    int32_t sample = raw >> 14;
    sample = sample * sensitivity;

    if (abs(sample) < noiseGate) sample = 0;

    f1 = f1 + smooth1 * (sample - f1);
    f2 = f2 + smooth2 * (f1 - f2);

    Blynk.virtualWrite(V1, abs(f2));

    Serial.print("Vibration: ");
    Serial.println(abs(f2));
  }
}

void readPresence() {
  int val = digitalRead(RADAR_PIN);

  if (val == LOW) {  
    presenceFlag = true;
    lastDetectTime = millis();
    Blynk.virtualWrite(V2, 1);

    Serial.println("Human Presence Detected!");
  }

  if (presenceFlag && millis() - lastDetectTime > 15000) {
    presenceFlag = false;
    Blynk.virtualWrite(V2, 0);

    Serial.println("Presence Reset");
  }
}
