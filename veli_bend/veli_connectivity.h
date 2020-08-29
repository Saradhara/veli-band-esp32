/*
  Author : Vishnu Saradhara
  Date   : 21/08/2020
*/
#include "WiFi.h"
#include "HTTPClient.h"
#include "PubSubClient.h"
#include "ArduinoJson.h"
#define WIFI_NETWORK "braver"
#define WIFI_PASSWORD "vishnu@392"
#define SERVER_END_POINT "http://192.168.1.11:8888/trace"
#define WIFI_TIMEOUT_MS 3000  // 5 second WiFi connection timeout, don't change this parameter 
#define WIFI_RECOVER_TIME_MS 30000  // Wait 10 seconds after a failed connection attempt

const char *mqttServer = "192.168.1.11";
const int mqttPort = 1883;
const char *clientID = "";
const char *topic_heartbeat = "heartbeat";
const char *topic_trace = "trace";
char *self_uuid;

WiFiClient MQTTclient;
PubSubClient client(MQTTclient);

/*structure that hold data*/
typedef struct
{
  String uuid;
  float distance;
  String risk;
}

Trace;

/*this variable hold queue handle */
xQueueHandle xQueue;
long lastReconnectAttempt = 0;

char *get_uuid(){
  static char uuid[100];
  char *uuid_addr_c = "8ec76ea3-6668-48da-9866-";
  String mac = WiFi.macAddress();
  mac.replace(":", "");
  mac.toLowerCase();
  const char *mac_addr_c = mac.c_str();
  strcpy(uuid,uuid_addr_c);
  strcat(uuid, mac_addr_c);
  Serial.printf("Self UUID: %s\n",uuid);
  return uuid;
}

void keepWiFiAlive(void *parameter)
{
  for (;;)
  {
    if (WiFi.status() == WL_CONNECTED)
    {
      Serial.println("[WIFI] Connected:");
      Serial.print("IP address: ");
      Serial.println(WiFi.localIP());
      vTaskDelay(10000 / portTICK_PERIOD_MS);
      continue;
    }

    Serial.println("[WIFI] Connecting");
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_NETWORK, WIFI_PASSWORD);
    unsigned long startAttemptTime = millis();
    /*Keep looping while we're not connected and haven't reached the timeout*/
    while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < WIFI_TIMEOUT_MS) {}

    /*When we couldn't make a WiFi connection (or the timeout expired), sleep for a while and then retry.*/
    if (WiFi.status() != WL_CONNECTED)
    {
      Serial.println("[WIFI] FAILED");
      vTaskDelay(WIFI_RECOVER_TIME_MS / portTICK_PERIOD_MS);
      continue;
    }
  }
}

void pushDataHTTP(void *parameter)
{
  const TickType_t xTicksToWait = pdMS_TO_TICKS(100);
  BaseType_t xStatus;
  Trace data;
  for (;;)
  {
    if (WiFi.status() == WL_CONNECTED)
    {
      HTTPClient http;
      http.begin(SERVER_END_POINT);
      http.addHeader("Content-Type", "text/plain");
      /*receive data from the queue */
      xStatus = xQueueReceive(xQueue, &data, xTicksToWait);
      String send_data = "";
      while (xStatus == pdPASS)
      {
        String uuid = data.uuid;
        float distance = data.distance;
        String risk = data.risk;
        Serial.print("receiveTask got data: ");
        Serial.print("uuid = ");
        Serial.print(uuid);
        Serial.print(" distance = ");
        Serial.println(distance);
        Serial.print("COVID risk = ");
        Serial.println(risk);
        send_data = send_data + "@@@" + uuid + "#" + String(distance, 2) + "#" + risk;
        xStatus = xQueueReceive(xQueue, &data, xTicksToWait);
      }

      if (send_data != "")
      {
        int httpResponseCode = http.POST(send_data);
        if (httpResponseCode == 200)
        {
          String response = http.getString();
          Serial.print("Response code:");
          Serial.println(httpResponseCode);
        }
        else
        {
          Serial.print("Error on sending POST: ");
          Serial.println(httpResponseCode);
        }

        http.end();
      }
    }
    else
    {
      Serial.println("No wifi connection ....");
    }

    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}

void pushDataMQTT(void *parameter)
{
  const TickType_t xTicksToWait = pdMS_TO_TICKS(100);
  BaseType_t xStatus;
  Trace data;
  for (;;)
  {  
    if (WiFi.status() == WL_CONNECTED && client.connected())
    { 
    xStatus = xQueueReceive(xQueue, &data, xTicksToWait);
    while (xStatus == pdPASS)
    {
      String uuid = data.uuid;
      float distance = data.distance;
      String risk = data.risk;
      Serial.print("receiveTask got data: ");
      Serial.print("uuid = ");
      Serial.print(uuid);
      Serial.print(" distance = ");
      Serial.println(distance);
      Serial.print("COVID risk = ");
      Serial.println(risk);
      
      DynamicJsonDocument trace_json_doc(1024);
      trace_json_doc["partner_uuid"] = uuid;
      trace_json_doc["self_uuid"] = String(self_uuid);
      trace_json_doc["covid_risk"] = risk;
      trace_json_doc["distance"] = distance;
      trace_json_doc["timestamp"] = 11223344;
      char trace_json_buffer[500];
      size_t n = serializeJson(trace_json_doc, trace_json_buffer);
      Serial.println("Sending message to MQTT topic...");
      client.publish(topic_trace, trace_json_buffer, n);  // Publish message.
      xStatus = xQueueReceive(xQueue, &data, xTicksToWait);
    }
    }
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}

void heartBeat(void *parameter)
{
  const TickType_t xTicksToWait = pdMS_TO_TICKS(100);
  for (;;)
  {
    if (WiFi.status() == WL_CONNECTED)
    {
      if (!client.connected())
      {
        Serial.println("MQTT client is not connected ... Retrying ");
        client.connect(clientID);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        continue;
      }
      else
      {
        Serial.println("MQTT client is connected");
        client.loop();
        String mac = WiFi.macAddress();
        String ip = WiFi.localIP().toString();
        mac.replace(":", "");
        DynamicJsonDocument doc(1024);
        doc["self_uuid"] = String(self_uuid);
        doc["ip"] = ip;
        doc["battery"] = 33;
        char json_buffer[100];
        size_t n = serializeJson(doc, json_buffer);
        Serial.println("Sending message to MQTT topic..");
        Serial.println(json_buffer);
        client.publish(topic_heartbeat, json_buffer, n);  // Publish message.
        vTaskDelay(10000 / portTICK_PERIOD_MS);
      }
    }
  }
}
