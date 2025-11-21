#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <LiquidCrystal.h>

// -------------------------
// WIFI + MQTT SETTINGS
// -------------------------
const char* WIFI_SSID     = "SSID";
const char* WIFI_PASSWORD = "******";

const char* MQTT_SERVER   = "IP/domain";
const uint16_t MQTT_PORT     = PORT;
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
WiFiClient espClient;
PubSubClient client(espClient);

// status variables
bool wifiOK = false;
bool mqttOK = false;

// timing variables for non-blocking delay
unsigned long lastSensorRead = 0;
const unsigned long SENSOR_INTERVAL = 3000; // 3 seconds between readings

// MQTT buffer size for temperature/humidity strings
const int MQTT_VALUE_BUFFER_SIZE = 10; // Sufficient for format: "-XXX.XX\0"

// -------------------------
// Tick & Cross CUSTOM CHARS
// -------------------------
byte tickChar[8] = {
  B00000,
  B00001,
  B00011,
  B10110,
  B11100,
  B01000,
  B00000,
  B00000
};

byte crossChar[8] = {
  B00000,
  B10001,
  B01010,
  B00100,
  B01010,
  B10001,
  B00000,
  B00000
};

// -------------------------
void setupWiFiSafe() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  unsigned long startAttempt = millis();
  wifiOK = false;

  // Try 3 seconds max (non-blocking)
  while (WiFi.status() != WL_CONNECTED && millis() - startAttempt < 3000) {
    delay(200);
  }

  wifiOK = (WiFi.status() == WL_CONNECTED);
}

// -------------------------
void setupMQTTSafe() {
  if (!wifiOK) {
    mqttOK = false;
    return;
  }

  client.setServer(MQTT_SERVER, MQTT_PORT);

  // Try once (non-blocking, no loop)
  mqttOK = client.connect("nodeMCU_room1", MQTT_USER, MQTT_PASS);
}

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
void loop() {

  // reconnect attempts (SAFE, no blocking)
  if (WiFi.status() == WL_CONNECTED) {
    wifiOK = true;
  } else {
    wifiOK = false;
    setupWiFiSafe();
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

  // Non-blocking delay: only read sensor every SENSOR_INTERVAL milliseconds
  // Note: millis() overflow (every ~49 days) is handled correctly by unsigned arithmetic
  unsigned long currentMillis = millis();
  if (currentMillis - lastSensorRead < SENSOR_INTERVAL) {
    return; // Skip sensor reading and LCD update this iteration
  }
  lastSensorRead = currentMillis;

  float temp = dht.readTemperature();
  float hum  = dht.readHumidity();

  // lcd.clear(); // Removed to prevent flickering and reduce LCD wear

  // -------------------------
  // Line 1 - Temperature
  // -------------------------
  lcd.setCursor(0, 0);
  lcd.print("T: ");
  char tempStr[7];
  dtostrf(temp, 5, 2, tempStr); // 5 chars wide, 2 decimals
  lcd.print(tempStr);
  lcd.print((char)223);
  lcd.print("C ");

  // -------------------------
  // Line 2 - Humidity + status
  // -------------------------
  lcd.setCursor(0, 1);
  lcd.print("H: ");
  char humStr[7];
  dtostrf(hum, 5, 2, humStr); // 5 chars wide, 2 decimals
  lcd.print(humStr);
  lcd.print("% ");

  // WiFi Status
  lcd.setCursor(10, 1);
  lcd.print("W:");
  lcd.write(wifiOK ? 0 : 1);

  // MQTT Status
  lcd.setCursor(13, 1);
  lcd.print("M:");
  lcd.write(mqttOK ? 0 : 1);

  // -------------------------
  // MQTT publishing IF AVAILABLE ONLY
  // -------------------------
  if (wifiOK && mqttOK) {
    char tStr[MQTT_VALUE_BUFFER_SIZE];
    char hStr[MQTT_VALUE_BUFFER_SIZE];
    dtostrf(temp, 5, 2, tStr);
    dtostrf(hum, 5, 2, hStr);

    client.publish(TOPIC_TEMP, tStr);
    client.publish(TOPIC_HUM, hStr);
  }
}
