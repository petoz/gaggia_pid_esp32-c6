#include "config.h"
#include <Arduino.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <PID_v1.h>
#include <WiFi.h>

extern float currentTemperature;
extern double Setpoint, Input, Output;
extern double Kp, Ki, Kd;
extern PID myPID;

AsyncWebServer server(80);

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
      Target: <input type="number" step="0.1" name="target" value=""><br>
      Kp: <input type="number" step="0.1" name="kp" value=""><br>
      Ki: <input type="number" step="0.1" name="ki" value=""><br>
      Kd: <input type="number" step="0.1" name="kd" value=""><br>
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
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("Connecting to WiFi");
  int tryCount = 0;
  while (WiFi.status() != WL_CONNECTED && tryCount < 20) {
    delay(500);
    Serial.print(".");
    tryCount++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("WiFi Connection Failed!");
  }

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/html", index_html);
  });

  server.on("/status", HTTP_GET, [](AsyncWebServerRequest *request) {
    String json = "{";
    json += "\"temp\":" + String(currentTemperature);
    json += ",\"target\":" + String(Setpoint);
    json += ",\"output\":" + String(Output);
    json += ",\"kp\":" + String(Kp);
    json += ",\"ki\":" + String(Ki);
    json += ",\"kd\":" + String(Kd);
    json += "}";
    request->send(200, "application/json", json);
  });

  server.on("/update", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (request->hasParam("target")) {
      Setpoint = request->getParam("target")->value().toDouble();
    }
    if (request->hasParam("kp")) {
      Kp = request->getParam("kp")->value().toDouble();
    }
    if (request->hasParam("ki")) {
      Ki = request->getParam("ki")->value().toDouble();
    }
    if (request->hasParam("kd")) {
      Kd = request->getParam("kd")->value().toDouble();
    }
    myPID.SetTunings(Kp, Ki, Kd);
    request->redirect("/");
  });

  server.begin();
}
