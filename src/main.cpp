#include "WiFi.h"
#include "ESPAsyncWebServer.h"
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <HTTPClient.h>

// Alterar senha do wifi
const char *ssid = "";
const char *password = "";
const char *token = "";
const char *endPointSiot = "https://test.pub.host.com/api/v1";
String idMachine = "";

bool initMachine = false;

#define DHTPIN 25
#define PIN_LED_RUN 21
#define PIN_SENSOR_PRESENCA 22

#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);

// cria uma página na porta 80
AsyncWebServer server(80);

uint16_t delayEnviarSensor = 0;
uint16_t delaySaveEncontrado = 0;

String readDHTTemperature()
{
  float t = dht.readTemperature();
  if (isnan(t))
  {
    digitalWrite(PIN_LED_RUN, LOW); // apaga led ok
    Serial.println("Failed to read from DHT sensor!");
    return "--";
  }
  else
  {
    digitalWrite(PIN_LED_RUN, HIGH); // acende led ok
    Serial.println(t);
    return String(t);
  }
}

String readDHTHumidity()
{
  float h = dht.readHumidity();
  if (isnan(h))
  {
    Serial.println("Failed to read from DHT sensor!");
    return "--";
  }
  else
  {
    Serial.println(h);
    return String(h);
  }
}

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <link rel="stylesheet" href="https://use.fontawesome.com/releases/v5.7.2/css/all.css" integrity="sha384-fnmOCqbTlWIlj8LyTjo7mOUStjsKC4pOpQbqyi7RrhN7udi9RwhKkMHpvLbHG9Sr" crossorigin="anonymous">
  <style>
    html {
     font-family: Arial;
     display: inline-block;
     margin: 0px auto;
     text-align: center;
    }
    h2 { font-size: 3.0rem; }
    p { font-size: 3.0rem; }
    .units { font-size: 1.2rem; }
    .dht-labels{
      font-size: 1.5rem;
      vertical-align:middle;
      padding-bottom: 15px;
    }
  </style>
</head>
<body>
  <h2>ESP32 DHT Server</h2>
  <p>
    <i class="fas fa-thermometer-half" style="color:#059e8a;"></i>
    <span class="dht-labels">Temperature</span>
    <span id="temperature">%TEMPERATURE%</span>
    <sup class="units">&deg;C</sup>
  </p>
  <p>
    <i class="fas fa-tint" style="color:#00add6;"></i>
    <span class="dht-labels">Humidity</span>
    <span id="humidity">%HUMIDITY%</span>
    <sup class="units">%</sup>
  </p>
</body>
<script>
setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("temperature").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/temperature", true);
  xhttp.send();
}, 10000 ) ;

setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("humidity").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/humidity", true);
  xhttp.send();
}, 60000 ) ;
</script>
</html>)rawliteral";

String processor(const String &var)
{
  if (var == "TEMPERATURE")
  {
    return readDHTTemperature();
  }
  else if (var == "HUMIDITY")
  {
    return readDHTHumidity();
  }
  return String();
}

void ConnectingWifi()
{
  digitalWrite(PIN_LED_RUN, LOW);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }
  Serial.println(WiFi.localIP());
}

void ConfigureServer()
{
  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html", index_html, processor);
  });
  server.on("/temperature", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/plain", readDHTTemperature().c_str());
  });
  server.on("/humidity", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/plain", readDHTHumidity().c_str());
  });

  // Start server
  server.begin();
}

void sendHttp(String pathUri, String body)
{
  HTTPClient http;

  http.begin(endPointSiot + pathUri);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("key-token", token);
  int httpResponseCode = http.POST(body);
  if (httpResponseCode > 0)
  {
    Serial.println(httpResponseCode);
  }
  else
  {
    Serial.print("Error on sending POST: ");
    Serial.println(httpResponseCode);
  }
  http.end();
}

void setup()
{
  Serial.begin(115200);
  pinMode(PIN_LED_RUN, OUTPUT);
  pinMode(PIN_SENSOR_PRESENCA, INPUT);
  dht.begin();

  ConnectingWifi();
  ConfigureServer();
}

void loop()
{
  int signal = digitalRead(PIN_SENSOR_PRESENCA);
  if (signal == HIGH && initMachine && delaySaveEncontrado == 0)
  {
    delaySaveEncontrado = 20000;
    sendHttp("/state/sensor", "{\"idMachine\":  \""+ idMachine +"\",\"status\": \"active\",\"date\": \"\",\"message\": \"Sensor ativo\",\"sensorId\": \"scanner\",\"sensor\": \"Scanner\",\"signals\": [{\"signal\": \"detected\",\"value\": true }]}");
  }

  delay(1);
  if (delaySaveEncontrado == 1) {
    sendHttp("/state/sensor", "{\"idMachine\":  \""+ idMachine +"\",\"status\": \"active\",\"date\": \"\",\"message\": \"Sensor ativo\",\"sensorId\": \"scanner\",\"sensor\": \"Scanner\",\"signals\": [{\"signal\": \"detected\",\"value\": false }]}");
  }
  if (delaySaveEncontrado > 0){
    delaySaveEncontrado--;
  }
    

  delayEnviarSensor++;

  if (!initMachine || (delayEnviarSensor > 60000))
  {

    if (WiFi.status() == WL_CONNECTED)
    {
      if (!initMachine)
      {
        sendHttp("/state/machine", "{\"idMachine\": \""+ idMachine +"\",\"operation\": \"Analisar temperatura/umidade\",\"status\": \"active\",\"date\": \"\",\"message\": \"\"}");
        initMachine = true;
      }
      String temperatureNow = readDHTTemperature().c_str();
      String temperatureStatus = "active";
      if (temperatureNow == "--")
      {
        temperatureNow = "0";
        temperatureStatus = "problem";
      }

      sendHttp("/state/sensor", "{\"idMachine\":  \""+ idMachine +"\",\"status\": \"" + temperatureStatus + "\",\"date\": \"\",\"message\": \"Sensor ativo\",\"sensorId\": \"temperature\",\"sensor\": \"Temperatura\",\"signals\": [{\"signal\": \"temperature\",\"value\":" + temperatureNow + ",\"unity\": \"ºC\"}]}");

      String humidityNow = readDHTHumidity().c_str();
      String himidityStatus = "active";
      if (humidityNow == "--")
      {
        humidityNow = "0";
        himidityStatus = "problem";
      }

      sendHttp("/state/sensor", "{\"idMachine\":  \""+ idMachine +"\",\"status\": \"" + himidityStatus + "\",\"date\": \"\",\"message\": \"Sensor ativo\",\"sensorId\": \"humidity\",\"sensor\": \"Umidade\",\"signals\": [{\"signal\": \"humidity\",\"value\":" + humidityNow + ",\"unity\": \"%\"}]}");
    }
    else
    {
      WiFi.disconnect();
      ConnectingWifi();
    }
    delayEnviarSensor = 0;
  }
}
