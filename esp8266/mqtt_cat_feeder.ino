#include <Servo.h> 
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

#include "credentials.c"

WiFiClient net;
PubSubClient mqtt(net);
Servo foodServo;

void setup() {
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);
  foodServo.attach(PIN_SERVO);
  foodServo.write(0);

  mqtt.setServer(MQTT_HOST, MQTT_PORT);
  mqtt.setCallback(on_mqtt_message);
}

void ensureWifiConnection() {
  delay(1000);
  Serial.println("----------");
  Serial.println("Connecting to: ");
  Serial.println(WIFI_SSID);
  WiFi.mode(WIFI_STA);
  
  while (WiFi.waitForConnectResult() != WL_CONNECTED){
    Serial.print(".");
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    
    delay(500);
  }
  Serial.println("OK!");
  randomSeed(micros());
  printWifiInfo();
}

void printWifiInfo() {
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void ensureMqttConnection() {
  while (!mqtt.connected()) {
    Serial.println("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "CatFeeder-";
    clientId += String(random(0xffff), HEX);
    Serial.print("Client id: ");
    Serial.println(clientId);
    if (mqtt.connect(clientId.c_str(), MQTT_USER, MQTT_PASSWORD)) {
      printWifiInfo();
      Serial.print("Subscribing: ");
      Serial.println(MQTT_TRIGGER_TOPIC);
      mqtt.subscribe(MQTT_TRIGGER_TOPIC);
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqtt.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void on_mqtt_message(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.println("] ");

  digitalWrite(LED_BUILTIN, LOW);
  foodServo.write(180);
  delay(1500);
  digitalWrite(LED_BUILTIN, HIGH);
  foodServo.write(0);
  delay(1500);
}

void loop() {
  if (mqtt.connected()) {
    mqtt.loop();
  } else {
    ensureWifiConnection();
    ensureMqttConnection();
    delay(1000);
  }
}
