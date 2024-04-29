#include <SPI.h>
#include <Wire.h>
#include <Adafruit_MLX90614.h>
#include <time.h>
#include <PubSubClient.h>
#include "config.h"
#include <WiFi.h>

Adafruit_MLX90614 mlx = Adafruit_MLX90614();

struct tm timeinfo;
char messageBuffer[512];

WiFiClient espClient;
PubSubClient client(espClient);

//Methoden die in void setup verwendet werden, müssen vor void setup deklariert werden, deshalb void setup und void loop am Ende des Codes

//Verbindung mit WiFi aufbauen
void connectWifi() {
  WiFi.mode(WIFI_STA); //WiFi Station Mode, Alternativ WIFI_AP, dann baut ESP eigenen AP(AccesPoint) auf 
  WiFi.begin(ssid, password);
  delay(1000);
  while (WiFi.status() != WL_CONNECTED) { //während Verbindung nicht aufgebaut ist
    Serial.print(".");
    delay(1000);
  }
    Serial.print("\nWifi connected, IP of this ESP32: ");
    Serial.print(WiFi.localIP());
    Serial.print(", MAC-Adresse: ");
    Serial.println(WiFi.macAddress());
}

//MQTT-Setup
//callback Routine für MQTT-Broker
void callbackRoutine(char* topic, byte* message, unsigned int length){
  Serial.print("Message received on Topic: ");
  Serial.print(topic); // Ausgabe des empfangenen Topics
  Serial.print("  Message: ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)message[i]);
  }
  Serial.println();
}

void connectMqtt() {
  Serial.println("Connecting to MQTT...");
  client.setServer(MQTT_BROKER_IP, MQTT_PORT); //MQTT-Broker und PORT über den kommuniziert wird
  
  while (!client.connected()) {
    if (!client.connect(MQTT_CLIENTID, MQTT_USERNAME, MQTT_PASSWORD)) {
      Serial.print("MQTT connection failed:");
      Serial.print(client.state());
      Serial.println("Retrying...");
      delay(MQTT_RETRY_WAIT);
    }
  }

  Serial.println("MQTT connected");
  Serial.println();

  Serial.print("\nIPv4 of MQTT-Broker: ");
  Serial.println(MQTT_BROKER_IP);

  client.setCallback(callbackRoutine); //callbackRoutine definieren, d.h. wenn Broker die Message erhalten hat und eine Antwort zurückgibt, dann wird diese Methode aufgerufen
  client.subscribe(baseTopic); //hier wird ein Topic im MQTT-Broker subscribet, das asugewählte Topic wird dann in derCallbackRoutiner übergeben
  
}

//die Payload/Message generieren aus den Sensordaten
void createPayload(){
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time, skipping");
    return;
  }
  int objectTemp = mlx.readObjectTempC(); //Objekttemperatur als int lesen
  int ambientTemp = mlx.readAmbientTempC(); //Umgebungstemperatur
  snprintf(messageBuffer, 512, "{\"IRSensor\":{\"Besitzer\":\"Kaan\",\"Objekt-Temperatur\":%d, \"Umgebungstemperatur\":%d}}",objectTemp,ambientTemp);
  }

void setup() {
  Serial.begin(9600);
  Wire.begin(); // I2C-Kommunikation initialisieren
  if (!mlx.begin()) {
    Serial.println("Error connecting to MLX sensor. Check wiring.");
    while (1);
  }
  Serial.println("Connected to MLX90614!");
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  connectWifi();
  connectMqtt();
  Serial.println("Initialisiert");
}

void loop() {
  client.loop();
  createPayload();
  client.publish(baseTopic,messageBuffer);
  delay(2000); 
}