/*
 * Author : Vishnu Saradhara 
 * Date   : 04/08/2020
*/
#include "sys/time.h"
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include "BLEBeacon.h"
#include "BLEEddystoneTLM.h"
#include "BLEEddystoneURL.h"

#define BEACON_UUID  "8ec76ea3-6668-48da-9866-75be8bc86f4d" // UUID 1 128-Bit (may use linux tool uuidgen or random numbers via https://www.uuidgenerator.net/)
int LED_BUILTIN = 2;
int scanTime = 1; //In seconds

BLEAdvertising *pAdvertising;
BLEScan* pBLEScan;

//uint16_t beconUUID = 0xFEAA;
#define ENDIAN_CHANGE_U16(x) ((((x)&0xFF00)>>8) + (((x)&0xFF)<<8))

float power(float x, int y) 
{ 
    float temp; 
    if( y == 0) 
       return 1; 
    temp = power(x, y/2);        
    if (y%2 == 0) 
        return temp*temp; 
    else
    { 
        if(y > 0) 
            return x*temp*temp; 
        else
            return (temp*temp)/x; 
    } 
}
   
float calculate_distance (int tx_power, int rssi )
{
    float A = 0.950827299;
    float B = 4.61399983;
    float C = 0.06503583;
    if (rssi == 0){
        return -1.0;
    }
    if (tx_power == 0){
        return -1.0;
    }
    float ratio = rssi * 1.0 / tx_power;
    if (ratio < 1.0){
        return power(ratio,10);
    }
    float distance = (A) * (power(ratio,B)) + C;
    return distance;
}

void setBeacon() {
  BLEBeacon oBeacon = BLEBeacon();
  oBeacon.setManufacturerId(0x4C00); // fake Apple 0x004C LSB (ENDIAN_CHANGE_U16!)
  oBeacon.setProximityUUID(BLEUUID(BEACON_UUID));
  oBeacon.setMajor(0x8153);
  oBeacon.setMinor(0x0);
  oBeacon.setSignalPower(-56);
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
            if(rssi >= -75){
                float distance = calculate_distance(tx_power, rssi);
                Serial.printf("distance violated by: %s, d=%fm \n",oBeacon.getProximityUUID().toString().c_str(), distance);
                digitalWrite(LED_BUILTIN, HIGH);
                delay(100);
                digitalWrite(LED_BUILTIN, LOW);
              }
          }
         }
    }
};

void setup() {
  Serial.begin(115200);
  Serial.println("Scanning...");
  pinMode (LED_BUILTIN, OUTPUT);
  
  BLEDevice::init("");
  
  pAdvertising = BLEDevice::getAdvertising();
  setBeacon();
  pAdvertising->start();
  
  Serial.println("Advertizing started...");
  pBLEScan = BLEDevice::getScan(); //create new scan
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true); //active scan uses more power, but get results faster
}

void loop() {
  BLEScanResults foundDevices = pBLEScan->start(scanTime);
  Serial.printf("-------------------\n");
  delay(100);
}
