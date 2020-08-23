/*
 *Author : Vishnu Saradhara 
 *Date   : 21/08/2020
 */
#include "WiFi.h"
#include "HTTPClient.h"
#include "PubSubClient.h"
#include "ArduinoJson.h"
#define WIFI_NETWORK "braverpixel"
#define WIFI_PASSWORD "vishnu@392"
#define SERVER_END_POINT "http://192.168.43.251:8888/trace"
#define WIFI_TIMEOUT_MS 3000  // 5 second WiFi connection timeout, don't change this parameter 
#define WIFI_RECOVER_TIME_MS 30000  // Wait 10 seconds after a failed connection attempt

const char *mqttServer = "192.168.43.251";
const int mqttPort = 1883;
const char *clientID = "";
const char *channelName = "hello_world";

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

void pushData(void *parameter)
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
        doc["mac"] = mac;
        doc["ip"] = ip;
        doc["battery"] = 33;
        char json_buffer[100];
        size_t n = serializeJson(doc, json_buffer);
        Serial.println("Sending message to MQTT topic..");
        Serial.println(json_buffer);
        client.publish(channelName, json_buffer, n);  // Publish message.
        vTaskDelay(10000 / portTICK_PERIOD_MS);
      }
    }
  }
}
