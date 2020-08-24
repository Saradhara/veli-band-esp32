/*
 *Author : Vishnu Saradhara 
 *Date   : 04/08/2020
 */
#include "sys/time.h"
#include "Arduino.h"
#include "veli_utils.h"
#include "veli_connectivity.h"
#include "veli_ibeacon_tx_rx.h"

void setup()
{
  Serial.begin(115200);
  self_uuid = get_uuid();
  /*create the queue which size can contains 5 elements of Data */
  xQueue = xQueueCreate(20, sizeof(Trace));
  client.setServer(mqttServer, mqttPort); // Connect to PubNub.

  /*Task to WIFI keep Alive*/
  xTaskCreatePinnedToCore(keepWiFiAlive, "keepWiFiAlive", 4096, NULL, 1, NULL, 0);
  /*Task to push data to Backend*/
//  xTaskCreatePinnedToCore(pushDataHTTP, "pushDataHTTP", 4096, NULL, 1, NULL, 0);
  xTaskCreatePinnedToCore(pushDataMQTT, "pushDataMQTT", 4096, NULL, 1, NULL, 0);
  /*Task to send HeartBeat*/
  xTaskCreatePinnedToCore(heartBeat, "heartBeat", 4096, NULL, 0, NULL, 0);

  Serial.println("Scanning...........");
  pinMode(LED_BUILTIN, OUTPUT);
  BLEDevice::init("VeliBand");
  BLEDevice::setPower(ESP_PWR_LVL_P9);

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
  BLEServer *pServer = BLEDevice::createServer();
  pAdvertising = BLEDevice::getAdvertising();
  setBeacon();
  pAdvertising->start();
  Serial.println("Advertizing started...");
  pBLEScan = BLEDevice::getScan();  //create new scan
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true);  //active scan uses more power, but get results faster

  /*Task to Scan iBeacon*/
  xTaskCreatePinnedToCore(startBLEScan, "startBLEScan", 10240, NULL, 7, NULL, 1);
}

void loop()
{
  /*Don't delete this task, will will create the watch dog error*/
  vTaskDelete(NULL);
}
