#ifndef BLE_JSON_FACADE_H
#define BLE_JSON_FACADE_H

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <functional>

class BleJsonFacade {
public:
  using CommandCallback = std::function<void(const String&)>;

  BleJsonFacade();

  // Initializes BLE stack, Nordic UART Service, and starts advertising
  void begin(const char* deviceName = "ESP32_Bluefruit");

  // Call inside loop() to handle automatic re-advertising after disconnect
  void update();

  bool isConnected() const;

  // Optional callback when text/commands are sent from the iPhone app
  void onReceive(const CommandCallback &callback);

  // --- Single Key-Value Helpers (sends immediately as {"key":value}\n) ---
  bool sendValue(const char* key, int value);
  bool sendValue(const char* key, float value, unsigned int decimals = 2);
  bool sendValue(const char* key, bool value);
  bool sendValue(const char* key, const char* value);
  bool sendValue(const char* key, const String& value);

  // --- Multi-Field JSON Builder (bundles multiple fields into one packet) ---
  void beginObject();
  void addField(const char* key, int value);
  void addField(const char* key, float value, unsigned int decimals = 2);
  void addField(const char* key, bool value);
  void addField(const char* key, const char* value);
  void addField(const char* key, const String& value);
  bool sendObject();

  // Sends a pre-formatted JSON string (automatically appends '\n' and chunks if needed)
  bool sendRawJson(const String& json) const;

private:
  // Encapsulated BLE callback handlers
  class ServerCallbacks;
  class RxCallbacks;

  BLEServer* _pServer;
  BLECharacteristic* _pTxCharacteristic;
  BLECharacteristic* _pRxCharacteristic;

  bool _deviceConnected;
  bool _oldDeviceConnected;
  String _jsonBuffer;
  bool _hasFields;
  CommandCallback _onReceiveCallback;

  void appendKey(const char* key);
  static String escapeJson(const char* input) ;
  bool transmit(const String& payload) const;
};

#endif // BLE_JSON_FACADE_H