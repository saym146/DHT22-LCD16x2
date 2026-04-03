#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <LiquidCrystal.h>

// -------------------------
// WIFI + MQTT SETTINGS
// -------------------------
const char* WIFI_SSID     = "SSID";
const char* WIFI_PASSWORD = "wifiPass";

const char* MQTT_SERVER   = "broker Ip/Domain";
const uint16_t MQTT_PORT     = 1883;
const char* MQTT_USER     = "USER";
const char* MQTT_PASS     = "PASS";

const char* TOPIC_TEMP = "home/room1/temperature";
const char* TOPIC_HUM  = "home/room1/humidity";

// -------------------------
// DHT22 CONFIG
// -------------------------
#define DHTPIN 10
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

// -------------------------
// LCD CONFIG
// -------------------------
LiquidCrystal lcd(D1, D2, D3, D4, D5, D6);

// -------------------------
// GLOBAL VARIABLES
// -------------------------
WiFiClient espClient;
PubSubClient client(espClient);

bool wifiOK = false;
bool mqttOK = false;

// Variables to track previous states
float lastTemp = -999.0;
float lastHum  = -999.0;
const float threshold = 0.10; // Minimum change required to trigger MQTT send

// -------------------------
// CUSTOM CHARACTERS (Tick & Cross)
// -------------------------
byte tickChar[8] = {
  B00000, B00001, B00011, B10110, B11100, B01000, B00000, B00000
};

byte crossChar[8] = {
  B00000, B10001, B01010, B00100, B01010, B10001, B00000, B00000
};

// -------------------------
// HELPER FUNCTIONS
// -------------------------
void setupWiFiSafe() {
  if (WiFi.status() == WL_CONNECTED) return;
  
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  unsigned long startAttempt = millis();
  
  while (WiFi.status() != WL_CONNECTED && millis() - startAttempt < 3000) {
    delay(100);
  }
  wifiOK = (WiFi.status() == WL_CONNECTED);
}

void setupMQTTSafe() {
  if (!wifiOK) {
    mqttOK = false;
    return;
  }
  client.setServer(MQTT_SERVER, MQTT_PORT);
  mqttOK = client.connect("ESP8266_Room_Inaaya", MQTT_USER, MQTT_PASS);
}

// -------------------------
// MAIN SETUP
// -------------------------
void setup() {
  Serial.begin(115200);
  lcd.begin(16, 2);

  lcd.createChar(0, tickChar);
  lcd.createChar(1, crossChar);

  dht.begin();

  setupWiFiSafe();
  setupMQTTSafe();
}

// -------------------------
// MAIN LOOP
// -------------------------
void loop() {
  // 1. Connection Maintenance
  if (WiFi.status() != WL_CONNECTED) {
    wifiOK = false;
    setupWiFiSafe();
  } else {
    wifiOK = true;
  }

  if (wifiOK) {
    if (!client.connected()) {
      setupMQTTSafe();
    } else {
      mqttOK = true;
    }
  } else {
    mqttOK = false;
  }

  client.loop();

  // 2. Read Sensor
  float temp = dht.readTemperature();
  float hum  = dht.readHumidity();

  // 3. Update LCD (Real-time display)
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("T: ");
  if (isnan(temp)) lcd.print("--"); else lcd.print(temp, 1);
  lcd.print((char)223);
  lcd.print("C");

  lcd.setCursor(0, 1);
  lcd.print("H: ");
  if (isnan(hum)) lcd.print("--"); else lcd.print(hum, 1);
  lcd.print("% ");

  // Status Icons
  lcd.setCursor(10, 1);
  lcd.print("W:"); lcd.write(wifiOK ? 0 : 1);
  lcd.setCursor(13, 1);
  lcd.print("M:"); lcd.write(mqttOK ? 0 : 1);

  // 4. Conditional MQTT Publishing
  if (wifiOK && mqttOK && !isnan(temp) && !isnan(hum)) {
    
    // Check if Temperature changed enough
    if (abs(temp - lastTemp) >= threshold) {
      char tStr[10];
      dtostrf(temp, 4, 2, tStr);
      if (client.publish(TOPIC_TEMP, tStr)) {
        lastTemp = temp;
        Serial.print("Temp Published: "); Serial.println(tStr);
      }
    }

    // Check if Humidity changed enough
    if (abs(hum - lastHum) >= threshold) {
      char hStr[10];
      dtostrf(hum, 4, 2, hStr);
      if (client.publish(TOPIC_HUM, hStr)) {
        lastHum = hum;
        Serial.print("Hum Published: "); Serial.println(hStr);
      }
    }
  }

  delay(3000); // 3-second cycle
}