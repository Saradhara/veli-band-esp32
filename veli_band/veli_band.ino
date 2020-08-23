/*
 * Author : Vishnu Saradhara 
 * Date   : 04/08/2020
*/
#include "sys/time.h"
#include "BLEDevice.h"
#include "BLEUtils.h"
#include "BLEScan.h"
#include "BLEAdvertisedDevice.h"
#include "BLEBeacon.h"
#include "BLEEddystoneTLM.h"
#include "BLEEddystoneURL.h"
#include "Arduino.h"
#include "WiFi.h"
#include "HTTPClient.h"
#include "veli_utils.h"

#define WIFI_NETWORK "braver"
#define WIFI_PASSWORD "vishnu@392"
#define BEACON_UUID  "8ec76ea3-6668-48da-9866-75be8bc86f4d" // UUID 1 128-Bit (may use linux tool uuidgen or random numbers via https://www.uuidgenerator.net/)
#define SERVER_END_POINT  "http://192.168.1.11:8888/trace"
#define WIFI_TIMEOUT_MS 5000 // 5 second WiFi connection timeout, don't change this parameter 
#define WIFI_RECOVER_TIME_MS 10000 // Wait 10 seconds after a failed connection attempt
#define TX_POWER -57 // measured RSSI at 1m distance
#define THRESHOLD -64 // cut off

int LED_BUILTIN = 2;
int LED_BUILTIN_3 = 3;
int scanTime = 1; //In seconds

BLEAdvertising *pAdvertising;
BLEScan* pBLEScan;
/* structure that hold data*/
typedef struct{
  String uuid;
  float distance;
  String risk;
}Trace;

/* this variable hold queue handle */
xQueueHandle xQueue;

//uint16_t beconUUID = 0xFEAA;
#define ENDIAN_CHANGE_U16(x) ((((x)&0xFF00)>>8) + (((x)&0xFF)<<8))

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

void setBeacon() {
  BLEBeacon oBeacon = BLEBeacon();
  oBeacon.setManufacturerId(0x4C00); // fake Apple 0x004C LSB (ENDIAN_CHANGE_U16!)
  oBeacon.setProximityUUID(BLEUUID(BEACON_UUID));
  oBeacon.setMajor(0x8153);
  oBeacon.setMinor(0x0);
  oBeacon.setSignalPower(TX_POWER);
  BLEAdvertisementData oAdvertisementData = BLEAdvertisementData();
  BLEAdvertisementData oScanResponseData = BLEAdvertisementData();  
  oAdvertisementData.setFlags(0x04); // BR_EDR_NOT_SUPPORTED 0x04
  std::string strServiceData = "";
  strServiceData += (char)26;     // Len
  strServiceData += (char)0xFF;   // Type
  strServiceData += oBeacon.getData(); 
  oAdvertisementData.addData(strServiceData);
  pAdvertising->setAdvertisementData(oAdvertisementData);
  pAdvertising->setScanResponseData(oScanResponseData);
}

class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
    /* keep the status of sending data */
    BaseType_t xStatus;
    /* time to block the task until the queue has free space */
    const TickType_t xTicksToWait = pdMS_TO_TICKS(100);
    /* create data to send */
    Trace data;
    void onResult(BLEAdvertisedDevice advertisedDevice) {
      std::string strServiceData = advertisedDevice.getServiceData();
       uint8_t cServiceData[100];
       strServiceData.copy((char *)cServiceData, strServiceData.length(), 0);
        if (advertisedDevice.haveManufacturerData()==true) {
          std::string strManufacturerData = advertisedDevice.getManufacturerData();
          
          uint8_t cManufacturerData[100];
          strManufacturerData.copy((char *)cManufacturerData, strManufacturerData.length(), 0);
          
          if (strManufacturerData.length()==25 && cManufacturerData[0] == 0x4C  && cManufacturerData[1] == 0x00 ) {
            BLEBeacon oBeacon = BLEBeacon();
            oBeacon.setData(strManufacturerData);
            Serial.printf("iBeacon Frame =>");
            Serial.printf("ID: %04X Major: %d Minor: %d UUID: %s Power: %d RSSI: %d\n",oBeacon.getManufacturerId(),ENDIAN_CHANGE_U16(oBeacon.getMajor()),ENDIAN_CHANGE_U16(oBeacon.getMinor()),oBeacon.getProximityUUID().toString().c_str(),oBeacon.getSignalPower(),advertisedDevice.getRSSI());
            int rssi = advertisedDevice.getRSSI();
            int tx_power = oBeacon.getSignalPower();
            if(rssi >= THRESHOLD){
                float distance = calculate_distance(tx_power, rssi);
                String uuid = oBeacon.getProximityUUID().toString().c_str();
                String covid_risk = "";
                if(distance<=1){
                  covid_risk = "HIGH RISK";
                  //buzzer code place here
                   digitalWrite(LED_BUILTIN, HIGH);
                  vTaskDelay(100 / portTICK_PERIOD_MS);
                  digitalWrite(LED_BUILTIN, LOW);
                  vTaskDelay(100 / portTICK_PERIOD_MS);
                  digitalWrite(LED_BUILTIN, HIGH);
                  vTaskDelay(100 / portTICK_PERIOD_MS);
                  digitalWrite(LED_BUILTIN, LOW);
                }else if(distance<=2){
                  covid_risk = "MEDIUM RISK";
                  //buzzer code place here
                  digitalWrite(LED_BUILTIN, HIGH);
                  vTaskDelay(100 / portTICK_PERIOD_MS);
                  digitalWrite(LED_BUILTIN, LOW);
                  vTaskDelay(100 / portTICK_PERIOD_MS);
                  digitalWrite(LED_BUILTIN, HIGH);
                  vTaskDelay(100 / portTICK_PERIOD_MS);
                  digitalWrite(LED_BUILTIN, LOW);
                }else{
                  covid_risk = "LOW RISK";
                  digitalWrite(LED_BUILTIN, HIGH);
                  vTaskDelay(100 / portTICK_PERIOD_MS);
                  digitalWrite(LED_BUILTIN, LOW);
                }
                data.distance = distance;
                data.uuid = uuid;
                data.risk = covid_risk;
                Serial.printf("distance violated by: %s, d=%fm \n",oBeacon.getProximityUUID().toString().c_str(), distance);
                xStatus = xQueueSendToFront( xQueue, &data, xTicksToWait );
                if( xStatus != pdPASS ) {
                  Serial.printf("Queue is full !!");
                 }
              }
          }
         }
    }
};

void startBLEScan(void * parameter){
    for(;;){
        BLEScanResults foundDevices = pBLEScan->start(scanTime,false);
      }
    vTaskDelete( NULL );   
   }

void setup() {
  Serial.begin(115200);
  /* create the queue which size can contains 5 elements of Data */
  xQueue = xQueueCreate(20, sizeof(Trace));
//  WiFi.mode(WIFI_STA);
//  WiFi.begin(WIFI_NETWORK, WIFI_PASSWORD);
  delay(5000);
  xTaskCreatePinnedToCore(
  keepWiFiAlive,
  "keepWiFiAlive",  // Task name
  10000,             // Stack size (bytes)
  NULL,             // Parameter
  1,                // Task priority
  NULL,             // Task handle
  0
  );
  xTaskCreatePinnedToCore(
  pushData,
  "pushData",  // Task name
  10000,             // Stack size (bytes)
  NULL,             // Parameter
  1,                // Task priority
  NULL,             // Task handle
  0
  );
  Serial.println("Scanning...");
  pinMode (LED_BUILTIN, OUTPUT);
  BLEDevice::init("");
  BLEDevice::setPower(ESP_PWR_LVL_P9);
  // Changing had no affect. Client reports power level = 3
  esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_DEFAULT, ESP_PWR_LVL_P9);
  esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_CONN_HDL0, ESP_PWR_LVL_P9);
  esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_CONN_HDL1, ESP_PWR_LVL_P9);
  esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_CONN_HDL2, ESP_PWR_LVL_P9);
  esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_CONN_HDL3, ESP_PWR_LVL_P9);
  esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_CONN_HDL4, ESP_PWR_LVL_P9);
  esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_CONN_HDL5, ESP_PWR_LVL_P9);
  esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_CONN_HDL6, ESP_PWR_LVL_P9);
  esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_CONN_HDL8, ESP_PWR_LVL_P9);
  esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_ADV, ESP_PWR_LVL_P9);

  pAdvertising = BLEDevice::getAdvertising();
  setBeacon();
  pAdvertising->start();
  Serial.println("Advertizing started...");
  pBLEScan = BLEDevice::getScan(); //create new scan
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true); //active scan uses more power, but get results faster
 xTaskCreatePinnedToCore(
  startBLEScan,
  "startBLEScan",  // Task name
  10000,             // Stack size (bytes)
  NULL,             // Parameter
  7,                // Task priority
  NULL,             // Task handle
  1
  );
}
void loop() {
}
