  #include <ESP8266WiFi.h>
  #include <ESPAsyncTCP.h>
  #include <ESPAsyncWebServer.h>
  #include <Arduino_JSON.h>
  #include "wificonfig.h"
  #include "sensors.h"
  #include <Wire.h>
  #include "ClosedCube_HDC1080.h"
  #if defined(ESP8266)
  #include <ESP8266WiFi.h>
  #endif
  #define GPIO02 2
  

  ClosedCube_HDC1080 hdc1080;
  
  #define RESET false

  // current temperature & humidity, updated in loop()
  float temperature[9] = { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 };
  float humidity[9] = { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 };

  // Create a webserver at port 80
  AsyncWebServer server(80);

  // Will hold last time the sensor was updated
  unsigned long dhtpreviousMillis = 0;
  unsigned long rstpreviousMillis = 0;

  // Updates sensor reading every 10 seconds
  const long dhtinterval = 10000;  

  // Sleep interval every half minute
  const long rstinterval = 30000;  
  
  const char index_html[] PROGMEM = R"rawliteral(<!DOCTYPE HTML>
  <html><head><meta name="viewport" content="width=device-width, initial-scale=1"><link rel="stylesheet" href="https://use.fontawesome.com/releases/v5.7.2/css/all.css" integrity="sha384-fnmOCqbTlWIlj8LyTjo7mOUStjsKC4pOpQbqyi7RrhN7udi9RwhKkMHpvLbHG9Sr" crossorigin="anonymous"><title>Environmental data</title> 
  <style>
    * { box-sizing: border-box; }
    body { font-size: 1.25rem; font-family: sans-serif; line-height: 150%%; text-shadow: 0 2px 2px #787878; padding: 0px; margin: 0px;}
    section { color: #fff; text-align: center; }
    div { height: 100%%; }
    article { position: absolute; top: 50%%; left: 50%%; transform: translate(-50%%, -50%%); width: 100%%; padding: 20px; }
    h1 { font-size: 1.75rem; margin: 0 0 0.75rem 0; }
    .container { display: table; width: 100%%; }
    .left-half { background-color: #c4c4c4; position: absolute; left: 0px; width: 50%%; }
    .right-half { background-color: #a8a8a8; position: absolute; right: 0px; width: 50%%; }
    .units { font-size: 1.2rem; }
    .dht-section { display: block; }
    .dht-labels { font-size: 1.5rem; vertical-align: middle; padding-bottom: 15px; padding-right: 5px;}
    .dht-values {  font-size: 1.5rem; vertical-align: middle; padding-bottom: 15px; }
    .s0 { display: none; }
    .s1 { }
  </style>
  <script src="/reload.js"></script>
  </head>
  <body>
    <section class="container">
      <div class="left-half">
        <article>
          <h1>Temperatures</h1>
          <span class="dht-section s%S1%"><i class="fas fa-thermometer-half" style="color:#059e8a;">&nbsp;</i><span id="tn1" class="dht-labels">%N1%</span><span class="dht-labels">: </span><span id="t1" class="dht-values">%T1%</span><sup class="units">o</sup>C</span>
        </article>
      </div>
      <div class="right-half">
        <article>
          <h1>Humidity</h1>
          <span class="dht-section s%S1%"><i class="fas fa-tint" style="color:#00add6;">&nbsp;</i><span id="hn1" class="dht-labels">%N1%</span><span class="dht-labels">: </span><span id="h1" class="dht-values">%H1%</span><sup class="units">%%</sup></span>
        </article>
      </div>
    </section>
  </body>
  </html>)rawliteral";

  const char sensors_json[] PROGMEM = R"rawliteral([{"name":"%N0%", "state": "%S0%", "index": "0", "temperature": "%T0%", "humidity": "%H0%"},{"name":"%N1%", "state": "%S1%", "index": "1", "temperature": "%T1%", "humidity": "%H1%"},{"name":"%N2%", "state": "%S2%", "index": "2", "temperature": "%T2%", "humidity": "%H2%"},{"name":"%N3%", "state": "%S3%", "index": "3", "temperature": "%T3%", "humidity": "%H3%"},{"name":"%N4%", "state": "%S4%", "index": "4", "temperature": "%T4%", "humidity": "%H4%"},{"name":"%N5%", "state": "%S5%", "index": "5", "temperature": "%T5%", "humidity": "%H5%"},{"name":"%N6%", "state": "%S6%", "index": "6", "temperature": "%T6%", "humidity": "%H6%"},{"name":"%N7%", "state": "%S7%", "index": "7", "temperature": "%T7%", "humidity": "%H7%"},{"name":"%N8%", "state": "%S8%", "index": "8", "temperature": "%T8%", "humidity": "%H8%"}])rawliteral";
  const char zabbix_json[] PROGMEM = R"rawliteral({
  "data":[
      {"{#NAME}":"%N1%", "{#STATE}":"%S1%", "{#INDEX}":"1", "{#AVERAGE_L}":"%AVGL1%", "{#AVERAGE_H}":"%AVGH1%", "{#HIGH_L}":"%HIL1%", "{#HIGH_H}":"%HIH1%", "{#DISASTER_L}":"%DISL1%", "{#DISASTER_H}":"%DISH1%"}
    ]
  })rawliteral";

  const char reload_js[] PROGMEM = R"rawliteral(
  setInterval(function ( ) { var xhttp = new XMLHttpRequest(); xhttp.onreadystatechange = function() { if (this.readyState == 4 && this.status == 200) { document.getElementById("tn1").innerHTML = this.responseText; document.getElementById("hn1").innerHTML = this.responseText;}}; xhttp.open("GET", "/n/1", true); xhttp.send();}, 15000 );
  
  setInterval(function ( ) { var xhttp = new XMLHttpRequest(); xhttp.onreadystatechange = function() { if (this.readyState == 4 && this.status == 200) { document.getElementById("t1").innerHTML = this.responseText; }}; xhttp.open("GET", "/t/1", true); xhttp.send();}, 15000 );

  setInterval(function ( ) { var xhttp = new XMLHttpRequest(); xhttp.onreadystatechange = function() { if (this.readyState == 4 && this.status == 200) { document.getElementById("h1").innerHTML = this.responseText; }}; xhttp.open("GET", "/h/1", true); xhttp.send();}, 15000 );
  )rawliteral";

  // Replaces placeholder with DHT values
  String processor(const String& var) {
    // Serial.println(var);
   
    if(var == "N1") return String(DHT1_NAME);
    
    else if(var == "AVGL1") return String(DHT1_AVG_L);
    
    else if(var == "AVGH1") return String(DHT1_AVG_H);
   
    else if(var == "HIL1") return String(DHT1_HI_L);
    
    else if(var == "HIH1") return String(DHT1_HI_H);
   
    else if(var == "DISL1") return String(DHT1_DIS_L);
   
    else if(var == "DISH1") return String(DHT1_DIS_H);
    
    else if(var == "S1") return String(DHT1_ENABLED);
        
    else if(var == "T1") return String(temperature[1]);

    else if(var == "H1") return String(humidity[1]);
  
    return String();
  }

  void setup(){
    // Serial port for debugging purposes
    delay(1000);
    Serial.begin(9600);
    
    //pinMode(GPIO02, OUTPUT);
    //digitalWrite(GPIO02, HIGH);

    hdc1080.begin(0x40);

	  Serial.print("Manufacturer ID=0x");
	  Serial.println(hdc1080.readManufacturerId(), HEX); // 0x5449 ID of Texas Instruments
	  Serial.print("Device ID=0x");
	  Serial.println(hdc1080.readDeviceId(), HEX); // 0x1050 ID of the device
	

    Serial.println();
    Serial.print("MAC: ");
    Serial.println(WiFi.macAddress());
    Serial.print("Connecting to ");
    Serial.println(ssid);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }
    Serial.println("");
    Serial.println("WiFi connected.");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    Serial.println("----------------");
    

    if (DHT1_ENABLED == true) { 
        float newT1 = hdc1080.readTemperature();
        if (isnan(newT1)) { Serial.print("Failed to read temperature from HTC sensor, name: "); Serial.println(DHT1_NAME); }
        else { temperature[1] = newT1; Serial.print(DHT1_NAME); Serial.print(" temperature: "); Serial.println(temperature[1]); }
        float newH1 = hdc1080.readHumidity();
        if (isnan(newH1)) { Serial.print("Failed to read humidity from HTC sensor, name:    "); Serial.println(DHT1_NAME); }
        else { humidity[1] = newH1; Serial.print(DHT1_NAME); Serial.print(" humidity:    "); Serial.println(humidity[1]); }
      }


    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){ request->send_P(200, "text/html", index_html, processor); });
    server.on("/reload.js", HTTP_GET, [](AsyncWebServerRequest *request){ request->send_P(200, "application/javascript", reload_js, processor); });

    server.on("/sensors", HTTP_GET, [](AsyncWebServerRequest *request){ request->send_P(200, "application/json", sensors_json, processor); });
    server.on("/discovery", HTTP_GET, [](AsyncWebServerRequest *request){ request->send_P(200, "application/json", zabbix_json, processor); });
    server.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest *request){ request->send_P(200, "image/gif", "GIF89a"); });
   
    
    server.on("/n/1", HTTP_GET, [](AsyncWebServerRequest *request){ request->send_P(200, "text/plain", DHT1_NAME); });
   
    server.on("/t/1", HTTP_GET, [](AsyncWebServerRequest *request){ request->send_P(200, "text/plain", String(temperature[1]).c_str()); });
    
    server.on("/h/1", HTTP_GET, [](AsyncWebServerRequest *request){ request->send_P(200, "text/plain", String(humidity[1]).c_str()); });
    
    server.begin();

   
  }

  void(* resetFunc) (void) = 0; 
 
  void loop(){
    unsigned long dhtcurrentMillis = millis();
    unsigned long rstcurrentMillis = millis();
    if (dhtcurrentMillis - dhtpreviousMillis >= dhtinterval) {
      dhtpreviousMillis = dhtcurrentMillis;
      Serial.println('-----------------------------');
      Serial.print("Begin HDC transaction ");
      Serial.println(dhtcurrentMillis);

      if (DHT1_ENABLED == true) { 
        float newT1 = hdc1080.readTemperature();
        if (isnan(newT1)) { Serial.print("Failed to read temperature from HTC sensor, name: "); Serial.println(DHT1_NAME); }
        else { temperature[1] = newT1; Serial.print(DHT1_NAME); Serial.print(" temperature: "); Serial.println(temperature[1]); }
        float newH1 = hdc1080.readHumidity();
        if (isnan(newH1)) { Serial.print("Failed to read humidity from HTC sensor, name:    "); Serial.println(DHT1_NAME); }
        else { humidity[1] = newH1; Serial.print(DHT1_NAME); Serial.print(" humidity:    "); Serial.println(humidity[1]); }
      }
      
      Serial.print("End HDC transaction ");
      Serial.println(dhtcurrentMillis);
      Serial.println('-----------------------------');
    }
      if (rstcurrentMillis - rstpreviousMillis >= rstinterval) {
      rstpreviousMillis = rstcurrentMillis;
      Serial.println("agora vo dormi");
      ESP.deepSleep(1.8e9);
      }
  }
