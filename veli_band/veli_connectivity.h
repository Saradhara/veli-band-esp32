#include "WiFi.h"
#include "HTTPClient.h"
#define WIFI_NETWORK "braver"
#define WIFI_PASSWORD "vishnu@392"
#define SERVER_END_POINT  "http://192.168.1.11:8888/trace"
#define WIFI_TIMEOUT_MS 5000 // 5 second WiFi connection timeout, don't change this parameter 
#define WIFI_RECOVER_TIME_MS 10000 // Wait 10 seconds after a failed connection attempt

/* structure that hold data*/
typedef struct{
  String uuid;
  float distance;
  String risk;
}Trace;

/* this variable hold queue handle */
xQueueHandle xQueue;

void keepWiFiAlive(void * parameter){
    for(;;){
        if(WiFi.status() == WL_CONNECTED){
            Serial.println("[WIFI] Connected:");
            Serial.println("IP address: ");
            Serial.println(WiFi.localIP());
            vTaskDelay(10000 / portTICK_PERIOD_MS);
            continue;
        }
        Serial.println("[WIFI] Connecting");
        WiFi.mode(WIFI_STA);
        WiFi.begin(WIFI_NETWORK, WIFI_PASSWORD);
        unsigned long startAttemptTime = millis();
        // Keep looping while we're not connected and haven't reached the timeout
        while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < WIFI_TIMEOUT_MS){}
        // When we couldn't make a WiFi connection (or the timeout expired)
        // sleep for a while and then retry.
        if(WiFi.status() != WL_CONNECTED){
            Serial.println("[WIFI] FAILED");
            vTaskDelay(WIFI_RECOVER_TIME_MS / portTICK_PERIOD_MS);
            continue;
        }
    }
}
void pushData(void * parameter){
    const TickType_t xTicksToWait = pdMS_TO_TICKS(100);
    BaseType_t xStatus;
    Trace data;
    for(;;){
         if(WiFi.status()== WL_CONNECTED){
          HTTPClient http;   
          http.begin(SERVER_END_POINT);
          http.addHeader("Content-Type", "text/plain");
           /* receive data from the queue */
          xStatus = xQueueReceive( xQueue, &data, xTicksToWait );
          String send_data = "";
          while(xStatus == pdPASS){
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
              send_data = send_data + "@@@" +uuid + "#" +String(distance,2)+ "#" + risk;
              xStatus = xQueueReceive( xQueue, &data, xTicksToWait );
            }
          if(send_data != ""){
            int httpResponseCode = http.POST(send_data);
            if(httpResponseCode == 200){
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
