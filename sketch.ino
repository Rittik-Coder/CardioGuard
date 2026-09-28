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

// Pin Definitions (GPIO 12 changed to 26 to avoid strapping pin boot fails)
#define PIN_LED_GREEN   13
#define PIN_LED_YELLOW  26  
#define PIN_LED_RED     14
#define PIN_BUZZER      25
#define PIN_BTN_SOS      5
#define PIN_BTN_MUTE     4

// Wi-Fi & Telegram Configuration
const char* WIFI_SSID = "Wokwi-GUEST";
const char* WIFI_PASS = "";

#define BOT_TOKEN "8936704210:AAG8Y0-MeoYcDykJ8d352iCa770_3SOR5m8"
#define CHAT_ID   "7269897879"

WiFiClientSecure secured_client;
UniversalTelegramBot bot(BOT_TOKEN, secured_client);

// Vitals
int heartRate = 75;
int spo2 = 98;
float bodyTemp = 36.8;

// System States: 0 = NORMAL, 1 = WARNING, 2 = EMERGENCY
int systemState = 0;
bool buzzerMuted = false;
bool manualSOSActive = false;
unsigned long lastUpdate = 0;

// Telegram Alert Throttling
bool alertAlreadySent = false;
unsigned long lastTelegramSent = 0;
const unsigned long telegramCooldown = 30000;

void sendTelegramAlert(const String& reason);
void simulateSensorReadings();
void handleNormalState();
void handleWarningState();
void handleEmergencyState(String reason);
void updateOLED();

void sendTelegramAlert(const String& reason) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[Telegram] Wi-Fi not connected.");
    return;
  }

  String message = "🚨 *EMERGENCY SOS ALERT* 🚨\n\n";
  message += "*Trigger:* " + reason + "\n";
  message += "*Heart Rate:* " + String(heartRate) + " BPM\n";
  message += "*SpO2:* " + String(spo2) + " %\n";
  message += "*Body Temp:* " + String(bodyTemp, 1) + " °C\n\n";
  message += "📍 *Location:* Lat 28.6139, Lon 77.2090\n";
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

  pinMode(PIN_BTN_SOS, INPUT_PULLUP);
  pinMode(PIN_BTN_MUTE, INPUT_PULLUP);

  Wire.begin(21, 22);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("[Hardware] OLED init failed! Check connections."));
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
  Serial.println();

  display.setCursor(10, 40);
  if (WiFi.status() == WL_CONNECTED) {
    display.println("WiFi: Connected");
    Serial.println("[System] WiFi Connected successfully.");
  } else {
    display.println("WiFi: Offline");
    Serial.println("[System] WiFi connection timed out.");
  }
  display.display();
  delay(1200);
}

void loop() {
  // Mute Button Check
  if (digitalRead(PIN_BTN_MUTE) == LOW) {
    buzzerMuted = !buzzerMuted;
    Serial.printf("[Button] Mute toggled: %s\n", buzzerMuted ? "MUTED" : "UNMUTED");
    if (buzzerMuted) {
      noTone(PIN_BUZZER);
    } else if (systemState == 2) {
      tone(PIN_BUZZER, 1000);
    }
    updateOLED();
    delay(350);
  }

  // SOS Toggle Button Check
  if (digitalRead(PIN_BTN_SOS) == LOW) {
    manualSOSActive = !manualSOSActive;
    if (manualSOSActive) {
      systemState = 2;
      Serial.println("[Button] Manual SOS Activated!");
      handleEmergencyState("Manual Green SOS Button Pressed");
    } else {
      systemState = 0;
      buzzerMuted = false;
      alertAlreadySent = false;
      Serial.println("[Button] Manual SOS Cleared.");
      handleNormalState();
      updateOLED();
    }
    delay(350);
  }

  // Periodic sensor cycle
  if (millis() - lastUpdate > 2000) {
    lastUpdate = millis();

    if (!manualSOSActive) {
      simulateSensorReadings();

      if (heartRate > 120 || heartRate < 50 || spo2 < 90 || bodyTemp > 38.5) {
        systemState = 2;
        handleEmergencyState("Critical Vital Sign Deviation");
      } else if ((heartRate >= 101 && heartRate <= 120) || (spo2 >= 90 && spo2 <= 94) || (bodyTemp >= 37.6 && bodyTemp <= 38.5)) {
        systemState = 1;
        buzzerMuted = false;
        alertAlreadySent = false;
        handleWarningState();
        updateOLED();
      } else {
        systemState = 0;
        buzzerMuted = false;
        alertAlreadySent = false;
        handleNormalState();
        updateOLED();
      }
    } else {
      handleEmergencyState("Manual Green SOS Button Pressed");
    }
  }
}

void simulateSensorReadings() {
  int scenario = random(0, 10);
  if (scenario < 6) {
    heartRate = random(68, 88);
    spo2 = random(96, 100);
    bodyTemp = 36.5 + (random(0, 8) / 10.0);
  } else if (scenario < 8) {
    heartRate = random(102, 118);
    spo2 = random(91, 94);
    bodyTemp = 37.6 + (random(0, 6) / 10.0);
  } else {
    heartRate = random(130, 165);
    spo2 = random(82, 89);
    bodyTemp = 38.8;
  }
}

void handleNormalState() {
  digitalWrite(PIN_LED_GREEN, HIGH);
  digitalWrite(PIN_LED_YELLOW, LOW);
  digitalWrite(PIN_LED_RED, LOW);
  noTone(PIN_BUZZER);
  Serial.printf("[STATE: NORMAL] BPM: %d | SpO2: %d%% | Temp: %.1f C\n", heartRate, spo2, bodyTemp);
}

void handleWarningState() {
  digitalWrite(PIN_LED_GREEN, LOW);
  digitalWrite(PIN_LED_YELLOW, HIGH);
  digitalWrite(PIN_LED_RED, LOW);
  noTone(PIN_BUZZER);
  Serial.printf("[STATE: WARNING] BPM: %d | SpO2: %d%% | Temp: %.1f C\n", heartRate, spo2, bodyTemp);
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

  Serial.printf("[STATE: EMERGENCY] Reason: %s | BPM: %d | SpO2: %d%% | Temp: %.1f C\n Calling Ambulance\n", 
                reason.c_str(), heartRate, spo2, bodyTemp);

  updateOLED();

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