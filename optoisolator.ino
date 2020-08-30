#include <DHT.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

// WiFi settings
const char* ssid = "YourSSID";
const char* password = "YourPassword";

// MQTT configuration
const char* mqtt_server = "192.168.1.242";
const char* mqttUser = "YourMQTTuser";
const char* mqttPassword = "YourMQTTPassword";
const int mqttPort = 1883;

// Wifi network parameters
IPAddress ip(192,168,1,248);     
IPAddress gateway(192,168,1,1);   
IPAddress subnet(255,255,255,0);   

WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];
int value = 0;

// DHT11 definition & configuration
#define DHTTYPE DHT11
int DHTPin = 4;
DHT dht(DHTPin, DHTTYPE);
float Temperature;
float Humidity;

// Optoisolator definition
int optoisolator = 13;
int bell = 0;

// Wifi setup
void setup_wifi() 
{
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.config(ip, gateway, subnet);
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

void door_state()
{  
  Serial.println("Interrupt");    
  if(digitalRead(optoisolator)==HIGH)
  {
    bell = 1;
    client.publish("house/0/street/bell", "NO");  
  } else 
  {
    bell = 0;
    client.publish("house/0/street/bell", "RINGING");  
  }    
}

// Function to read MQTT Server msg
void callback(char* topic, byte* payload, unsigned int length) 
{
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) 
  {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  // Read temperature or humidity in case NodeMCU recives "7" or "8"
  if ((char)payload[0] == '7') 
  {
    delay(1000);
    Temperature = dht.readTemperature();         
    delay(1000);
    Serial.println(Temperature);
    char TempF[4];
    dtostrf(Temperature, 4, 2, TempF); 
    delay(1000);           
    client.publish("house/0/dinningroom/temp", TempF);           
  }  
  
  if ((char)payload[0] == '8') 
  {
    delay(1000);
    Humidity = dht.readHumidity();     
    delay(1000);
    Serial.println(Humidity);    
    char humF[4];
    dtostrf(Humidity, 4, 2, humF);        
    delay(1000);           
    client.publish("house/0/dinningroom/hum", humF);       
  }
}

// Reconnect WiFi
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str(), mqttUser, mqttPassword)) {
      Serial.println("connected");
      client.subscribe("house/0/menjador/read");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup() 
{    
  Serial.begin(9600);
  setup_wifi();  
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);  
  pinMode (optoisolator, INPUT);
  attachInterrupt(optoisolator, door_state, CHANGE);
}

void loop() 
{
  if (!client.connected()) {
    reconnect();
  }
  client.loop();  
  long now = millis();
}
