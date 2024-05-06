#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <SoftwareSerial.h>
#include <ModbusRTU.h>         // https://github.com/emelianov/modbus-esp8266

// Update these with values suitable for your network.

const char* ssid = "MikroTik-37B8A2";
const char* password = "demoen24";
const char* mqtt_server = "192.168.88.4";

WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE	(50)
char msg[MSG_BUFFER_SIZE];
int value = 0;
/*===============================*/
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
/*===============================*/
EspSoftwareSerial::UART swSerial;
ModbusRTU modbus;
int modbusLastStatusCode = -1;

/*===============================*/
const char* state_topic="PZEM-003_sensor/state";


void setup_wifi() 
{
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
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) 
  {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish("outTopic", "hello world");
      // ... and resubscribe
      client.subscribe("inTopic");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void config()
  {

  if (!client.connected()) {   reconnect();  }
  client.loop();

  String MAC = WiFi.macAddress();
  MAC.replace(":","");
  const char* MAC_ = MAC.c_str();
  String device_name= "PZEM_003 "+String(WiFi.getHostname());


  DynamicJsonDocument doc(1024);

  doc["name"] = "PZEM_003 current";
  doc["unique_id"] = "current"; 
  doc["device_class"]   = "current";
  //doc["state_class"]   = "measurement";
  doc["unit_of_measurement"]   = "A";  
  doc["value_template"] = "{{value_json.current}}";
  doc["state_topic"] = state_topic;
  JsonObject device  = doc.createNestedObject("device");
 
  device["identifiers"][0] = MAC_;
  device["model"] = device_name.c_str();
  device["name"] = device_name.c_str();   
  device["manufacturer"] = "Home"; 
  device["sw_version"] = "1.0";  
  client.beginPublish("homeassistant/sensor/PZEM_003_current/config", measureJson(doc),1);
  serializeJson(doc, client);
  client.endPublish();
  delay(1000);

  doc["name"] = "PZEM_003 voltage";
  doc["unique_id"] = "voltage"; 
  doc["device_class"]   = "voltage";
  //doc["state_class"]   = "measurement";
  doc["unit_of_measurement"]   = "V";  
  doc["value_template"] = "{{value_json.voltage}}";
  doc["state_topic"] = state_topic;

  client.beginPublish("homeassistant/sensor/PZEM_003_voltage/config", measureJson(doc),1);
  serializeJson(doc, client);
  client.endPublish();
  delay(1000);



  doc["name"] = "PZEM_003 power";
  doc["unique_id"] = "power"; 
  doc["device_class"]   = "power";
  //doc["state_class"]   = "measurement";
  doc["unit_of_measurement"]   = "W";  
  doc["value_template"] = "{{value_json.power}}";
  doc["state_topic"] = state_topic;

  client.beginPublish("homeassistant/sensor/PZEM_003_power/config", measureJson(doc),1);
  serializeJson(doc, client);
  client.endPublish();


  delay(1000);

  doc["name"] = "PZEM_003 energy";
  doc["unique_id"] = "energy"; 
  doc["device_class"]   = "energy";
  //doc["state_class"]   = "measurement";
  doc["unit_of_measurement"]   = "Wh";  
  doc["value_template"] = "{{value_json.energy}}";
  doc["state_topic"] = state_topic;

  client.beginPublish("homeassistant/sensor/PZEM_003_energy/config", measureJson(doc),1);
  serializeJson(doc, client);
  client.endPublish();

  delay(1000);
  }


bool modbusStatusCallback(Modbus::ResultCode event, uint16_t transactionId, void* data) {
  // Serial.printf_P("Request result: 0x%02X, Mem: %d\n", event, ESP.getFreeHeap());
  modbusLastStatusCode = uint(event);
  return true;
}

void setup() 
{
  pinMode(LED, OUTPUT);     // Initialize the BUILTIN_LED pin as an output
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);

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

  // Generate the minified JSON and send it to the Serial port.
  //
  //створює MQTT пристрій
  config();

}


uint16_t registers[8];
void loop() {
  if (!client.connected()){reconnect();}
  
  client.loop();

  unsigned long now = millis();
  if (now - lastMsg > 1000) 
  {
    lastMsg = now;
    JsonDocument doc;
    char buffer[256];
    if (!modbus.slave()) {
      modbus.readIreg(PZEM_MODBUS_ID, 0, registers, 8, modbusStatusCallback);
      if (modbusLastStatusCode == 0) {
        doc["voltage"] = registers[0]/100.0 ;
        doc["current"] = registers[1]/100.0 ;
        doc["power"] = (((uint16_t)registers[3] << 16) + (uint16_t)registers[2])/10.0;
        doc["energy"] = (((uint16_t)registers[5] << 16) + (uint16_t)registers[4])/10.0;

        serializeJson(doc, buffer);
        client.publish(state_topic, buffer);
        Serial.println(buffer);
        }
    else {Serial.printf_P("Modbus error code: 0x%02X\n", modbusLastStatusCode);  }
  }
  modbus.task();  
  }
}
