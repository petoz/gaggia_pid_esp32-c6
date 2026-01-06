#include "mqtt.h"
#include "config.h"
#include <PID_v1.h>
#include <Preferences.h>
#include <PubSubClient.h>
#include <WiFi.h>

extern double Setpoint, Input, Output;
extern double Kp, Ki, Kd;
extern float currentTemperature;
extern PID myPID;

WiFiClient espClient;
PubSubClient client(espClient);

String mqtt_server = "";
int mqtt_port = 1883;
String mqtt_user = "";
String mqtt_pass = "";

unsigned long lastMsg = 0;
const long interval = 2000; // Publish every 2s

void callback(char *topic, byte *payload, unsigned int length) {
  String message;
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  Serial.println(message);

  if (String(topic) == "gaggia/set/target") {
    Setpoint = message.toDouble();
    Preferences preferences;
    preferences.begin("gaggia", false);
    preferences.putDouble("target", Setpoint);
    preferences.end();
  } else if (String(topic) == "gaggia/set/kp") {
    Kp = message.toDouble();
    myPID.SetTunings(Kp, Ki, Kd);
    Preferences preferences;
    preferences.begin("gaggia", false);
    preferences.putDouble("kp", Kp);
    preferences.end();
  } else if (String(topic) == "gaggia/set/ki") {
    Ki = message.toDouble();
    myPID.SetTunings(Kp, Ki, Kd);
    Preferences preferences;
    preferences.begin("gaggia", false);
    preferences.putDouble("ki", Ki);
    preferences.end();
  } else if (String(topic) == "gaggia/set/kd") {
    Kd = message.toDouble();
    myPID.SetTunings(Kp, Ki, Kd);
    Preferences preferences;
    preferences.begin("gaggia", false);
    preferences.putDouble("kd", Kd);
    preferences.end();
  }
}

void sendDiscovery() {
  String deviceJson = "\"device\": {"
                      "\"ids\": [\"gaggia_pid_esp32c6\"],"
                      "\"name\": \"Gaggia PID\","
                      "\"mdl\": \"ESP32-C6 PID\","
                      "\"mf\": \"Antigravity\","
                      "\"sw\": \"1.0\""
                      "}";

  // Climate Entity
  String climateConfig =
      "{"
      "\"name\": \"Gaggia PID\","
      "\"unique_id\": \"gaggia_pid_climate\","
      "\"mode_cmd_t\": \"gaggia/set/mode\","
      "\"temp_cmd_t\": \"gaggia/set/target\","
      "\"temp_stat_t\": \"gaggia/status\","
      "\"temp_stat_tpl\": \"{{value_json.target}}\","
      "\"curr_temp_t\": \"gaggia/status\","
      "\"curr_temp_tpl\": \"{{value_json.temp}}\","
      "\"min_temp\": 80,\"max_temp\": 110,\"temp_step\": 0.1,"
      "\"modes\": [\"heat\", \"off\"]," +
      deviceJson + "}";
  client.publish("homeassistant/climate/gaggia_pid/config",
                 climateConfig.c_str(), true);

  // Kp Number
  String kpConfig = "{"
                    "\"name\": \"Gaggia Kp\","
                    "\"unique_id\": \"gaggia_kp\","
                    "\"cmd_t\": \"gaggia/set/kp\","
                    "\"stat_t\": \"gaggia/status\","
                    "\"stat_tpl\": \"{{value_json.kp}}\","
                    "\"min\": 0,\"max\": 200,\"step\": 0.1," +
                    deviceJson + "}";
  client.publish("homeassistant/number/gaggia_kp/config", kpConfig.c_str(),
                 true);

  // Ki Number
  String kiConfig = "{"
                    "\"name\": \"Gaggia Ki\","
                    "\"unique_id\": \"gaggia_ki\","
                    "\"cmd_t\": \"gaggia/set/ki\","
                    "\"stat_t\": \"gaggia/status\","
                    "\"stat_tpl\": \"{{value_json.ki}}\","
                    "\"min\": 0,\"max\": 200,\"step\": 0.01," +
                    deviceJson + "}";
  client.publish("homeassistant/number/gaggia_ki/config", kiConfig.c_str(),
                 true);

  // Kd Number
  String kdConfig = "{"
                    "\"name\": \"Gaggia Kd\","
                    "\"unique_id\": \"gaggia_kd\","
                    "\"cmd_t\": \"gaggia/set/kd\","
                    "\"stat_t\": \"gaggia/status\","
                    "\"stat_tpl\": \"{{value_json.kd}}\","
                    "\"min\": 0,\"max\": 200,\"step\": 0.1," +
                    deviceJson + "}";
  client.publish("homeassistant/number/gaggia_kd/config", kdConfig.c_str(),
                 true);

  // Output Sensor
  String outConfig = "{"
                     "\"name\": \"Gaggia Output\","
                     "\"unique_id\": \"gaggia_output\","
                     "\"stat_t\": \"gaggia/status\","
                     "\"stat_tpl\": \"{{value_json.output}}\","
                     "\"unit_of_meas\": \"%\"," +
                     deviceJson + "}";
  client.publish("homeassistant/sensor/gaggia_output/config", outConfig.c_str(),
                 true);
}

void reconnect() {
  if (mqtt_server == "") {
    // Serial.println("MQTT: No server configured"); // Too spammy for loop?
    return;
  }

  if (!client.connected()) {
    Serial.print("Attempting MQTT connection to ");
    Serial.print(mqtt_server);
    Serial.print(":");
    Serial.println(mqtt_port);

    String clientId = "GaggiaPID-" + String(random(0xffff), HEX);

    bool connected;
    if (mqtt_user.length() > 0) {
      Serial.printf("MQTT: Logging in as user '%s'\n", mqtt_user.c_str());
      connected = client.connect(clientId.c_str(), mqtt_user.c_str(),
                                 mqtt_pass.c_str());
    } else {
      Serial.println("MQTT: Logging in anonymously");
      connected = client.connect(clientId.c_str());
    }

    if (connected) {
      Serial.println("MQTT: Connected!");
      client.subscribe("gaggia/set/#");
      sendDiscovery();
    } else {
      Serial.print("MQTT: Failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
    }
  }
}

void setupMQTT() {
  Preferences preferences;
  preferences.begin("gaggia", true);
  mqtt_server = preferences.getString("mqtt_server", "");
  mqtt_port = preferences.getInt("mqtt_port", 1883);
  mqtt_user = preferences.getString("mqtt_user", "");
  mqtt_pass = preferences.getString("mqtt_pass", "");
  preferences.end();

  Serial.println("--- MQTT Config Loaded ---");
  Serial.printf("Server: %s\n", mqtt_server.c_str());
  Serial.printf("Port: %d\n", mqtt_port);
  Serial.printf("User: %s\n", mqtt_user.c_str());
  Serial.println("--------------------------");

  if (mqtt_server != "") {
    client.setServer(mqtt_server.c_str(), mqtt_port);
    client.setCallback(callback);
    client.setBufferSize(1024); // Increase buffer for HA Discovery
  }
}

void publishStatus() {
  if (mqtt_server == "" || !client.connected())
    return;

  String json = "{";
  json += "\"temp\":" + String(currentTemperature);
  json += ",\"target\":" + String(Setpoint);
  json += ",\"output\":" + String(Output); // Raw output (0-1000)
  // Convert to percentage for HA if needed roughly output/10
  json += ",\"kp\":" + String(Kp);
  json += ",\"ki\":" + String(Ki);
  json += ",\"kd\":" + String(Kd);
  json += "}";

  client.publish("gaggia/status", json.c_str());
}

void handleMQTT() {
  if (mqtt_server == "")
    return;

  if (!client.connected()) {
    static unsigned long lastReconnectAttempt = 0;
    long now = millis();
    if (now - lastReconnectAttempt > 5000) {
      lastReconnectAttempt = now;
      reconnect();
    }
  } else {
    client.loop();

    unsigned long now = millis();
    if (now - lastMsg > interval) {
      lastMsg = now;
      publishStatus();
    }
  }
}
