#include <F503i.h>

// つながったら、ここが呼ばれるよ
void onConnect(F503i* device) {
  Serial.printf("connected %s", device->getAddress().toString().c_str());
  Serial.println();
}

// 切れたら、ここが呼ばれるよ
void onDisconnect(F503i* device, int reason) {
  Serial.printf("disconnected %s reason %d", device->getAddress().toString().c_str(), reason);
  Serial.println();
}

// キーが押されたときに、ここが呼ばれるよ
void onKey(F503i* device, const char key) {
  Serial.print(device->getAddress().toString().c_str());
  if(key)
    Serial.printf(" pressed %c", key);
  else
    Serial.print(" released");
  Serial.printf(" illuminance %d", device->getIlluminance());
  Serial.println();

  const uint8_t brightness[] = {0x11, 0x33, 0x77, 0xff};
  switch(key) {
    case '1':
      device->setLed(0, brightness[0]);
      device->setBuzzer(36);
      break;
    case '2':
      device->setLed(1, brightness[0]);
      device->setBuzzer(38);
      break;
    case '3':
      device->setLed(2, brightness[0]);
      device->setBuzzer(40);
      break;
    case '4':
      device->setLed(0, brightness[1]);
      device->setBuzzer(41);
      break;
    case '5':
      device->setLed(1, brightness[1]);
      device->setBuzzer(43);
      break;
    case '6':
      device->setLed(2, brightness[1]);
      device->setBuzzer(45);
      break;
    case '7':
      device->setLed(0, brightness[2]);
      device->setBuzzer(47);
      break;
    case '8':
      device->setLed(1, brightness[2]);
      device->setBuzzer(48);
      break;
    case '9':
      device->setLed(2, brightness[2]);
      device->setBuzzer(50);
      break;
    case '0':
      device->setLed(1, brightness[3]);
      device->setBuzzer(53);
      break;
    case '*':
      device->setLed(0, brightness[3]);
      device->setBuzzer(52);
      break;
    case '#':
      device->setLed(2, brightness[3]);
      device->setBuzzer(55);
      break;
    case 0x00:
      device->setLed(0, 0x00);
      device->setLed(1, 0x00);
      device->setLed(2, 0x00);
      device->setBuzzer(0x00);
      break;
  }
}

void setup() {
  Serial.begin(115200);

  F503i::setup_NimBLE();              // NimBLE_Arduino の処理をライブラリが引き受けます
  F503i::onKey        = onKey;        // キーが押されたとき
  F503i::onConnect    = onConnect;    // つながったとき
  F503i::onDisconnect = onDisconnect; // 切れたとき
}

void loop() {
  F503i::handle();                    // これは消さないでね
}
