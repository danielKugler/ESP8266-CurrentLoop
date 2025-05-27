#ifndef CPORTAL_H
#define CPORTAL_H

#include <ArduinoJson.h>
#include <DNSServer.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESPAsyncWebServer.h>
#include <pgmspace.h>

#include "LittleFSManager.h"

class CPortal {
   public:
    CPortal();
    void begin();
    void update(unsigned int level, unsigned int adcValue, unsigned int minAdcValue, unsigned int maxAdcValue, unsigned int interval, unsigned int timestamp, boolean menuDirection);
    void reset();
    void onIntervalChanged(std::function<void(unsigned int)> callback);
    void onAdcChanged(std::function<void(String, unsigned int)> callback);
    void onLedDirectionChanged(std::function<void(boolean)> callback);

   private:
    String CP_SSID = "Sensor";
    IPAddress APIP = IPAddress(192, 168, 42, 1);
    String HOSTNAME = "sensor";
    int DNS_PORT = 53;

    void setupAccessPoint();
    void stopAccessPoint();
    void setupWebServer();
    void setupDNS();
    void stopDNS();

    unsigned int sensorLevel;
    unsigned int sensorAdc;
    unsigned int sensorAdcMin;
    unsigned int sensorAdcMax;
    unsigned int measureTimestamp;
    unsigned int measureInterval;
    boolean menuUpsideDown;

    AsyncWebServer server;
    DNSServer dnsServer;
    ESP8266WiFiMulti WiFiMulti;

    String currentSSID;
    String currentPassword;
    LittleFSManager store;

    void handleRoot(AsyncWebServerRequest* request);
    void handleScan(AsyncWebServerRequest* request);
    void handleConnect(AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total);
    void handleDisconnect(AsyncWebServerRequest* request);
    void handleStatus(AsyncWebServerRequest* request);
    void handleManifest(AsyncWebServerRequest* request);
    void handleInterval(AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total);
    void handleAdc(AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total);
    void handleLedDirection(AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total);
    void handleSensorLevel(AsyncWebServerRequest* request);

    bool tryConnect(const String& ssid, const String& password);

    void redirect(AsyncWebServerRequest* request);
    void handleSuccess(AsyncWebServerRequest* request);

    String toStringIp(IPAddress ip);

    // Callback
    std::function<void(unsigned int)> onIntervalChangedCallback;
    std::function<void(String, unsigned int)> onIntervalAdcChangedCallback;
    std::function<void(boolean)> onLedDirectionChangedCallback;
};

#endif
