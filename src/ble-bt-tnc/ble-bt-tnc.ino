#include <HardwareSerial.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

#define ADAPTER_NAME "Aprs.fi TNC" // This is the name the adapter will be seen as

/*
  More info about those magic numbers
  https://github.com/hessu/aprs-specs/blob/master/BLE-KISS-API.md
*/
#define SERVICE_UUID "00000001-ba2a-46c9-ae49-01b0961f68bb"
#define TX_UUID "00000002-ba2a-46c9-ae49-01b0961f68bb" // From the perspective of the BLE app
#define RX_UUID "00000003-ba2a-46c9-ae49-01b0961f68bb" // From the perspective of the BLE app

const size_t RX_BUF_SIZE = 256; // Max bytes for BLE is 20. BLE 4.2 supports up to 512. Should experiment with this.

HardwareSerial SerialPort(2); // UART2 - RX-GPIO16, TX-GPIO17

BLECharacteristic *pTx;
BLECharacteristic *pRx;
bool bleDeviceConnected = false;

void startAdvertising()
{
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06); // functions that help with iPhone connections issue
  //pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();  
}

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
    std::string rxValue = pCharacteristic->getValue();
    if (rxValue.length() > 0)
    {
        Serial.println("Received Value from aprs.fi App: ");
        for (int i = 0; i < rxValue.length(); i++){
          Serial.print(rxValue[i]);
        }
        Serial.println("...");
        Serial.println("Send received values via UART");
        SerialPort.write(pCharacteristic->getData(), rxValue.length());
    }
  }
};

void setup()
{
  // setup power saving
  setCpuFrequencyMhz(80); // 80-160-240 MHz - needs testing for the best value

  Serial.begin(115200); // debug
  SerialPort.begin(9600, SERIAL_8N1, 16, 17); // UART TNC
  delay(100);
  Serial.println("Serial port init - UART2 - RX-GPIO16, TX-GPIO17");

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

void loop()
{
    if (bleDeviceConnected)
    {
      uint8_t rxBuf[RX_BUF_SIZE];
      size_t rxLen = 0;

      // Data available from the TNC, send to BLE
      while (SerialPort.available() && rxLen < RX_BUF_SIZE)
      {
        rxBuf[rxLen++] = SerialPort.read();
      }

      if (rxLen > 0)
      {
        Serial.println("Send data received from TNC to aprr.fi App.");
        pRx->setValue(rxBuf, rxLen);
        pRx->notify();
      }
    } else
  {
    Serial.println("BLE device not connected");
    delay(1000); // wait
  }
  // Needed otherwise watchdog freaks out
  delay(10);
}