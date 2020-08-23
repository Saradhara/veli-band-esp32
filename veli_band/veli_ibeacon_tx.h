#include "BLEBeacon.h"
#define BEACON_UUID  "8ec76ea3-6668-48da-9866-75be8bc86f4d" // UUID 1 128-Bit (may use linux tool uuidgen or random numbers via https://www.uuidgenerator.net/)
#define TX_POWER -57 // measured RSSI at 1m distance
BLEAdvertising *pAdvertising;

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
