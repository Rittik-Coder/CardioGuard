#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <UniversalTelegramBot.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Pin Definitions
#define PIN_LED_GREEN   13  // Green LED anode
#define PIN_LED_YELLOW  12  // Yellow LED anode
#define PIN_LED_RED     14  // Red LED anode
#define PIN_BUZZER      25  // Buzzer positive terminal
#define PIN_BTN_SOS      5  // Green push button (Manual SOS)
#define PIN_BTN_MUTE     4  // Blue push button (Buzzer Mute)

// Wi-Fi & Telegram Configuration
const char* WIFI_SSID = "Wokwi-GUEST";
const char* WIFI_PASS = "";

#define BOT_TOKEN "8936704210:AAG8Y0-MeoYcDykJ8d352iCa770_3SOR5m8"
#define CHAT_ID   "7269897879"

WiFiClientSecure secured_client;
UniversalTelegramBot bot(BOT_TOKEN, secured_client);

// Simulated Vital Parameters
int heartRate = 75;       // BPM
int spo2 = 98;            // %
float bodyTemp = 36.8;    // °C

// Dynamic GPS Coordinates
float currentLat = 28.6139;
float currentLon = 77.2090;

// System States: 0 = NORMAL, 1 = WARNING, 2 = EMERGENCY
int systemState = 0;
bool buzzerMuted = false;
bool manualSOSActive = false;
unsigned long lastUpdate = 0;

// Telegram Alert Throttling
bool alertAlreadySent = false;
unsigned long lastTelegramSent = 0;
const unsigned long telegramCooldown = 30000;

// Function Declarations
void sendTelegramAlert(const String& reason);
void simulateSensorReadings();
void handleNormalState();
void handleWarningState();
void handleEmergencyState(String reason);
void updateOLED();

void sendTelegramAlert(const String& reason) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[Telegram] Wi-Fi not connected. Cannot send alert.");
    return;
  }

  String message = "🚨 *EMERGENCY SOS ALERT* 🚨\n\n";
  message += "*Trigger:* " + reason + "\n";
  message += "*Heart Rate:* " + String(heartRate) + " BPM\n";
  message += "*SpO2:* " + String(spo2) + " %\n";
  message += "*Body Temp:* " + String(bodyTemp, 1) + " °C\n\n";
  message += "📍 *Location:* Lat " + String(currentLat, 6) + ", Lon " + String(currentLon, 6) + "\n";
  message += "🚑 *Action:* Dispatching EMS & Alerting Family.";

  Serial.println("[Telegram] Sending emergency message...");
  if (bot.sendMessage(CHAT_ID, message, "Markdown")) {
    Serial.println("[Telegram] Message delivered successfully.");
  } else {
    Serial.println("[Telegram] Message delivery failed.");
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(PIN_LED_GREEN, OUTPUT);
  pinMode(PIN_LED_YELLOW, OUTPUT);
  pinMode(PIN_LED_RED, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);

  // Push buttons using internal pull-up resistors (Active LOW)
  pinMode(PIN_BTN_SOS, INPUT_PULLUP);
  pinMode(PIN_BTN_MUTE, INPUT_PULLUP);

  Wire.begin(21, 22);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("OLED init failed!"));
    for (;;);
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(15, 10);
  display.println("CardioGuard v1.0");
  display.setCursor(10, 25);
  display.println("Connecting WiFi...");
  display.display();

  WiFi.begin(WIFI_SSID, WIFI_PASS);
  secured_client.setInsecure();

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  display.setCursor(10, 40);
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected.");
    display.println("WiFi: Connected");
  } else {
    Serial.println("\nWiFi connection failed.");
    display.println("WiFi: Offline");
  }
  display.display();
  delay(1500);
}

void loop() {
  // Read Blue Button (Mute/Unmute Buzzer)
  if (digitalRead(PIN_BTN_MUTE) == LOW) {
    buzzerMuted = !buzzerMuted;
    
    if (buzzerMuted) {
      noTone(PIN_BUZZER);
      Serial.println("[Audio] Buzzer MUTED.");
    } else {
      Serial.println("[Audio] Buzzer UNMUTED.");
      if (systemState == 2) {
        tone(PIN_BUZZER, 1000);
      }
    }
    
    updateOLED();
    delay(300); // Debounce
  }

  // Read Green Button (Manual SOS Toggle)
  if (digitalRead(PIN_BTN_SOS) == LOW) {
    manualSOSActive = !manualSOSActive;
    if (manualSOSActive) {
      systemState = 2;
      handleEmergencyState("Manual Green SOS Button Pressed");
    } else {
      systemState = 0;
      buzzerMuted = false;
      alertAlreadySent = false;
      handleNormalState();
    }
    updateOLED();
    delay(300); // Debounce
  }

  // Periodic sensor simulation & evaluation every 2.5 seconds
  if (millis() - lastUpdate > 2500) {
    lastUpdate = millis();

    simulateSensorReadings();

    // Check emergency thresholds
    if (manualSOSActive || heartRate > 120 || heartRate < 50 || spo2 < 90 || bodyTemp > 38.5) {
      systemState = 2;
      String reason = manualSOSActive ? "Manual Button SOS" : "Critical Vital Sign Deviation";
      handleEmergencyState(reason);
    } 
    // Check warning thresholds
    else if ((heartRate >= 101 && heartRate <= 120) || (spo2 >= 90 && spo2 <= 94) || (bodyTemp >= 37.6 && bodyTemp <= 38.5)) {
      systemState = 1;
      alertAlreadySent = false; 
      handleWarningState();
    } 
    // Normal state
    else {
      systemState = 0;
      alertAlreadySent = false;
      handleNormalState();
    }

    updateOLED();
  }
}

void simulateSensorReadings() {
  int scenario = random(0, 10);
  
  // 60% chance: Normal vitals (Green LED)
  if (scenario < 6) {
    heartRate = random(65, 85);
    spo2 = random(96, 100);
    bodyTemp = 36.5 + (random(0, 7) / 10.0);
  } 
  // 20% chance: Warning vitals (Yellow LED)
  else if (scenario < 8) {
    heartRate = random(102, 118);
    spo2 = random(91, 94);
    bodyTemp = 37.6 + (random(0, 6) / 10.0);
  } 
  // 20% chance: Emergency vitals (Red LED)
  else {
    heartRate = random(130, 160);
    spo2 = random(82, 88);
    bodyTemp = 38.8;
  }

  // Dynamically update GPS coordinates
  currentLat += (random(-50, 51) / 100000.0);
  currentLon += (random(-50, 51) / 100000.0);
}

void handleNormalState() {
  digitalWrite(PIN_LED_GREEN, HIGH);
  digitalWrite(PIN_LED_YELLOW, LOW);
  digitalWrite(PIN_LED_RED, LOW);
  noTone(PIN_BUZZER);

  Serial.println("--------------- NORMAL MONITORING ---------------");
  Serial.printf("Heart Rate: %d BPM | SpO2: %d%% | Temp: %.1f C\n", heartRate, spo2, bodyTemp);
  Serial.printf("Simulated GPS: Lat %.6f, Lon %.6f\n", currentLat, currentLon);
  Serial.println("-------------------------------------------------");
}

void handleWarningState() {
  digitalWrite(PIN_LED_GREEN, LOW);
  digitalWrite(PIN_LED_YELLOW, HIGH);
  digitalWrite(PIN_LED_RED, LOW);
  noTone(PIN_BUZZER);

  Serial.println("--------------- CAUTION WARNING ---------------");
  Serial.println("[WARNING] Borderline vitals detected - Monitor closely.");
  Serial.printf("Heart Rate: %d BPM | SpO2: %d%% | Temp: %.1f C\n", heartRate, spo2, bodyTemp);
  Serial.printf("Simulated GPS: Lat %.6f, Lon %.6f\n", currentLat, currentLon);
  Serial.println("-----------------------------------------------");
}

void handleEmergencyState(String reason) {
  digitalWrite(PIN_LED_GREEN, LOW);
  digitalWrite(PIN_LED_YELLOW, LOW);
  digitalWrite(PIN_LED_RED, HIGH);

  if (!buzzerMuted) {
    tone(PIN_BUZZER, 1000);
  } else {
    noTone(PIN_BUZZER);
  }

  Serial.println("================ EMERGENCY ALERT ================");
  Serial.println("[CRITICAL] Severe Cardiac Abnormality Detected!");
  Serial.printf("Heart Rate: %d BPM | SpO2: %d%% | Temp: %.1f C\n", heartRate, spo2, bodyTemp);
  Serial.printf("Simulated GPS: Lat %.6f, Lon %.6f (Transmitted to EMS)\n", currentLat, currentLon);
  Serial.println("=================================================");

  if (!alertAlreadySent || (millis() - lastTelegramSent > telegramCooldown)) {
    sendTelegramAlert(reason);
    alertAlreadySent = true;
    lastTelegramSent = millis();
  }
}

void updateOLED() {
  display.clearDisplay();
  display.setCursor(0, 0);

  if (systemState == 2) {
    display.setTextSize(1);
    display.println("! CARDIAC ALERT !");
    display.drawLine(0, 10, 127, 10, SSD1306_WHITE);
    display.setCursor(0, 16);
    display.printf("BPM: %d (CRIT)\n", heartRate);
    display.printf("SpO2: %d%% (LOW)\n", spo2);
    display.printf("Temp: %.1f C\n", bodyTemp);
    display.setCursor(0, 52);
    if (buzzerMuted) {
      display.println("ALERT (MUTED)");
    } else {
      display.println("CALLING AMBULANCE...");
    }
  } else if (systemState == 1) {
    display.setTextSize(1);
    display.println("CardioGuard: WARNING");
    display.drawLine(0, 10, 127, 10, SSD1306_WHITE);
    display.setCursor(0, 16);
    display.printf("Pulse: %d BPM (ELEV)\n", heartRate);
    display.printf("SpO2 : %d %% (FAIR)\n", spo2);
    display.printf("Temp : %.1f C\n", bodyTemp);
    display.setCursor(0, 52);
    display.println("Status: Caution");
  } else {
    display.setTextSize(1);
    display.println("CardioGuard: NORMAL");
    display.drawLine(0, 10, 127, 10, SSD1306_WHITE);
    display.setCursor(0, 18);
    display.printf("Pulse: %d BPM\n", heartRate);
    display.printf("SpO2 : %d %%\n", spo2);
    display.printf("Temp : %.1f C\n", bodyTemp);
    display.setCursor(0, 52);
    display.println("Status: Monitoring");
  }

  display.display();
}