#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <PubSubClient.h> 
#include <ArduinoJson.h>
#include <DHT.h>


#define DHTPIN            D4         // Pin which is connected to the DHT sensor.
#define DHTTYPE           DHT11     // DHT 11 


/************ WiFI ******************/
const char* WIFI_SSID = "**CONF_WIFI_SSID**"; 
const char* WIFI_PASSWORD =  "**CONF_WIFI_PASSWORD**"; 


/************ MQTT Server ******************/
#define MQTT_VERSION MQTT_VERSION_3_1_1

const PROGMEM char* MQTT_CLIENT_ID = "office_dht11";
const PROGMEM char* MQTT_SERVER_IP = "192.168.1.30";
const PROGMEM uint16_t MQTT_SERVER_PORT = 1883;
const PROGMEM char* MQTT_USER = "";
const PROGMEM char* MQTT_PASSWORD = "";


/************ MQTT Topics ******************/
const PROGMEM char* MQTT_SENSOR_TOPIC = "bedroom/sensor1";


/************ sleeping time  ******************/
const PROGMEM uint16_t SLEEPING_TIME_IN_SECONDS = 300; // 5 minutes x 60 seconds


/************ Vars ******************/ 
WiFiClient espClient;
PubSubClient client(espClient);
DHT dht(DHTPIN, DHTTYPE);


// function called to publish the temperature and the humidity
void publishData(float p_temperature, float p_humidity) {
  // create a JSON object
  // doc : https://github.com/bblanchon/ArduinoJson/wiki/API%20Reference
  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  // INFO: the data must be converted into a string; a problem occurs when using floats...
  root["temperature"] = (String)p_temperature;
  root["humidity"] = (String)p_humidity;
  root.prettyPrintTo(Serial);
  Serial.println("");
  /*
     {
        "temperature": "23.20" ,
        "humidity": "43.70"
     }
  */
  char data[200];
  root.printTo(data, root.measureLength() + 1);
  client.publish(MQTT_SENSOR_TOPIC, data, true);
}


void callback(char* p_topic, byte* p_payload, unsigned int p_length) {
}





void setup() {
  Serial.begin(115200);
  
  delay(3000);
  
  Serial.println("Booting");
  
  connectToWIFI();
  connectToMQTT();

  OTASetup();
  
  dht.begin();
}

void loop() {

  if (WiFi.status() != WL_CONNECTED) {
    connectToWIFI();
  }

  if (!client.connected()) {
    Serial.println("MQTT Disconnected ");
    connectToMQTT();
  }

  ArduinoOTA.handle();
  

  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  float h = dht.readHumidity();
  // Read temperature as Celsius (the default)
  float t = dht.readTemperature();

  if (isnan(h) || isnan(t)) {
    Serial.println("ERROR: Failed to read from DHT sensor!");
    delay(5000);
    return;
  } else {
    Serial.println(h);
    Serial.println(t);
    publishData(t, h);
  }




  Serial.println("INFO: Closing the MQTT connection");
  client.disconnect();

  Serial.println("INFO: Closing the Wifi connection");
  WiFi.disconnect();

  //ESP.deepSleep(SLEEPING_TIME_IN_SECONDS * 1000000, WAKE_RF_DEFAULT);
  delay(SLEEPING_TIME_IN_SECONDS * 1000); // wait for deep sleep to happen
  
  

}



void connectToWIFI(){ 
  delay(2000);
  Serial.println();
  WiFi.disconnect();
  Serial.print("Connecting to ");
  Serial.println(WIFI_SSID);
  Serial.println(WIFI_PASSWORD);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }
  
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  
}


void connectToMQTT(){ 
  delay(3000);
  client.setServer(MQTT_SERVER_IP, MQTT_SERVER_PORT);
  //  client.setCallback(MQTTCallback);  
  while (!client.connected()) { 
    Serial.print("Attempting MQTT connection... "); 
    if (client.connect("Temp Sensor", MQTT_USER, MQTT_PASSWORD)) {
      break; 
    } else {
      Serial.println("failed...");
      delay(5000);
    }
  } 

  if (!client.connected()){ 
    connectToMQTT();
  }else{
    Serial.println("Connected");
    SubscribeToMQTT();
    client.setCallback(callback);
  }
}

void SubscribeToMQTT(){

}

void OTASetup(){
  

  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  // ArduinoOTA.setHostname("myesp8266");

  // No authentication by default
  // ArduinoOTA.setPassword((const char *)"123");
  
  ArduinoOTA.onStart([]() {
    Serial.println("Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}
