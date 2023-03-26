#include <ArduinoJson.h>
#include <ArduinoOTA.h>
#include <EmonLib.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include <driver/adc.h>

const uint energyMeterPin = 34;
const double vRms = 235.0;
EnergyMonitor emon;

const char* ssid = "Home_WiFi";
const char* password = "<removed>";

IPAddress mqttServer(192, 168, 178, 23);
const char* mqttUsername = "marchinho93";
const char* mqttPassword = "<removed>";

const char* deviceName = "energy_meter";
const char* currentSensorUniqueID = "energy_meter_current_sensor";
const char* powerSensorUniqueID = "energy_meter_power_sensor";

const char* deviceCurrentSensorConfigTopic =
    "homeassistant/sensor/energy_meter_current_sensor/config";
const char* deviceCurrentSensorStateTopic =
    "homeassistant/sensor/energy_meter_current_sensor/state";
const char* devicePowerSensorConfigTopic =
    "homeassistant/sensor/energy_meter_power_sensor/config";
const char* devicePowerSensorStateTopic =
    "homeassistant/sensor/energy_meter_power_sensor/state";

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

void setup() {
  Serial.begin(115200);

  analogReadResolution(ADC_BITS);

  pinMode(energyMeterPin, INPUT);

  emon.current(energyMeterPin, 25.599);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  // Initialize OTA
  ArduinoOTA.begin();
  ArduinoOTA.setHostname("marcotronics-energy-meter-ota");
  ArduinoOTA.setPassword("<removed>");

  // Set up OTA event handlers
  ArduinoOTA.onStart(onOTAStart);
  ArduinoOTA.onProgress(onOTAProgress);
  ArduinoOTA.onEnd(onOTAEnd);

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  mqttClient.setServer(mqttServer, 1883);
  mqttClient.setBufferSize(1024);
}

long lastUpdateMillis = 0;
const long updatePeriod = 2000;

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    ESP.restart();
  }

  ArduinoOTA.handle();

  if (!mqttClient.connected()) {
    onDisconnected();
    return;
  }

  long timeNow = millis();
  if (timeNow - lastUpdateMillis >= updatePeriod) {
    publishStateUpdates();
    lastUpdateMillis = timeNow;
  }

  delay(50);
}

void onOTAStart() {
  Serial.println("OTA update starting...");
}

void onOTAProgress(unsigned int progress, unsigned int total) {
  Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
}

void onOTAEnd() {
  Serial.println("\nOTA update complete!");
  ESP.restart();
}

void onDisconnected() {
  // Try to reconnect forever
  Serial.print("Attempting MQTT connection...");

  bool res = mqttClient.connect(deviceName, mqttUsername, mqttPassword);
  if (!res) {
    Serial.print("failed, rc=");
    Serial.println(mqttClient.state());
    return;
  }

  Serial.println("Connected!");

  // Publish discovery messages
  publishDiscoveryConfigs();

  // Publish states
  publishStateUpdates();
}

void publishDiscoveryConfigs() {
  publishCurrentSensorDiscoveryConfig();
  publishPowerSensorDiscoveryConfig();
}

void publishStateUpdates() {
  publishCurrentSensorState();
  publishPowerSensorState();
}

void publishCurrentSensorDiscoveryConfig() {
  DynamicJsonDocument json(1024);

  json["name"] = currentSensorUniqueID;
  json["unique_id"] = currentSensorUniqueID;
  json["device"]["name"] = deviceName;
  json["device"]["identifiers"] = deviceName;
  json["unit_of_measurement"] = "A";
  json["device_class"] = "current";
  json["value_template"] = "{{ value_json.current | round(2) }}";
  json["state_topic"] = deviceCurrentSensorStateTopic;
  json["icon"] = "mdi:current-ac";

  char payload[1024];
  serializeJson(json, payload);

  bool res = mqttClient.publish(deviceCurrentSensorConfigTopic, payload);
  if (!res) {
    Serial.println("Error occurred publishing current sensor discovery config");
  }
}

void publishPowerSensorDiscoveryConfig() {
  DynamicJsonDocument json(1024);

  json["name"] = powerSensorUniqueID;
  json["unique_id"] = powerSensorUniqueID;
  json["device"]["name"] = deviceName;
  json["device"]["identifiers"] = deviceName;
  json["unit_of_measurement"] = "W";
  json["device_class"] = "power";
  json["value_template"] = "{{ value_json.apparent_power | round(2) }}";
  json["state_topic"] = devicePowerSensorStateTopic;
  json["icon"] = "mdi:home-lightning-bolt";

  char payload[1024];
  serializeJson(json, payload);

  bool res = mqttClient.publish(devicePowerSensorConfigTopic, payload);
  if (!res) {
    Serial.println("Error occurred publishing power sensor discovery config");
  }
}

void publishCurrentSensorState() {
  DynamicJsonDocument json(1024);

  json["current"] = emon.calcIrms(1480);

  char payload[1024];
  serializeJson(json, payload);

  bool res = mqttClient.publish(deviceCurrentSensorStateTopic, payload);
  if (!res) {
    Serial.println("Error occurred publishing current sensor state");
  }
}

void publishPowerSensorState() {
  DynamicJsonDocument json(1024);

  json["apparent_power"] = readApparentPower();

  char payload[1024];
  serializeJson(json, payload);

  bool res = mqttClient.publish(devicePowerSensorStateTopic, payload);
  if (!res) {
    Serial.println("Error occurred publishing power sensor state");
  }
}

double readApparentPower() {
  double iRms = emon.calcIrms(1480);
  return iRms * vRms;
}
