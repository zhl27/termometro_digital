#include "BleJsonFacade.h"

// Standard Nordic UART Service (NUS) UUIDs recognized by Bluefruit Connect
static const char* NUS_SERVICE_UUID = "6E400001-B5A3-F393-E0A9-E50E24DCCA9E";
static const char* NUS_RX_CHAR_UUID = "6E400002-B5A3-F393-E0A9-E50E24DCCA9E";
static const char* NUS_TX_CHAR_UUID = "6E400003-B5A3-F393-E0A9-E50E24DCCA9E";

// Safe payload size per BLE notification (iOS negotiates ~185 byte MTU)
static const size_t BLE_CHUNK_SIZE = 180;

class BleJsonFacade::ServerCallbacks : public BLEServerCallbacks {
public:
  explicit ServerCallbacks(BleJsonFacade* parent) : _parent(parent) {}

  void onConnect(BLEServer* pServer) override {
    _parent->_deviceConnected = true;
  }

  void onDisconnect(BLEServer* pServer) override {
    _parent->_deviceConnected = false;
  }

private:
  BleJsonFacade* _parent;
};

class BleJsonFacade::RxCallbacks : public BLECharacteristicCallbacks {
public:
  explicit RxCallbacks(BleJsonFacade* parent) : _parent(parent) {}

  void onWrite(BLECharacteristic* pCharacteristic) override {
    if (!_parent->_onReceiveCallback) return;

    // Supports both ESP32 Arduino Core 3.x (String) and Core 2.x (std::string)
    #if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
      String rxValue = pCharacteristic->getValue();
    #else
      std::string raw = pCharacteristic->getValue();
      String rxValue = String(raw.c_str());
    #endif

    rxValue.trim();
    if (rxValue.length() > 0) {
      _parent->_onReceiveCallback(rxValue);
    }
  }

private:
  BleJsonFacade* _parent;
};

BleJsonFacade::BleJsonFacade()
  : _pServer(nullptr),
    _pTxCharacteristic(nullptr),
    _pRxCharacteristic(nullptr),
    _deviceConnected(false),
    _oldDeviceConnected(false),
    _hasFields(false),
    _onReceiveCallback(nullptr) {}

void BleJsonFacade::begin(const char* deviceName) {
  BLEDevice::init(deviceName);
  BLEDevice::setMTU(185);

  _pServer = BLEDevice::createServer();
  _pServer->setCallbacks(new ServerCallbacks(this));

  BLEService* pService = _pServer->createService(NUS_SERVICE_UUID);

  // TX Characteristic: ESP32 -> iPhone
  _pTxCharacteristic = pService->createCharacteristic(
    NUS_TX_CHAR_UUID,
    BLECharacteristic::PROPERTY_NOTIFY
  );
  _pTxCharacteristic->addDescriptor(new BLE2902());

  // RX Characteristic: iPhone -> ESP32
  _pRxCharacteristic = pService->createCharacteristic(
    NUS_RX_CHAR_UUID,
    BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_WRITE_NR
  );
  _pRxCharacteristic->setCallbacks(new RxCallbacks(this));

  pService->start();

  BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(NUS_SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  BLEDevice::startAdvertising();
}

void BleJsonFacade::update() {
  // Automatically restart advertising after a client disconnects
  if (!_deviceConnected && _oldDeviceConnected) {
    delay(500);
    BLEDevice::startAdvertising();
    _oldDeviceConnected = _deviceConnected;
  }
  if (_deviceConnected && !_oldDeviceConnected) {
    _oldDeviceConnected = _deviceConnected;
  }
}

bool BleJsonFacade::isConnected() const {
  return _deviceConnected;
}

void BleJsonFacade::onReceive(const CommandCallback &callback) {
  _onReceiveCallback = callback;
}

// --- Single Key-Value Methods ---

bool BleJsonFacade::sendValue(const char* key, int value) {
  beginObject();
  addField(key, value);
  return sendObject();
}

bool BleJsonFacade::sendValue(const char* key, float value, unsigned int decimals) {
  beginObject();
  addField(key, value, decimals);
  return sendObject();
}

bool BleJsonFacade::sendValue(const char* key, bool value) {
  beginObject();
  addField(key, value);
  return sendObject();
}

bool BleJsonFacade::sendValue(const char* key, const char* value) {
  beginObject();
  addField(key, value);
  return sendObject();
}

bool BleJsonFacade::sendValue(const char* key, const String& value) {
  return sendValue(key, value.c_str());
}

// --- Multi-Field JSON Builder Methods ---

void BleJsonFacade::beginObject() {
  _jsonBuffer = "{";
  _hasFields = false;
}

void BleJsonFacade::appendKey(const char* key) {
  if (_hasFields) {
    _jsonBuffer += ",";
  }
  _jsonBuffer += "\"";
  _jsonBuffer += escapeJson(key);
  _jsonBuffer += "\":";
  _hasFields = true;
}

void BleJsonFacade::addField(const char* key, int value) {
  appendKey(key);
  _jsonBuffer += String(value);
}

void BleJsonFacade::addField(const char* key, float value, unsigned int decimals) {
  appendKey(key);
  if (isnan(value) || isinf(value)) {
    _jsonBuffer += "null";
  } else {
    _jsonBuffer += String(value, decimals);
  }
}

void BleJsonFacade::addField(const char* key, const bool value) {
  appendKey(key);
  _jsonBuffer += (value ? "true" : "false");
}

void BleJsonFacade::addField(const char* key, const char* value) {
  appendKey(key);
  _jsonBuffer += "\"";
  _jsonBuffer += escapeJson(value);
  _jsonBuffer += "\"";
}

void BleJsonFacade::addField(const char* key, const String& value) {
  addField(key, value.c_str());
}

bool BleJsonFacade::sendObject() {
  _jsonBuffer += "}";
  return sendRawJson(_jsonBuffer);
}

bool BleJsonFacade::sendRawJson(const String& json) const {
  if (!_deviceConnected || _pTxCharacteristic == nullptr) {
    return false;
  }
  String packet = json;
  if (!packet.endsWith("\n")) {
    packet += "\n";
  }
  return transmit(packet);
}

// --- Internal Helpers ---

String BleJsonFacade::escapeJson(const char* input) {
  if (!input) return "";
  String out;
  for (size_t i = 0; input[i] != '\0'; i++) {
    char c = input[i];
    switch (c) {
      case '\"': out += "\\\""; break;
      case '\\': out += "\\\\"; break;
      case '\n': out += "\\n";  break;
      case '\r': out += "\\r";  break;
      case '\t': out += "\\t";  break;
      default:   out += c;      break;
    }
  }
  return out;
}

bool BleJsonFacade::transmit(const String& payload) const {
  const size_t totalLen = payload.length();
  size_t offset = 0;

  while (offset < totalLen) {
    const size_t len = min(BLE_CHUNK_SIZE, totalLen - offset);
    String chunk = payload.substring(offset, offset + len);

    _pTxCharacteristic->setValue(chunk.c_str());
    _pTxCharacteristic->notify();

    offset += len;
    if (offset < totalLen) {
      delay(10); // Prevent TX buffer overflow when splitting large JSON strings
    }
  }
  return true;
}
