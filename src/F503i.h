/*
 * https://twitter.com/wakwak_koba/
 */
#ifndef _WAKWAK_KOBA_F503I_
#define _WAKWAK_KOBA_F503I_

#include <NimBLEDevice.h>
#include <functional>
#include <map>
#include <vector>

class F503i : public Stream {
  public:
    static void setup_NimBLE();
    static F503i* connect(const NimBLEAdvertisedDevice* advertisedDevice);
    static bool handle();
    static std::vector<F503i*> getConnected();

    bool isConnected();
    void setLed(const uint8_t no, const uint8_t brightness);
    void setBuzzer(const uint8_t tone);
    uint16_t getIlluminance();
    NimBLEAddress getAddress();

    /* key: '0'～'9' '*' '#', 0x00:released */
    static std::function<void(F503i*, const char key)> onKey;
    static std::function<void(F503i*)> onConnect;
    static std::function<void(F503i*, int)> onDisconnect;

  protected:
    F503i(const NimBLEAdvertisedDevice* advertisedDevice);
    ~F503i();
    const bool connect();

    static char convertKey(uint16_t key);
    static F503i* getDevice(NimBLEAddress bleAddress);

    static std::map<NimBLEAddress, F503i*> bleClients;
    
    static NimBLEScan* bleScan;

    const NimBLEAdvertisedDevice* advertisedDevice = nullptr;
    QueueHandle_t qhTask;
    NimBLEClient* bleClient = nullptr;
    NimBLERemoteService* bleService = nullptr;

    class AdvertisedDeviceCallbacks : public NimBLEScanCallbacks {
      void onResult(const NimBLEAdvertisedDevice* advertisedDevice) override {
        NimBLEScanCallbacks::onResult(advertisedDevice);
        if(F503i::connect(advertisedDevice) && F503i::bleScan)
          F503i::bleScan->stop();
      }      
    };
    static AdvertisedDeviceCallbacks* advertisedDeviceCallbacks;

    class ClientCallbacks : public NimBLEClientCallbacks {
      void onConnect(NimBLEClient* pClient) override {
        NimBLEClientCallbacks::onConnect(pClient);
      }
      void onDisconnect(NimBLEClient* pClient, int reason) override {
        NimBLEClientCallbacks::onDisconnect(pClient, reason);
        auto client = F503i::getDevice(pClient->getPeerAddress());
        if(client && F503i::onDisconnect)
          F503i::onDisconnect(client, reason);
      }
    };
    static ClientCallbacks clientCallbacks;

    struct TaskQueue {
      public:
        TaskQueue(F503i* device, uint8_t* pData, size_t length) {
          this->device = device;
          this->pData = new uint8_t[length];
          this->length = length;
          memcpy(this->pData, pData, length);
        }
        ~TaskQueue() {
          delete []pData;
        }

      F503i*    device;
      uint8_t*  pData;
      size_t    length;
    };

  public: // Stream
    size_t write(uint8_t value);
    int available() override;
    int read() override;
    int peek() override;

};

#endif