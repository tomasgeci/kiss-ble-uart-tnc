#include "BluetoothSerial.h"
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

#define SERVICE_UUID "00000001-ba2a-46c9-ae49-01b0961f68bb"
#define TX_UUID "00000002-ba2a-46c9-ae49-01b0961f68bb" // From the perspective of the BLE app
#define RX_UUID "00000003-ba2a-46c9-ae49-01b0961f68bb" // From the perspective of the BLE app

const size_t RX_BUF_SIZE = 20; // Max bytes for BLE

BluetoothSerial SerialBT;

BLECharacteristic *pTx;
BLECharacteristic *pRx;
bool bleDeviceConnected = false;
bool btcDeviceConnected = false;
String deviceName = "TH-D74";

class MyServerCallbacks : public BLEServerCallbacks
{
  void onConnect(BLEServer *pServer)
  {
    Serial.println("BLE device connected");
    bleDeviceConnected = true;
  };
  void onDisconnect(BLEServer *pServer)
  {
    Serial.println("BLE device disconnected");
    bleDeviceConnected = false;
  }
};

class MyCallbacks : public BLECharacteristicCallbacks
{
  void onRead(BLECharacteristic *pCharacteristic)
  {
    Serial.println("Received read data on BLE");
  }

  void onWrite(BLECharacteristic *pCharacteristic)
  {
    std::string txValue = pCharacteristic->getValue();
    if (txValue.length() > 0)
    {
      Serial.println("Received data on BLE, sending to TNC");
      SerialBT.write(pCharacteristic->getData(), txValue.length());
      SerialBT.flush();
    }
  }
};

void on_classic_bluetooth_events(esp_spp_cb_event_t event, esp_spp_cb_param_t *param)
{
  Serial.println("BT event");

  if (event == ESP_SPP_SRV_OPEN_EVT)
  {
    Serial.println("BT Client connected");
  }

  if (event == ESP_SPP_CLOSE_EVT)
  {
    Serial.println("BT Client disconnected");
  }
}

void setup()
{
  Serial.begin(115200);

  /* 
    BT Classic
  */
  SerialBT.register_callback(on_classic_bluetooth_events);

  SerialBT.enableSSP();

  SerialBT.begin("TNC Blues", true); //Bluetooth device name

  Serial.println("Trying to pair TNC via bluetooth classic");
  btcDeviceConnected = SerialBT.connect(deviceName);

  if (btcDeviceConnected)
  {
    Serial.println("Bluetooth connected");
  }
  else
  {
    while (!SerialBT.connected(10000))
    {
      Serial.println("Failed to connect. Make sure remote device is available and in range, then restart app.");
    }
  }

  SerialBT.connect();

  BLEDevice::init("TNC Blues");

  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  BLEService *pService = pServer->createService(SERVICE_UUID);

  pTx = pService->createCharacteristic(
      TX_UUID,
      BLECharacteristic::PROPERTY_WRITE_NR);

  pRx = pService->createCharacteristic(
      RX_UUID,
      BLECharacteristic::PROPERTY_NOTIFY);

  pRx->addDescriptor(new BLE2902());

  pRx->setAccessPermissions(ESP_GATT_PERM_READ);
  pTx->setAccessPermissions(ESP_GATT_PERM_WRITE);

  pTx->setCallbacks(new MyCallbacks());
  pRx->setCallbacks(new MyCallbacks());

  pService->start();

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06); // functions that help with iPhone connections issue
  //pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();
}

void loop()
{
  if (SerialBT.connected())
  {
    if (bleDeviceConnected)
    {
      uint8_t rxBuf[RX_BUF_SIZE];
      size_t rxLen = 0;

      // Data available from the TNC, send to BLE
      while (SerialBT.available() && rxLen < RX_BUF_SIZE)
      {
        rxBuf[rxLen++] = SerialBT.read();
      }
      if (rxLen > 0)
      {
        pRx->setValue(rxBuf, rxLen);
        pRx->notify();
      }
    }
  }
  else
  {
    Serial.println("Trying to reconnect bluetooth device");
    SerialBT.connect();    
  }
  delay(20);
}
