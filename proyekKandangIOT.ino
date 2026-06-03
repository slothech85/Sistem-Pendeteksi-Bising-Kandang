#define BLYNK_TEMPLATE_ID   "TMPL6hy3Rq51w"
#define BLYNK_TEMPLATE_NAME "Sens0r Bis1ng K4ndang"
#define BLYNK_AUTH_TOKEN    "rLaUh-Hzm_AhdFI0CzoBiok2uzn2tLCW"

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>

#define BLYNK_PRINT Serial

// Inisialisasi BlynkTimer
BlynkTimer timer;

// ===================== KONFIGURASI =====================
char ssid[] = "namaWiFi";
char pass[] = "passwordWiFi";

const int ledHijau  = 5;
const int ledKuning = 4;
const int ledMerah  = 2;
const int pinSuara  = 34;

#define SDA_PIN 21
#define SCL_PIN 22

LiquidCrystal_I2C lcd(0x27, 16, 2);

const int   sampleWindow = 100;
const float ADC_MAX      = 4095.0;
const float ADC_VOLTAGE  = 3.3;

// =======================================================

void tampilLCD(String status) {
  lcd.setCursor(0, 0);
  lcd.print("Status Kandang: ");
  lcd.setCursor(0, 1);
  lcd.print(status);
  lcd.print("                ");
}

// Fungsi khusus pembacaan sensor & pengiriman data ke Blynk (Dijalankan via Timer)
void cekDanKirimSensor() {
  // ========== SAMPLING SUARA ==========
  unsigned long startMillis = millis();
  unsigned int signalMax    = 0;
  unsigned int signalMin    = 4095;
  unsigned long crossings   = 0;
  int lastState             = -1;

  while (millis() - startMillis < sampleWindow) {
    int sample = analogRead(pinSuara);

    if (sample > signalMax) signalMax = sample;
    if (sample < signalMin) signalMin = sample;

    int currentState = (sample > 2000) ? 1 : 0;
    if (lastState == -1) lastState = currentState;
    if (currentState != lastState) {
      crossings++;
      lastState = currentState;
    }
  }

  // ========== HITUNG NILAI ==========
  int   peakToPeak = signalMax - signalMin;
  float amplitudoV = (peakToPeak / ADC_MAX) * ADC_VOLTAGE;
  float frekuensi  = ((float)crossings / 2.0) * (1000.0 / sampleWindow);

  // ========== TENTUKAN STATUS ==========
  String statusKandang = "";

  if (amplitudoV < 0.805) {
    digitalWrite(ledHijau,  HIGH);
    digitalWrite(ledKuning, LOW);
    digitalWrite(ledMerah,  LOW);
    statusKandang = "AMAN";
  }
  else if (amplitudoV < 3.22) {
    digitalWrite(ledHijau,  LOW);
    digitalWrite(ledKuning, HIGH);
    digitalWrite(ledMerah,  LOW);
    statusKandang = "WASPADA";
  }
  else {
    digitalWrite(ledHijau,  LOW);
    digitalWrite(ledKuning, LOW);
    digitalWrite(ledMerah,  HIGH);
    statusKandang = "BAHAYA";
  }

  // ========== TAMPIL DI LCD ==========
  tampilLCD(statusKandang);

  // ========== KIRIM KE BLYNK ==========
  Blynk.virtualWrite(V0, amplitudoV);
  Blynk.virtualWrite(V1, frekuensi);
  Blynk.virtualWrite(V2, statusKandang);

  // ========== DEBUG SERIAL MONITOR ==========
  Serial.printf("ADC Mentah: %d | Amplitudo: %.3f V | Frekuensi: %.1f Hz | Status: %s\n",
                peakToPeak, amplitudoV, frekuensi, statusKandang.c_str());
  Serial.printf("WiFi RSSI    : %d dBm\n",  WiFi.RSSI());
  Serial.printf("Blynk Online : %s\n",      Blynk.connected() ? "YA" : "TIDAK");
}

void setup() {
  Serial.begin(115200);
  analogReadResolution(12);

  // Setup LED
  pinMode(ledHijau,  OUTPUT);
  pinMode(ledKuning, OUTPUT);
  pinMode(ledMerah,  OUTPUT);
  digitalWrite(ledHijau,  LOW);
  digitalWrite(ledKuning, LOW);
  digitalWrite(ledMerah,  LOW);

  // Setup LCD
  Wire.begin(SDA_PIN, SCL_PIN);
  delay(100);
  lcd.begin(16, 2);
  delay(100);
  lcd.backlight();
  delay(100);
  lcd.setCursor(0, 0);
  lcd.print("Sensor Bising");
  lcd.setCursor(0, 1);
  lcd.print("Kandang v1.0");
  delay(2000);
  lcd.clear();

  // Koneksi Blynk
  lcd.setCursor(0, 0);
  lcd.print("Konek WiFi...");
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Sistem Siap!");
  delay(1500);
  lcd.clear();

  // Atur interval pengisian/pembacaan data setiap 1000ms (1 detik)
  timer.setInterval(1000L, cekDanKirimSensor);
}

void loop() {
  Blynk.run();
  timer.run(); // Menjalankan BlynkTimer secara asinkron
}