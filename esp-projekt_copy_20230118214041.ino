#include <OneWire.h>
#include <DallasTemperature.h>

//#define STASSID "AWVYVOJ"
//#define STAPSK "GpyAMDf6"

#define STASSID "Telekom-163107"
#define STAPSK "615c3m6791xx"
#define DEBUG 1

#define BROKER "test.mosquitto.org"
#define BROKER_PORT 1883

#if DEBUG == 1
#define PRINT(x) Serial.print(x)
#define PRINTLN(x) Serial.println(x)
#define PRINTF(...) Serial.printf(__VA_ARGS__)
#else
#define PRINT(x)
#define PRINTLN(x)
#define PRINTF(...)
#endif

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <pins_arduino.h>

OneWire ow(D2);
DallasTemperature db(&ow);

// WiFi
const char *ssid = STASSID;     // Vložím názov svojej WiFi
const char *password = STAPSK;  // Vložím heslo na WiFi

// MQTT Broker
const char *mqtt_broker = BROKER;
const int mqtt_port = BROKER_PORT;
String temperature_topic = "/teplota";
String led_topic = "/led";
String node_name = "MOJEESP123456789-6";

String led_topic_fin;
String temp_topic_fin;

WiFiClient espClient;
PubSubClient client(espClient);

unsigned long previousMillis = 0;  // Ukladanie poslednej publikovanej teploty
const long interval = 120000;

void callback(char *topic, byte *payload, unsigned int length);
void reconnect(void);
void HandleMessage(String topic, String message);
int cntr = 0;
int connect_cntr = 0;

void setup()
{
  db.begin();

  // Potrebujem nastaviť software serial baud na 115200;
  Serial.begin(115200);
  // Pripojenie na WiFi
  WiFi.begin(ssid, password);
  connect_cntr = 0;
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    PRINTLN("Connecting to WiFi..");
    connect_cntr++;
    if(connect_cntr > 50)
    {
      //ESP.deepSleep(60e6); //na minutu sa uspi, ak sa 50x nepodari pripojit k WiFi
    }
  }
  Serial.println("Connected to the WiFi network");
  // Pripojenie na mqtt broker
  client.setServer(mqtt_broker, mqtt_port);
  client.setCallback(callback);

  pinMode(LED_BUILTIN, OUTPUT);

  connect_cntr = 0;
  while (!client.connected())
  {
    //String client_id = "Teplomer ";
    String client_id = node_name + " ";
    client_id += String(WiFi.macAddress());
    //node_name = client_id;
    PRINTF("The client %s connects to the public mqtt broker\r\n", client_id.c_str());
    client.setKeepAlive(90);
    if (client.connect(client_id.c_str()))
    {
      PRINTLN("Broker connected");
      break;
    }
    else
    {
      PRINT("failed with state ");
      PRINT(client.state());
      delay(2000);
    }
    /*connect_cntr++;
    if (connect_cntr > 10)
    {
      //ESP.deepSleep(60e6);  //na minutu sa uspi, ak sa mu nedari pripojit k brokeru
    }*/
  }

  led_topic_fin = node_name + led_topic;
  temp_topic_fin = node_name + temperature_topic;
  // publish and subscribe
  client.publish(temp_topic_fin.c_str(), NULL, false);
  client.subscribe(led_topic_fin.c_str(), 0);
}

void loop()
{
  if (!client.connected())
    reconnect();

  client.loop();

  //if (cntr > 10)
    //ESP.deepSleep(60e6);  //Uspi sa na 1 minutu

  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval)
  {
    // Save the last time a new reading was published
    previousMillis = currentMillis;
    float temp;
    db.requestTemperatures();
    temp = db.getTempCByIndex(0);
    String msg(temp);

    if(client.publish(temp_topic_fin.c_str(), msg.c_str(), false))
    {
      PRINTF("Publishuju topic: %s, zprava: %s\r\n",temp_topic_fin.c_str(),msg.c_str());
    }
    else
    {
      PRINTLN("Selhalo poslani zpravy...");
    }
    
    cntr++;
  }
}

void HandleMessage(String topic, String message)
{
  PRINTLN("Received message...");
  if (topic.indexOf(node_name) >= 0)
  {
    if (topic.indexOf("led") > 0)
    {
      if (message == "LED ON")
      {
        digitalWrite(BUILTIN_LED, 0);
      }
      else if (message == "LED OFF")
      {
        digitalWrite(BUILTIN_LED, 1);
      }
      else if (message == "TOGGLE")
      {
        digitalWrite(BUILTIN_LED, !digitalRead(BUILTIN_LED));
        PRINTLN("LED toggle");
      }
    }
  }
  else
  {
    PRINTLN("Topic sa netyka tohoto uzlu...");
  }
}

void callback(char *topic, byte *payload, unsigned int length)
{
  String tpc((const char *)topic);
  String msg;
  char buff[256];
  if (length < 256)
  {
    memcpy((void *)buff, (const void *)payload, length);
    buff[length] = 0x00;
    msg = String((const char *)buff);
  }
  if (msg.length())
    HandleMessage(tpc, msg);

  PRINTF("Message received...Topic: %s, Message: %s\r\n", tpc.c_str(), msg.c_str());
}

void reconnect(void)
{
  //String client_id = "Teplomer ";
  String client_id = node_name + " ";
  client_id += String(WiFi.macAddress());
  //node_name = client_id;
  PRINTF("The client %s connects to the mqtt broker\r\n", client_id.c_str());
  connect_cntr = 0;
  while (true)
  {
    if (client.connect(client_id.c_str()))
    {
      PRINTLN("Broker connected");
      break;
    }
    else
    {
      PRINT("failed with state ");
      PRINT(client.state());
      PRINTLN();
      delay(2000);
    }

    connect_cntr++;
    if (connect_cntr > 10)
    {
      //ESP.deepSleep(60e6);
    }
  }

  led_topic_fin = node_name + led_topic;
  temp_topic_fin = node_name + temperature_topic;
  // publish and subscribe
  client.publish(temp_topic_fin.c_str(), "");
  client.subscribe(led_topic_fin.c_str());
}