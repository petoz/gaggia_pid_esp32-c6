#include "config.h"
#include <Arduino.h>
#include <PID_v1.h>
#include <WebServer.h>
#include <WiFi.h>
#include <WiFiManager.h>

#include <Preferences.h>
#include <Update.h>

extern float currentTemperature;
extern double Setpoint, Input, Output;
extern double Kp, Ki, Kd;
extern PID myPID;

WebServer server(80);

// ... (index_html skip) ...

// ... (setupWeb start skip) ...

const char *index_html = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Gaggia PID</title>
  <style>
    body { font-family: Arial; text-align: center; margin:0px auto; padding-top: 30px; }
    .card { background-color: white; box-shadow: 2px 2px 12px 1px rgba(140,140,140,.5); padding-top:10px; padding-bottom:20px; }
    .button { background-color: #008CBA; border: none; color: white; padding: 15px 32px; text-align: center; text-decoration: none; display: inline-block; font-size: 16px; }
  </style>
</head>
<body>
  <h2>Gaggia PID Control</h2>
  <div class="card">
    <h3>Temperature: <span id="temp">--</span> &deg;C</h3>
    <h3>Target: <span id="target">--</span> &deg;C</h3>
    <h3>Output: <span id="output">--</span></h3>
  </div>
  <div class="card">
    <form action="/update" method="GET">
      <h3>PID Settings</h3>
      Target: <input type="number" step="0.1" name="target" id="input_target" value=""><br>
      Kp: <input type="number" step="0.1" name="kp" id="input_kp" value=""><br>
      Ki: <input type="number" step="0.1" name="ki" id="input_ki" value=""><br>
      Kd: <input type="number" step="0.1" name="kd" id="input_kd" value=""><br>
      
      <h3>MQTT Settings</h3>
      Server: <input type="text" name="mqtt_server" id="input_mqtt_server" placeholder="192.168.1.100"><br>
      Port: <input type="number" name="mqtt_port" id="input_mqtt_port" value="1883"><br>
      User: <input type="text" name="mqtt_user" id="input_mqtt_user"><br>
      Pass: <input type="password" name="mqtt_pass" id="input_mqtt_pass"><br>
      
      <br>
      <input type="submit" value="Update & Restart" class="button">
    </form>
  </div>
<script>
setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      var json = JSON.parse(this.responseText);
      document.getElementById("temp").innerHTML = json.temp.toFixed(2);
      document.getElementById("target").innerHTML = json.target.toFixed(2);
      document.getElementById("output").innerHTML = json.output.toFixed(0);
      
      // Update inputs only if not focused
      // PID
      if (document.activeElement.id !== "input_target" && document.getElementById("input_target").value == "") document.getElementById("input_target").value = json.target;
      if (document.activeElement.id !== "input_kp" && document.getElementById("input_kp").value == "") document.getElementById("input_kp").value = json.kp;
      if (document.activeElement.id !== "input_ki" && document.getElementById("input_ki").value == "") document.getElementById("input_ki").value = json.ki;
      if (document.activeElement.id !== "input_kd" && document.getElementById("input_kd").value == "") document.getElementById("input_kd").value = json.kd;
      
      // MQTT (only populate if empty to avoid jumping)
      if (document.activeElement.id !== "input_mqtt_server" && document.getElementById("input_mqtt_server").value == "") 
          document.getElementById("input_mqtt_server").value = json.mqtt_server || "";
      if (document.activeElement.id !== "input_mqtt_port" && document.getElementById("input_mqtt_port").value == "1883") 
          document.getElementById("input_mqtt_port").value = json.mqtt_port;
      if (document.activeElement.id !== "input_mqtt_user" && document.getElementById("input_mqtt_user").value == "") 
          document.getElementById("input_mqtt_user").value = json.mqtt_user || "";
      // Pass is usually not sent back for security, or we can send it. In status handler we sent it, so we can populate it.
      if (document.activeElement.id !== "input_mqtt_pass" && document.getElementById("input_mqtt_pass").value == "") 
          document.getElementById("input_mqtt_pass").value = json.mqtt_pass || "";
    }
  };
  xhttp.open("GET", "/status", true);
  xhttp.send();
}, 2000 ) ;
</script>
</body>
</html>)rawliteral";

void setupWeb() {
  WiFi.mode(WIFI_STA);

  // WiFiManager
  WiFiManager wm;

  // wm.resetSettings(); // Unlock to reset if needed

  bool res;
  res = wm.autoConnect("GaggiaPID_Setup");

  if (!res) {
    Serial.println("Failed to connect");
  } else {
    Serial.println("connected...yeey :)");
    Serial.println(WiFi.localIP());
  }

  // Main Page Handler
  server.on("/", HTTP_GET, []() { server.send(200, "text/html", index_html); });

  // Status Handler
  server.on("/status", HTTP_GET, []() {
    String json = "{";
    json += "\"temp\":" + String(currentTemperature);
    json += ",\"target\":" + String(Setpoint);
    json += ",\"output\":" + String(Output);
    json += ",\"kp\":" + String(Kp);
    json += ",\"ki\":" + String(Ki);
    json += ",\"kd\":" + String(Kd);

    Preferences preferences;
    preferences.begin("gaggia", true);
    json +=
        ",\"mqtt_server\":\"" + preferences.getString("mqtt_server", "") + "\"";
    json += ",\"mqtt_port\":" + String(preferences.getInt("mqtt_port", 1883));
    json += ",\"mqtt_user\":\"" + preferences.getString("mqtt_user", "") + "\"";
    json += ",\"mqtt_pass\":\"" + preferences.getString("mqtt_pass", "") + "\"";
    preferences.end();

    json += "}";
    server.send(200, "application/json", json);
  });

  // Settings Update Handler
  server.on("/update", HTTP_GET, []() {
    Preferences preferences;
    preferences.begin("gaggia", false); // false = read/write

    if (server.hasArg("target")) {
      Setpoint = server.arg("target").toDouble();
      preferences.putDouble("target", Setpoint);
    }
    if (server.hasArg("kp")) {
      Kp = server.arg("kp").toDouble();
      preferences.putDouble("kp", Kp);
    }
    if (server.hasArg("ki")) {
      Ki = server.arg("ki").toDouble();
      preferences.putDouble("ki", Ki);
    }
    if (server.hasArg("kd")) {
      Kd = server.arg("kd").toDouble();
      preferences.putDouble("kd", Kd);
    }

    // MQTT Settings
    if (server.hasArg("mqtt_server")) {
      preferences.putString("mqtt_server", server.arg("mqtt_server"));
    }
    if (server.hasArg("mqtt_port")) {
      preferences.putInt("mqtt_port", server.arg("mqtt_port").toInt());
    }
    if (server.hasArg("mqtt_user")) {
      preferences.putString("mqtt_user", server.arg("mqtt_user"));
    }
    if (server.hasArg("mqtt_pass")) {
      preferences.putString("mqtt_pass", server.arg("mqtt_pass"));
    }

    preferences.end();

    myPID.SetTunings(Kp, Ki, Kd);
    server.sendHeader("Location", "/");
    server.send(303);

    // Reboot to apply MQTT settings cleanly (simplest way)
    if (server.hasArg("mqtt_server")) {
      delay(500);
      ESP.restart();
    }
  });

  // OTA Update Form
  server.on("/firmware", HTTP_GET, []() {
    String html = "<html><body><h2>OTA Update</h2>";
    html += "<form method='POST' action='/update_fw' "
            "enctype='multipart/form-data'>";
    html += "<input type='file' name='update'>";
    html += "<input type='submit' value='Update Firmware'>";
    html += "</form></body></html>";
    server.send(200, "text/html", html);
  });

  // OTA Update Handler
  server.on(
      "/update_fw", HTTP_POST,
      []() {
        server.send(200, "text/plain",
                    (Update.hasError()) ? "Update Failed"
                                        : "Update Success! Restarting...");
        ESP.restart();
      },
      []() {
        HTTPUpload &upload = server.upload();
        if (upload.status == UPLOAD_FILE_START) {
          Serial.printf("Update: %s\n", upload.filename.c_str());
          if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
            Update.printError(Serial);
          }
        } else if (upload.status == UPLOAD_FILE_WRITE) {
          if (Update.write(upload.buf, upload.currentSize) !=
              upload.currentSize) {
            Update.printError(Serial);
          }
        } else if (upload.status == UPLOAD_FILE_END) {
          if (Update.end(true)) {
            Serial.printf("Update Success: %u\n", upload.totalSize);
          } else {
            Update.printError(Serial);
          }
        }
      });

  server.begin();
}

// Function to handle client requests in the loop (standard WebServer needs
// this)
void handleWebLoop() { server.handleClient(); }
