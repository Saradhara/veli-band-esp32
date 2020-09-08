/*
 *Author : Vishnu Saradhara 
 *Date   : 08/08/2020
 */
#include "BLEBeacon.h"
#include "BLEDevice.h"
#include "BLEUtils.h"
#include "BLEScan.h"
#include "BLEAdvertisedDevice.h"

int LED_RED = 14;
int LED_GREEN = 5;
int LED_BLUE = 13;
int Motor = 26;

#define ENDIAN_CHANGE_U16(x)((((x) &0xFF00) >> 8) + (((x) &0xFF) << 8))
#define TX_POWER -70 // measured RSSI at 1m distance
#define THRESHOLD -90  // cut off
int scanTime = 1; //In seconds
BLEScan * pBLEScan;
BLEAdvertising * pAdvertising;
/*iBeacon Transmitter called from setup*/
void setBeacon()
{
  BLEBeacon oBeacon = BLEBeacon();
  oBeacon.setManufacturerId(0x4C00);  // fake Apple 0x004C LSB (ENDIAN_CHANGE_U16!)
  Serial.printf("Setting UUID: %s/n",self_uuid);
  oBeacon.setProximityUUID(BLEUUID(self_uuid));
  oBeacon.setMajor(0x8153);
  oBeacon.setMinor(0x0);
  oBeacon.setSignalPower(TX_POWER);
  BLEAdvertisementData oAdvertisementData = BLEAdvertisementData();
  BLEAdvertisementData oScanResponseData = BLEAdvertisementData();
  oAdvertisementData.setFlags(0x04);  // BR_EDR_NOT_SUPPORTED 0x04
  std::string strServiceData = "";
  strServiceData += (char) 26;  // Len
  strServiceData += (char) 0xFF;  // Type
  strServiceData += oBeacon.getData();
  oAdvertisementData.addData(strServiceData);
  pAdvertising->setAdvertisementData(oAdvertisementData);
  pAdvertising->setScanResponseData(oScanResponseData);
}

/*iBeacon Scanner funcation called by startBLEScan Task*/
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks
{
  /*keep the status of sending data */
  BaseType_t xStatus;
  /*time to block the task until the queue has free space */
  const TickType_t xTicksToWait = pdMS_TO_TICKS(100);
  /*create data to send */
  Trace data;
  void onResult(BLEAdvertisedDevice advertisedDevice)
  {
    std::string strServiceData = advertisedDevice.getServiceData();
    uint8_t cServiceData[100];
    strServiceData.copy((char*) cServiceData, strServiceData.length(), 0);
    if (advertisedDevice.haveManufacturerData() == true)
    {
      std::string strManufacturerData = advertisedDevice.getManufacturerData();

      uint8_t cManufacturerData[100];
      strManufacturerData.copy((char*) cManufacturerData, strManufacturerData.length(), 0);

      if (strManufacturerData.length() == 25 && cManufacturerData[0] == 0x4C && cManufacturerData[1] == 0x00)
      {
        BLEBeacon oBeacon = BLEBeacon();
        oBeacon.setData(strManufacturerData);
        Serial.printf("iBeacon Frame =>");
        Serial.printf("ID: %04X Major: %d Minor: %d UUID: %s Power: %d RSSI: %d\n", oBeacon.getManufacturerId(), ENDIAN_CHANGE_U16(oBeacon.getMajor()), ENDIAN_CHANGE_U16(oBeacon.getMinor()), oBeacon.getProximityUUID().toString().c_str(), oBeacon.getSignalPower(), advertisedDevice.getRSSI());
        int rssi = advertisedDevice.getRSSI();
        int tx_power = oBeacon.getSignalPower();
        if (rssi >= THRESHOLD)
        {
          float distance = calculate_distance(tx_power, rssi);
          String uuid = oBeacon.getProximityUUID().toString().c_str();
          String covid_risk = "";
          if (distance <= 1)
          {
            covid_risk = "HIGH_RISK";
            //buzzer code place here
            digitalWrite(LED_RED, HIGH);
            digitalWrite(Motor, HIGH);
            ledcWriteTone(0, 4005);
            vTaskDelay(100 / portTICK_PERIOD_MS);
            digitalWrite(LED_RED, LOW);
            digitalWrite(Motor, LOW);
            ledcWriteTone(0, 0);
          }
          else if (distance <= 2)
          {
            covid_risk = "MEDIUM_RISK";
            //buzzer code place here
            digitalWrite(LED_BLUE, HIGH);
            digitalWrite(Motor, HIGH);
            ledcWriteTone(0, 4005);
            vTaskDelay(100 / portTICK_PERIOD_MS);
            digitalWrite(LED_BLUE, LOW);
            digitalWrite(Motor, LOW);
            ledcWriteTone(0, 0);
          }
          else
          {
            covid_risk = "LOW_RISK";
            digitalWrite(LED_GREEN, HIGH);
            digitalWrite(Motor, HIGH);
//            ledcWriteTone(0, 4005);
            vTaskDelay(100 / portTICK_PERIOD_MS);
            digitalWrite(LED_GREEN, LOW);
            digitalWrite(Motor, LOW);
//            ledcWriteTone(0, 0);
          }

          data.distance = distance;
          data.uuid = uuid;
          data.risk = covid_risk;
          Serial.printf("distance violated by: %s, d=%fm \n", oBeacon.getProximityUUID().toString().c_str(), distance);
          xStatus = xQueueSendToFront(xQueue, &data, xTicksToWait);
          if (xStatus != pdPASS)
          {
            Serial.printf("Queue is full !!");
          }
        }
      }
    }
  }
};
/*iBeacon Scanner Task*/
void startBLEScan(void *parameter)
{
  for (;;)
  {
    BLEScanResults foundDevices = pBLEScan->start(scanTime, false);
  }

  vTaskDelete(NULL);
};
