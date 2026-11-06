#include "Comms.h"
#include "config.h"
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLE2902.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <esp_now.h>

namespace {

// Nordic UART Service (what Adafruit BLEUart / Bluefruit Connect use)
const char* NUS_SERVICE = "6E400001-B5A3-F393-E0A9-E50E24DCCA9E";
const char* NUS_RX      = "6E400002-B5A3-F393-E0A9-E50E24DCCA9E";  // phone writes here
const char* NUS_TX      = "6E400003-B5A3-F393-E0A9-E50E24DCCA9E";  // we notify here

const uint8_t BROADCAST[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
const uint8_t PACKET_MAGIC[2] = {'B', 'W'};
constexpr size_t MAX_MSG = 20;

struct __attribute__((packed)) Packet {
  uint8_t magic[2];
  uint8_t len;
  char data[MAX_MSG];
};

QueueHandle_t cmdQueue;
uint8_t myWheel = 0;
bool espNowOk = false;
BLECharacteristic* txChar = nullptr;

void pushCommand(const char* p, size_t n) {
  Command c = {};
  c.len = min(n, sizeof(c.text) - 1);
  memcpy(c.text, p, c.len);
  xQueueSend(cmdQueue, &c, 0);
}

// Pick this wheel's chunk out of a message and queue it.
void applyMessage(const char* data, size_t len) {
  const char* starts[8];
  size_t lens[8];
  int chunks = 0;
  size_t s = 0;
  for (size_t i = 0; i < len && chunks < 8; i++) {
    if (data[i] == 'X') {
      starts[chunks] = data + s;
      lens[chunks] = i - s + 1;
      chunks++;
      s = i + 1;
    }
  }
  if (chunks == 0) return;
  int pick = (chunks == 1) ? 0 : myWheel;
  if (pick < chunks) pushCommand(starts[pick], lens[pick]);
}

void broadcast(const char* data, size_t len) {
  if (!espNowOk) return;
  Packet p = {};
  memcpy(p.magic, PACKET_MAGIC, 2);
  p.len = min(len, MAX_MSG);
  memcpy(p.data, data, p.len);
  esp_now_send(BROADCAST, (const uint8_t*)&p, sizeof(p));
}

void onEspNowRecv(const esp_now_recv_info_t*, const uint8_t* data, int len) {
  if (len != sizeof(Packet)) return;
  const Packet* p = (const Packet*)data;
  if (memcmp(p->magic, PACKET_MAGIC, 2) != 0 || p->len > MAX_MSG) return;
  applyMessage(p->data, p->len);
}

class RxCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* c) override {
    String v = c->getValue();
    Comms::handlePhoneMessage(v.c_str(), v.length());
  }
};

class ServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer*) override { Serial.println("[BLE] phone connected"); }
  void onDisconnect(BLEServer*) override {
    Serial.println("[BLE] phone disconnected");
    BLEDevice::startAdvertising();
  }
};

void startEspNow() {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  esp_wifi_set_channel(ESPNOW_CHANNEL, WIFI_SECOND_CHAN_NONE);
  if (esp_now_init() != ESP_OK) {
    Serial.println("[ESP-NOW] init failed");
    return;
  }
  esp_now_peer_info_t peer = {};
  memcpy(peer.peer_addr, BROADCAST, 6);
  peer.channel = ESPNOW_CHANNEL;
  peer.ifidx = WIFI_IF_STA;
  peer.encrypt = false;
  esp_now_add_peer(&peer);
  esp_now_register_recv_cb(onEspNowRecv);
  espNowOk = true;
  Serial.printf("[ESP-NOW] ready, MAC %s\n", WiFi.macAddress().c_str());
}

void startBle() {
  String name = String(BLE_NAME_PREFIX) + char('A' + myWheel);
  BLEDevice::init(name.c_str());
  BLEServer* server = BLEDevice::createServer();
  server->setCallbacks(new ServerCallbacks());

  BLEService* svc = server->createService(NUS_SERVICE);
  // READ too: the iOS app calls readValue() on this characteristic after connecting.
  txChar = svc->createCharacteristic(NUS_TX, BLECharacteristic::PROPERTY_NOTIFY | BLECharacteristic::PROPERTY_READ);
#if defined(CONFIG_BLUEDROID_ENABLED)
  txChar->addDescriptor(new BLE2902());   // NimBLE adds this automatically
#endif
  BLECharacteristic* rx = svc->createCharacteristic(
      NUS_RX, BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_WRITE_NR);
  rx->setCallbacks(new RxCallbacks());
  svc->start();

  BLEAdvertising* adv = BLEDevice::getAdvertising();
  adv->addServiceUUID(NUS_SERVICE);
  adv->setScanResponse(true);
  BLEDevice::startAdvertising();
  Serial.printf("[BLE] advertising as %s\n", name.c_str());
}

}  // namespace

namespace Comms {

void begin(uint8_t wheelId) {
  myWheel = wheelId;
  cmdQueue = xQueueCreate(8, sizeof(Command));
  startEspNow();   // WiFi first so BLE/WiFi coexistence is set up with WiFi running
  startBle();
}

void handlePhoneMessage(const char* data, size_t len) {
  if (len == 0) return;
  if (data[0] == 'W') {           // wheel id: this unit only, never relayed
    pushCommand(data, len);
    return;
  }
  applyMessage(data, len);
  broadcast(data, len);
  if (txChar) {
    txChar->setValue("OK");
    txChar->notify();
  }
}

bool receive(Command& out) {
  return xQueueReceive(cmdQueue, &out, 0) == pdTRUE;
}

}  // namespace Comms
