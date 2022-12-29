#include "BluetoothSerial.h"
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>
#include <TinyPICO.h>

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

#define RADIO_BLUETOOTH_NAME "TH-D74" // Change this to match your radio name
#define ADAPTER_NAME "TNC Blues" // This is the name the adapter will be seen as

/*
  More info about those magic numbers
  https://github.com/hessu/aprs-specs/blob/master/BLE-KISS-API.md
*/
#define SERVICE_UUID "00000001-ba2a-46c9-ae49-01b0961f68bb"
#define TX_UUID "00000002-ba2a-46c9-ae49-01b0961f68bb" // From the perspective of the BLE app
#define RX_UUID "00000003-ba2a-46c9-ae49-01b0961f68bb" // From the perspective of the BLE app

const size_t RX_BUF_SIZE = 256; // Max bytes for BLE is 20. BLE 4.2 supports up to 512. Should experiment with this.

BluetoothSerial btSerial;
TinyPICO tp = TinyPICO();

BLECharacteristic *pTx;
BLECharacteristic *pRx;
bool bleDeviceConnected = false;

void BTConfirmRequestCallback(uint32_t numVal)
{
  Serial.println("Pairing pin");
  Serial.println(numVal);
  btSerial.confirmReply(true);
}

void BTAuthCompleteCallback(boolean success)
{
  if (success)
  {
    Serial.println("Pairing success!!");
  }
  else
  {
    Serial.println("Pairing failed, rejected by user!!");
  }
}

class MyServerCallbacks : public BLEServerCallbacks
{
  void onConnect(BLEServer *pServer)
  {
    Serial.println("BLE device connected");
    bleDeviceConnected = true;
    tp.DotStar_SetPixelColor(0, 0, 255);
  };
  void onDisconnect(BLEServer *pServer)
  {
    Serial.println("BLE device disconnected");
    bleDeviceConnected = false;
    tp.DotStar_SetPixelColor(0, 255, 0);
    startAdvertising();
  }
};

class MyCallbacks : public BLECharacteristicCallbacks
{
  void onRead(BLECharacteristic *pCharacteristic)
  {
    // This should never be called
  }

  // Data was written from the phone to the adapter
  void onWrite(BLECharacteristic *pCharacteristic)
  {
    std::string txValue = pCharacteristic->getValue();
    if (txValue.length() > 0)
    {
      // Received data on BLE, sending to TNC via BTC
      btSerial.write(pCharacteristic->getData(), txValue.length());
      // Does not seem necessary
      //btSerial.flush();
    }
  }
};

void setup()
{
  Serial.begin(115200);
  tp.DotStar_Clear();
  tp.DotStar_SetPixelColor(255, 0, 0);

  /* 
    BT Classic
  */
  btSerial.enableSSP();
  btSerial.onConfirmRequest(BTConfirmRequestCallback);
  btSerial.onAuthComplete(BTAuthCompleteCallback);

  // Bluetooth classic adapter name
  if (!btSerial.begin(ADAPTER_NAME, true))
  {
    Serial.println("BTC init failed.");
    while(true)
    {
      delay(1000);
    }
  }
  tp.DotStar_Clear();
  Serial.println("The adapter started, now you can pair with BTC.");

  // Connect to radio
  if (btSerial.connect(RADIO_BLUETOOTH_NAME))
  {
    tp.DotStar_SetPixelColor(0, 255, 0);
    Serial.println("BTC connected to radio");
  }
  else
  {
    waitForBTCConnect();
  }
  
  /*
   BLE
  */

  // BLE adapter name
  BLEDevice::init(ADAPTER_NAME);

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

  startAdvertising();
}

void startAdvertising()
{
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06); // functions that help with iPhone connections issue
  //pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();  
}

void waitForBTCConnect()
{
  while (!btSerial.connected(10000))
  {
    Serial.println("Failed to connect to BTC device");
    btSerial.connect();
  }
  tp.DotStar_SetPixelColor(0, 255, 0);
  Serial.println("Bluetooth connected");
}

void loop()
{
  if (btSerial.connected())
  {
    if (bleDeviceConnected)
    {
      uint8_t rxBuf[RX_BUF_SIZE];
      size_t rxLen = 0;

      // Data available from the TNC, send to BLE
      while (btSerial.available() && rxLen < RX_BUF_SIZE)
      {
        rxBuf[rxLen++] = btSerial.read();
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
    Serial.println("BTC device not connected");
    tp.DotStar_Clear();
    waitForBTCConnect();
  }
  // Needed otherwise watchdog freaks out
  delay(10);
}