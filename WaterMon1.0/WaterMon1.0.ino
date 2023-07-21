// WaterMon - IoT Water Monitor
// Written by Waldir Borba Junior
// Released under an MIT license.

// https://www.losant.com/blog/getting-started-with-the-esp8266-and-dht22-sensor
// https://gist.github.com/LosantGists/783405ca2bf7380c2aa6fd3855414b73

#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>

#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino

//needed for library
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>         //https://github.com/tzapu/WiFiManager



#define DEBUG true

#define DHTPIN            4         // {D2} Pin which is connected to the DHT sensor.

// Uncomment the type of sensor in use:
#define DHTTYPE           DHT11     // DHT 11 
//#define DHTTYPE           DHT22     // DHT 22 (AM2302)
//#define DHTTYPE           DHT21     // DHT 21 (AM2301)

DHT_Unified dht(DHTPIN, DHTTYPE);

uint32_t delayMS;

// WiFi credentials
const char* WIFI_SSID = "VIVO-0DEC";
const char* WIFI_PASS = "C6622E0DEC";

// ThingSpeak credentials
String apiKey = "O9XSNNY1NCBIAMSE";
const char* server = "api.thingspeak.com";

// WiFi Setup
WiFiClient client;

void setupWiFiAuto() {
  //WiFiManager
  //Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wifiManager;
  //reset saved settings
  //  wifiManager.resetSettings();

  if (WiFi.SSID() != "") wifiManager.setConfigPortalTimeout(60); //If no access point name has been previously entered disable timeout.


  //set custom ip for portal
  wifiManager.setAPStaticIPConfig(IPAddress(14, 6, 6, 9), IPAddress(10, 0, 0, 1), IPAddress(255, 255, 255, 0));

  //fetches ssid and pass from eeprom and tries to connect
  //if it does not connect it starts an access point with the specified name "WaterMonSetup"
  //and goes into a blocking loop awaiting configuration
  wifiManager.autoConnect("WaterMonSetup");
  //or use this for auto generated name ESP + ChipID
  //wifiManager.autoConnect();


  //if you get here you have connected to the WiFi
  Serial.println("connected...yeey :)");
}

void setupWiFi() {

  // Connect to WiFi network
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(WIFI_SSID);

  // ?????
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  WiFi.persistent(false);
  WiFi.mode(WIFI_OFF);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  unsigned long wifiConnectStart = millis();

  while (WiFi.status() != WL_CONNECTED) {

    if (WiFi.status() == WL_CONNECT_FAILED) {
      Serial.println("Failed to connect to WIFI. Please verify credentials: ");
      delay(10000);
      //      Serial.println();
      //      Serial.print("SSID: ");
      //      Serial.println(WIFI_SSID);
      //      Serial.print("Password: ");
      //      Serial.println(WIFI_PASS);
      //      Serial.println();
    }

    delay(500);
    Serial.print("...");

    // Only try for 5 seconds.
    if (millis() - wifiConnectStart > 15000) {
      Serial.println("Failed to connect to WiFi");
      Serial.println("Please attempt to send updated configuration parameters.");
      return;
    }
  }

  if (DEBUG) {
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
    Serial.println();
  }

  Serial.print("Authenticating Device...");

}

void setupDHT() {
  // Initialize device.
  dht.begin();
  Serial.println("DHTxx Unified Sensor");
  // Print temperature sensor details.
  sensor_t sensor;
  dht.temperature().getSensor(&sensor);
  Serial.println("------------------------------------");
  Serial.println("Temperature");
  Serial.print  ("Sensor:       "); Serial.println(sensor.name);
  Serial.print  ("Driver Ver:   "); Serial.println(sensor.version);
  Serial.print  ("Unique ID:    "); Serial.println(sensor.sensor_id);
  Serial.print  ("Max Value:    "); Serial.print(sensor.max_value); Serial.println(" *C");
  Serial.print  ("Min Value:    "); Serial.print(sensor.min_value); Serial.println(" *C");
  Serial.print  ("Resolution:   "); Serial.print(sensor.resolution); Serial.println(" *C");
  Serial.println("------------------------------------");
  // Print humidity sensor details.
  dht.humidity().getSensor(&sensor);
  Serial.println("------------------------------------");
  Serial.println("Humidity");
  Serial.print  ("Sensor:       "); Serial.println(sensor.name);
  Serial.print  ("Driver Ver:   "); Serial.println(sensor.version);
  Serial.print  ("Unique ID:    "); Serial.println(sensor.sensor_id);
  Serial.print  ("Max Value:    "); Serial.print(sensor.max_value); Serial.println("%");
  Serial.print  ("Min Value:    "); Serial.print(sensor.min_value); Serial.println("%");
  Serial.print  ("Resolution:   "); Serial.print(sensor.resolution); Serial.println("%");
  Serial.println("------------------------------------");
  // Set delay between sensor readings based on sensor details.
  delayMS = sensor.min_delay / 1000;
}

float readLDR() {
  int sensorValue = analogRead(A0);   // read the input on analog pin 0
  //  float voltage = sensorValue * (5.0 / 1023.0);   // Convert the analog reading (which goes from 0 - 1023) to a voltage (0 - 5V)
  //  float voltage = sensorValue * (3.0 / 1023.0);   // Convert the analog reading (which goes from 0 - 1023) to a voltage (0 - 5V)
  float voltage = sensorValue;

  //  <= 200 - Dark

  return voltage;
}

void sendData(float temperature, float humidity, float lumen) {

  if (client.connect(server, 80)) { // "184.106.153.149" or api.thingspeak.com

    String postStr = apiKey;
    postStr += "&field1=";
    postStr += String(temperature);
    postStr += "&field2=";
    postStr += String(humidity);
    postStr += "&field3=";
    postStr += String(lumen);
    postStr += "\r\n\r\n";

    client.print("POST /update HTTP/1.1\n");
    client.print("Host: api.thingspeak.com\n");
    client.print("Connection: close\n");
    client.print("X-THINGSPEAKAPIKEY: " + apiKey + "\n");
    client.print("Content-Type: application/x-www-form-urlencoded\n");
    client.print("Content-Length: ");
    client.print(postStr.length());
    client.print("\n\n");
    client.print(postStr);
  }
  client.stop();

}



void setup() {
  Serial.begin(9600);
  Serial.setTimeout(2000);

  // Wait for serial to initialize
  while (!Serial) { }

  setupWiFiAuto();

  setupDHT();

}



void loop() {

  // Get temperature event and print its value.
  sensors_event_t event;

  dht.temperature().getEvent(&event);
  float t = event.temperature;

  // Get humidity event and print its value.
  dht.humidity().getEvent(&event);
  float h = event.relative_humidity;

  if ( isnan(t) || isnan(h) ) {
    if (DEBUG) {
      Serial.println("Failed to read from DHT sensor!");
    }
    delay(5000);
    return;
  }

  float l = readLDR();

  if (DEBUG) {
    Serial.print("LDR: ");
    Serial.print(l);   // print out the value you read
    Serial.println(" LUMEN");
  }

  // Send data to cloud
  sendData(t, h, l);

  if (DEBUG) {
    Serial.print("Temperature: ");
    Serial.print(t);
    Serial.println(" *C");

    Serial.print("Humidity: ");
    Serial.print(h);
    Serial.println("%");
  }

  if (DEBUG) {
    Serial.println("Waiting…");
  }

  // thingspeak needs minimum 30 sec delay between updates
  delay(40000);

}
