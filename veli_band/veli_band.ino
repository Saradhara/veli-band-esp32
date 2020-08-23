/*
 * Author : Vishnu Saradhara 
 * Date   : 04/08/2020
*/
#include "sys/time.h"
#include "BLEDevice.h"
#include "BLEUtils.h"
#include "BLEScan.h"
#include "BLEAdvertisedDevice.h"

#include "Arduino.h"
#include "veli_utils.h"
#include "veli_connectivity.h"
#include "veli_ibeacon_tx.h"
#define THRESHOLD -64 // cut off

int LED_BUILTIN = 2;
int LED_BUILTIN_3 = 3;
int scanTime = 1; //In seconds
BLEScan* pBLEScan;

//uint16_t beconUUID = 0xFEAA;
#define ENDIAN_CHANGE_U16(x) ((((x)&0xFF00)>>8) + (((x)&0xFF)<<8))

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
