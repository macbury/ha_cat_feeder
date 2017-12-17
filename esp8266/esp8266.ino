#include <Servo.h>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

#define PIN_SERVO D0
#define PIN_STATUS_LED D2
#define PIN_ACTION_BUTTON D3

#include "credentials.c"

const char* CMD_ON = "ON";
const char* CMD_OFF = "OFF";

WiFiClient net;
PubSubClient mqtt(net);
Servo foodServo;

void setupOTA() {
  Serial.println("Configuring ArduinoOTA");
  ArduinoOTA.setPort(OTA_PORT);
  ArduinoOTA.setHostname(OTA_HOST);
  ArduinoOTA.setPassword(OTA_PASSWORD);

  ArduinoOTA.onStart([]() {
    Serial.println("Starting");
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
}

void setup() {
  Serial.begin(115200);
  pinMode(PIN_STATUS_LED, OUTPUT);
  pinMode(PIN_ACTION_BUTTON, INPUT);
  digitalWrite(PIN_STATUS_LED, LOW);
  
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
    digitalWrite(PIN_STATUS_LED, HIGH);
    delay(250);
    digitalWrite(PIN_STATUS_LED, LOW);
  }
  Serial.println("OK!");
  randomSeed(micros());
  printWifiInfo();
  digitalWrite(PIN_STATUS_LED, LOW);
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
      Serial.println(MQTT_RESET_TOPIC);
      mqtt.subscribe(MQTT_RESET_TOPIC);
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
  
  /**
   * Transform this to more normal stuff like array of char not a bytefuck
   */
  char message[length + 1];
  for (int i = 0; i < length; i++) {
    message[i] = (char)payload[i];
  }
  message[length] = '\0';

  if (String(MQTT_RESET_TOPIC) == topic) {
    if (String(message) == CMD_ON) {
      Serial.println("Enable OTA");
      setupOTA();
    } else {
      Serial.println("Reseting ESP!");
      ESP.restart();
    }
    return;
  } else {
    dispenseFood();
  }
}

void dispenseFood() {
  digitalWrite(PIN_STATUS_LED, HIGH);
  foodServo.write(180);
  delay(1000);
  foodServo.write(0);
  delay(1000);
  digitalWrite(PIN_STATUS_LED, LOW);
  mqtt.publish(MQTT_COMPLETE_TOPIC, CMD_ON, false);
}


boolean actionButtonPressed() {
  int actionButtonState = digitalRead(PIN_ACTION_BUTTON);

  if (actionButtonState == HIGH) {
    return false;
  }

  digitalWrite(PIN_STATUS_LED, HIGH);
  while(actionButtonState == LOW) {
    delay(100);
    actionButtonState = digitalRead(PIN_ACTION_BUTTON);
  }
  digitalWrite(PIN_STATUS_LED, LOW);

  return true;
}


void loop() {
  if (mqtt.connected()) {
    mqtt.loop();
    ArduinoOTA.handle();
    if (actionButtonPressed()) {
      Serial.println("Give cat a food!");
      dispenseFood();
    }
  } else {
    ensureWifiConnection();
    ensureMqttConnection();
    delay(1000);
  }
}