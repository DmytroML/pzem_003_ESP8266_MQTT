/*
  ModbusRTU ESP8266/ESP32
  Read multiple coils from slave device example

  (c)2019 Alexander Emelianov (a.m.emelianov@gmail.com)
  https://github.com/emelianov/modbus-esp8266

  modified 13 May 2020
  by brainelectronics

  This code is licensed under the BSD New License. See LICENSE.txt for more info.
*/

#include <Arduino.h>
//#include <ArduinoOTA.h>
#include <ModbusRTU.h>         // https://github.com/emelianov/modbus-esp8266
#include <SoftwareSerial.h>

// ENM deps
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266HTTPUpdateServer.h>
// application deps
#include <PubSubClient.h>

#define INTERVAL 1000

const char* ssid = "Hygge_2G";
const char* password = "demoen24";
const char* mqtt_server = "192.168.88.4";
#define mqtt_login "root"
#define mqtt_PSK  "demoen24"

// ESP8266 pins
#define LED LED_BUILTIN
#define D6 12
#define D7 13
#define D8 15


// My settings
#define SERIAL_RX D8
#define SERIAL_TX D7
#define SERIAL_CONTROL D6

#define PZEM_MODBUS_ID 0x01

/*=============================================*/
WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE	(50)
char msg[MSG_BUFFER_SIZE];
int value = 0;
/*=============================================*/
EspSoftwareSerial::UART swSerial;
ModbusRTU modbus;
int modbusLastStatusCode = -1;
void ledOn() {  digitalWrite(LED, LOW);}
/*=============================================*/


bool modbusStatusCallback(Modbus::ResultCode event, uint16_t transactionId, void* data) {
  // Serial.printf_P("Request result: 0x%02X, Mem: %d\n", event, ESP.getFreeHeap());
  modbusLastStatusCode = uint(event);
  return true;
}

void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
  // Switch on the LED if an 1 was received as first character
  if ((char)payload[0] == '1') {
    digitalWrite(BUILTIN_LED, LOW);   // Turn the LED on (Note that LOW is the voltage level
    // but actually the LED is on; this is because
    // it is active low on the ESP-01)
  } else {
    digitalWrite(BUILTIN_LED, HIGH);  // Turn the LED off by making the voltage HIGH
  }
}
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    //if (client.connect(clientId.c_str(), mqtt_login, mqtt_PSK)) {
      if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish("outTopic", "hello world");
      // ... and resubscribe
      client.subscribe("livingroom/AC/Temp");    
      //Temp: 26C, Power: On, Mode: 1 (Cool), Fan: 1 (Low), Turbo: Off, Econo: Off
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}


void setup() {
  Serial.begin(115200);
  pinMode(LED, OUTPUT);
  digitalWrite(LED, LOW);
  Serial.println("OK");
  //setup_wifi();
  //client.setServer(mqtt_server, 1883);
  //client.setCallback(callback);



/*

  swSerial.begin(9600
                , SWSERIAL_8N1
                , SERIAL_RX
                , SERIAL_TX
                , false);
  modbus.begin(&swSerial, SERIAL_CONTROL);
  modbus.client();
  modbus.task();
  Serial.println("OK");
  Serial.print("Starting signalK client...");
  Serial.println("OK");
  Serial.println("Setup finished");
*/
}


int connect_mqtt() {
  if (client.connected()) {
    return 0;
  }

  Serial.print("Attempting MQTT connection...");

  // Create a random client ID
  String clientId = "ESP8266Client-";
  clientId += String(random(0xffff), HEX);

  // Attempt to connect
  if (client.connect(clientId.c_str())) {
    Serial.println("connected");
    return 0;
  }

  Serial.print("failed, rc=");
  Serial.print(client.state());
  return 1;
}

uint16_t registers[8];
float voltage;
float current;


void loop() {
/*  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  unsigned long now = millis();
  if (now - lastMsg > 5000) {
    lastMsg = now;
    ++value;
    snprintf (msg, MSG_BUFFER_SIZE, "hello world #%ld", value);
    
    
    Serial.print("Publish message: ");
    Serial.println(msg);
    
    
    client.publish("outTopic", msg);
  };


  if (!modbus.slave()) {
    modbus.readIreg(PZEM_MODBUS_ID, 0, registers, 2, modbusStatusCallback);
    if (modbusLastStatusCode == 0) {
        voltage = registers[0] / 100.0;
        current = registers[1] / 100.0;
        //myIP = WiFi.localIP().toString();

        Serial.print("Voltage: ");
        Serial.print(voltage);
        Serial.print("V, Current: ");
        Serial.print(current);
        Serial.print("3 ");
        Serial.print(registers[3] / 10.0);
        Serial.print("4 ");
        Serial.print(registers[4] / 10.0);
        Serial.print("5 ");
        Serial.print(registers[5] / 10.0);
        Serial.print("6 ");
        Serial.print(registers[6] / 10.0);
        Serial.print("7 ");
        Serial.println(registers[7] / 10.0);

    }
    else {
        Serial.printf_P("Modbus error code: 0x%02X\n", modbusLastStatusCode);
    }
  }
  modbus.task();

  ledOn();
*/

}
