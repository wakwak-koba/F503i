/*
 * https://twitter.com/wakwak_koba/
 */
#include "F503i.h"

static const char *TAG = "F503i";
static const NimBLEUUID UUID_serviceuuid("f7fce500-7a0b-4b89-a675-a79137223e2c");
static const NimBLEUUID UUID_service    ("f7fce510-7a0b-4b89-a675-a79137223e2c");
static const NimBLEUUID UUID_led0       ("f7fce517-7a0b-4b89-a675-a79137223e2c");
static const NimBLEUUID UUID_led1       ("f7fce518-7a0b-4b89-a675-a79137223e2c");
static const NimBLEUUID UUID_led2       ("f7fce51b-7a0b-4b89-a675-a79137223e2c");
static const NimBLEUUID UUID_buzzer     ("f7fce521-7a0b-4b89-a675-a79137223e2c");
static const NimBLEUUID UUID_key        ("f7fce531-7a0b-4b89-a675-a79137223e2c");
static const NimBLEUUID UUID_cds        ("f7fce532-7a0b-4b89-a675-a79137223e2c");

std::function<void(F503i*, const char key)> F503i :: onKey = nullptr;
std::function<void(F503i*)>                 F503i :: onConnect = nullptr;
std::function<void(F503i*, int)>            F503i :: onDisconnect = nullptr;
NimBLEScan*                                 F503i :: bleScan = nullptr;
F503i :: AdvertisedDeviceCallbacks*         F503i :: advertisedDeviceCallbacks  = nullptr;
std::map<NimBLEAddress, F503i*>             F503i :: bleClients;
QueueHandle_t                               F503i :: qhTask = nullptr;
F503i :: ClientCallbacks                    F503i :: clientCallbacks;

// static member
void F503i :: setup_NimBLE() {
  if(advertisedDeviceCallbacks)
    return;

  advertisedDeviceCallbacks = new AdvertisedDeviceCallbacks();
  NimBLEDevice::init("");
  bleScan = NimBLEDevice::getScan();
  bleScan->setScanCallbacks(advertisedDeviceCallbacks);
  bleScan->setActiveScan(true);
  bleScan->start(0);
}

// static member
F503i* F503i :: connect(const NimBLEAdvertisedDevice* advertisedDevice) {
  if(!advertisedDevice->haveServiceUUID() || !advertisedDevice->isAdvertisingService(UUID_serviceuuid))
    return nullptr;

  auto pAddress = advertisedDevice->getAddress();
  if(!bleClients.count(pAddress))
    bleClients[pAddress] = new F503i(advertisedDevice);
  else
    bleClients[pAddress]->advertisedDevice = advertisedDevice;
  return bleClients[pAddress];
}

// static member
bool F503i :: handle() {
  if(!qhTask)
    qhTask = xQueueCreate(10, sizeof(TaskQueue *));
  if(!bleScan)
    bleScan = NimBLEDevice::getScan();

  bool result = false;
  if(!bleScan->isScanning()) {
    for(auto pClient : bleClients) {
      auto device = pClient.second;
      if(device && !device->isConnected()) {
        device->connect();
        result = true;
      }
    }
  }

  if(advertisedDeviceCallbacks && !bleScan->isScanning())
    bleScan->start(0);

  TaskQueue* task;
  if(xQueueReceive( qhTask, &task, 0 ) == pdPASS) {
    if(onKey) {
      char key = 0;
      switch(*(uint16_t *)task->pData) {
        case 0x0ffd:
          key = '1';
          break;
        case 0x0ffb:
          key = '2';
          break;
        case 0x0ff7:
          key = '3';
          break;
        case 0x0fef:
          key = '4';
          break;
        case 0x0fdf:
          key = '5';
          break;
        case 0x0fbf:
          key = '6';
          break;
        case 0x0f7f:
          key = '7';
          break;
        case 0x0eff:
          key = '8';
          break;
        case 0x0dff:
          key = '9';
          break;
        case 0x0bff:
          key = '*';
          break;
        case 0x0ffe:
          key = '0';
          break;
        case 0x07ff:
          key = '#';
          break;
        case 0x0fff:
          break;
      }
      onKey(task->device, key);
    }
    delete task;
  }

  return result;
}

// static member
std::vector<F503i *> F503i :: getConnected() {
  std::vector<F503i *> result;
  for(auto bleClient : bleClients)
    if(bleClient.second->isConnected())
      result.push_back(bleClient.second);
  return result;
}

// static member
F503i* F503i :: getDevice(NimBLEAddress bleAddress) {
  return bleClients[bleAddress];
}

F503i :: F503i(const NimBLEAdvertisedDevice* advertisedDevice) {
  this->advertisedDevice = advertisedDevice;
}

const bool F503i :: connect() {
  if(NimBLEDevice::getCreatedClientCount()) {
    bleClient = NimBLEDevice::getClientByPeerAddress(advertisedDevice->getAddress());
    if(!bleClient)
      bleClient = NimBLEDevice::getDisconnectedClient();
    else if(bleClient && !bleClient->connect(advertisedDevice, false))
      return false;
  }

  if(!bleClient) {
    if(NimBLEDevice::getCreatedClientCount() >= NIMBLE_MAX_CONNECTIONS)
      return false;

    bleClient = NimBLEDevice::createClient();
    if(!bleClient)
      return false;
    bleClient->setClientCallbacks(&clientCallbacks, false);
    bleClient->setConnectTimeout(5000);
    if (!bleClient->connect(advertisedDevice)) {
      NimBLEDevice::deleteClient(bleClient);
      return false;
    }
  }

  if(!bleClient->isConnected() && !bleClient->connect(advertisedDevice))
    return false;

  auto onKeyHandle = [&bleClients, &qhTask](NimBLERemoteCharacteristic* pRemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify) {
    auto parameter = new TaskQueue(getDevice(pRemoteCharacteristic->getClient()->getPeerAddress()), pData, length);
    xQueueSend( qhTask, &parameter, 0 );
    return;
  };

  bleClient->getServices(true);
  bleService = bleClient->getService(UUID_service);
  bleService->getCharacteristics(true);
  bleService->getCharacteristic(UUID_key)->subscribe(false, onKeyHandle);

  if(onConnect)
    onConnect(this);
  return true;
}

bool F503i :: isConnected() {
  return bleClient && bleClient->isConnected();
}

void F503i :: setLed(const uint8_t no, const uint8_t brightness) { 
  if(!bleService)
    return;

  auto value = NimBLEAttValue(&brightness, 1);
  switch(no) {
    case 0:
      bleService->setValue(UUID_led0, value);
      break;
    case 1:
      bleService->setValue(UUID_led1, value);
      break;
    case 2:
      bleService->setValue(UUID_led2, value);
      break;
  }
}

void F503i :: setBuzzer(const uint8_t tone) {
  if(!bleService)
    return;
    
  bleService->setValue(UUID_buzzer, NimBLEAttValue(&tone, 1));
}

uint16_t F503i :: getIlluminance() {
  if(!bleService)
    return 0x0000;
    
  auto value = bleService->getValue(UUID_cds);
  return *(uint16_t *)value.data();
}

NimBLEAddress F503i :: getAddress() {
  return bleClient->getPeerAddress();
}