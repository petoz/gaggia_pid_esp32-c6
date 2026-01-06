#include "config.h"
#include <Arduino.h>
#include <PID_v1.h>
#include <WebServer.h>
#include <WiFi.h>
#include <WiFiManager.h>

#include <Preferences.h>

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
      Target: <input type="number" step="0.1" name="target" id="input_target" value=""><br>
      Kp: <input type="number" step="0.1" name="kp" id="input_kp" value=""><br>
      Ki: <input type="number" step="0.1" name="ki" id="input_ki" value=""><br>
      Kd: <input type="number" step="0.1" name="kd" id="input_kd" value=""><br>
      <input type="submit" value="Update" class="button">
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
      
      // Update inputs only if not focused to avoid overwriting user while typing
      if (document.activeElement.id !== "input_target" && document.getElementById("input_target").value == "") document.getElementById("input_target").value = json.target;
      if (document.activeElement.id !== "input_kp" && document.getElementById("input_kp").value == "") document.getElementById("input_kp").value = json.kp;
      if (document.activeElement.id !== "input_ki" && document.getElementById("input_ki").value == "") document.getElementById("input_ki").value = json.ki;
      if (document.activeElement.id !== "input_kd" && document.getElementById("input_kd").value == "") document.getElementById("input_kd").value = json.kd;
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

  server.on("/", HTTP_GET, []() { server.send(200, "text/html", index_html); });

  server.on("/status", HTTP_GET, []() {
    String json = "{";
    json += "\"temp\":" + String(currentTemperature);
    json += ",\"target\":" + String(Setpoint);
    json += ",\"output\":" + String(Output);
    json += ",\"kp\":" + String(Kp);
    json += ",\"ki\":" + String(Ki);
    json += ",\"kd\":" + String(Kd);
    json += "}";
    server.send(200, "application/json", json);
  });

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
    preferences.end();

    myPID.SetTunings(Kp, Ki, Kd);
    server.sendHeader("Location", "/");
    server.send(303);
  });

  server.begin();
}

// Function to handle client requests in the loop (standard WebServer needs
// this)
void handleWebLoop() { server.handleClient(); }
