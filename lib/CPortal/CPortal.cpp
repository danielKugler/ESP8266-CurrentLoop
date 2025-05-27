#include "CPortal.h"

#define ADDR_SSID "wifi_ssid"
#define ADDR_PASSWORD "wifi_password"

CPortal::CPortal() : server(80) {}

void CPortal::begin() {
    // Serial.println("CPortal::begin");
    WiFi.hostname(HOSTNAME);
    WiFi.mode(WIFI_STA);
    // WiFi.disconnect();
    currentSSID = store.read(ADDR_SSID, "");
    currentPassword = store.read(ADDR_PASSWORD, "");

    if (currentSSID.isEmpty()) {
        setupAccessPoint();
    } else {
        WiFi.begin(currentSSID.c_str(), currentPassword.c_str());
        if (WiFi.waitForConnectResult() != WL_CONNECTED) {
            Serial.println("CaptivePortal::begin --> FAILED TO CONNECT");
            reset();
            setupAccessPoint();
        } else {
            Serial.println("CaptivePortal::begin --> CONNECTED: " + WiFi.localIP().toString());
        }
    }

    setupWebServer();
}

/**
 * @brief Updates the captive portal's sensor data.
 *
 * This function updates the captive portal's sensor data with the given values.
 * The values are stored in the class's member variables and can be accessed
 * through the corresponding getter functions.
 *
 * @param level The current level of the sensor.
 * @param adcValue The current ADC value of the sensor.
 * @param minAdcValue The minimum ADC value of the sensor.
 * @param maxAdcValue The maximum ADC value of the sensor.
 * @param interval The current measurement interval of the sensor.
 * @param timestamp The current timestamp of the sensor.
 * @param upsideDown Whether the menu is upside down or not.
 */
void CPortal::update(unsigned int level, unsigned int adcValue, unsigned int minAdcValue, unsigned int maxAdcValue, unsigned int interval, unsigned int timestamp, boolean upsideDown) {
    sensorLevel = level;
    sensorAdc = adcValue;
    sensorAdcMin = minAdcValue;
    sensorAdcMax = maxAdcValue;
    measureInterval = interval;
    measureTimestamp = timestamp;
	menuUpsideDown = upsideDown;
    dnsServer.processNextRequest();  // DNS-Anfragen verarbeiten
}

/**
 * @brief Starts the captive portal's access point.
 *
 * This function starts the captive portal's access point. The access point is
 * configured to use the IP address 192.168.42.1 and the subnet mask 255.255.255.0.
 * The DNS server is also started to redirect all DNS requests to the captive
 * portal's IP address.
 */
void CPortal::setupAccessPoint() {
    // Serial.println("CPortal::setupAccessPoint");
    WiFi.disconnect();
    WiFi.mode(WIFI_AP);
    WiFi.softAPConfig(APIP, APIP, IPAddress(255, 255, 255, 0));
    WiFi.softAP(CP_SSID);

    setupDNS();  // DNS-Server einrichten
    IPAddress apIP = WiFi.softAPIP();

    Serial.println("Captive Portal gestartet. IP: " + apIP.toString());
}

/**
 * @brief Stops the captive portal's access point.
 *
 * This function stops the captive portal's access point and switches the
 * Wi-Fi mode to station mode. It also stops the DNS server that was started
 * for the captive portal.
 */
void CPortal::stopAccessPoint() {
    // Serial.println("CPortal::stopAccessPoint");
    WiFi.mode(WIFI_STA);
    stopDNS();
}

/**
 * @brief Starts the DNS server for the captive portal.
 *
 * This function starts the DNS server for the captive portal. The DNS server
 * is used to redirect all DNS requests to the captive portal's IP address.
 */
void CPortal::setupDNS() {
    // Serial.println("CPortal::setupDNS");
    dnsServer.start(DNS_PORT, "*", APIP);
}

/**
 * @brief Stops the DNS server.
 *
 * This function stops the DNS server that was started for the captive portal,
 * effectively disabling the DNS resolution for the access point.
 */
void CPortal::stopDNS() {
    // Serial.println("CPortal::stopDNS");
    dnsServer.stop();
}

    /**
     * @brief Set up the web server.
     *
     * This function sets up the web server, including routes for the main
     * page, the sensor level, the status, and more. It also sets up the
     * captive portal detection routes.
     */
void CPortal::setupWebServer() {
    // Serial.println("CPortal::setupWebServer");

    server.on("/", HTTP_GET, [this](AsyncWebServerRequest* request) {
        handleRoot(request);
    });

    // Andere Routen
    server.on("/scan", HTTP_GET, [this](AsyncWebServerRequest* request) { handleScan(request); });
    server.on("/sensor", HTTP_GET, [this](AsyncWebServerRequest* request) { handleSensorLevel(request); });
    server.on("/status", HTTP_GET, [this](AsyncWebServerRequest* request) { handleStatus(request); });

    server.on("/manifest.json", HTTP_GET, [this](AsyncWebServerRequest* request) { handleManifest(request); });
    server.on("/icon.webp", HTTP_GET, [](AsyncWebServerRequest* request) {
        request->send(LittleFS, "/icon.webp", "image/webp");
    });

    server.on("/interval", HTTP_POST, [](AsyncWebServerRequest* request) {}, NULL, [this](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) { handleInterval(request, data, len, index, total); });
    server.on("/connect", HTTP_POST, [](AsyncWebServerRequest* request) {}, NULL, [this](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) { handleConnect(request, data, len, index, total); });

    server.on("/adc", HTTP_POST, [](AsyncWebServerRequest* request) {}, NULL, [this](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) { handleAdc(request, data, len, index, total); });
    server.on("/ledDirection", HTTP_POST, [](AsyncWebServerRequest* request) {}, NULL, [this](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) { handleLedDirection(request, data, len, index, total); });

    server.on("/disconnect", HTTP_GET, [this](AsyncWebServerRequest* request) { handleDisconnect(request); });

    // Allways redirect to captive portal. Request comes with IP (8.8.8.8) or URL (connectivitycheck.XXX / captive.apple / etc.)
    server.on("/hotspot-detect.html", HTTP_GET, [this](AsyncWebServerRequest* request) { handleRoot(request); });
    server.on("/success.txt", [this](AsyncWebServerRequest* request) { handleSuccess(request); });     // detectportal.firefox.com/sucess.txt
                                                                                                       // redirects
    server.on("/generate_204", [this](AsyncWebServerRequest* request) { redirect(request); });         // Android captive portal.
    server.on("/fwlink", [this](AsyncWebServerRequest* request) { redirect(request); });               // Microsoft captive portal.
    server.on("/connecttest.txt", [this](AsyncWebServerRequest* request) { redirect(request); });      // www.msftconnecttest.com
    server.on("/hotspot-detect.html", [this](AsyncWebServerRequest* request) { redirect(request); });  // captive.apple.com

    server.begin();
}

/**
 * @brief Redirect to captive portal.
 *
 * This function is called when a redirect is necessary. It redirects the
 * request to the captive portal using the IP address of the ESP.
 *
 * @param request The request object.
 */
void CPortal::redirect(AsyncWebServerRequest* request) {
    // Serial.println("CaptivePortal::redirect");
    request->redirect(String("http://") + toStringIp(WiFi.localIP()));
    // request->addHeader("Location", String("http://") + toStringIp(WiFi.localIP()), true);
    // request->send(302, "text/plain", "");  // Empty content inhibits Content-length header so we have to close the socket ourselves.
    // server.client().stop();              // Stop is needed because we sent no content length
}

/**
 * @brief Handle success request.
 *
 * This function is called when a success state is reached. It sends a 200
 * success response with the string "success" to the client.
 *
 * @param request The request object.
 */
void CPortal::handleSuccess(AsyncWebServerRequest* request) {
    // Serial.println("CaptivePortal::handleSuccess");
    // Serial.println(F("Handle success.txt"));
    request->send(200, "text/plain", "success");
}

/**
 * @brief Handle root request.
 *
 * This function serves as the main entry point for the captive portal.
 * It creates and sends a chunked HTML response containing the main
 * page content of the portal. The HTML content is embedded in the 
 * code and sent in chunks to the client. This approach is used to 
 * efficiently handle large HTML payloads. The function ensures the 
 * entire HTML content is transmitted by iterating over the content 
 * and sending it in pieces until fully delivered. The optimized html, css and js code
 * will be packed in here during build process.
 *
 * @param request The HTTP request object.
 */
void CPortal::handleRoot(AsyncWebServerRequest* request) {
    // Serial.println("CaptivePortal::handleRoot");
    AsyncWebServerResponse* response = request->beginChunkedResponse("text/html", [](uint8_t* buffer, size_t maxLen, size_t index) -> size_t {
        // AUTOGENERATED_HTML
        const char* html = R"=(__HTML_CONTENT_WILL_BE_INJECTED__)=";

        size_t len = strlen(html);
        if (index >= len) {
            return 0;  // No more data to send
        }
        size_t remaining = len - index;
        size_t toSend = (remaining > maxLen) ? maxLen : remaining;
        memcpy(buffer, html + index, toSend);
        return toSend;
    });
    request->send(response);
}

/**
 * @brief Handle scan request.
 *
 * This function processes an HTTP GET request to scan for available
 * WiFi networks. It uses WiFi.scanNetworksAsync() to scan for networks
 * and sends a JSON response containing the SSID, signal strength, channel,
 * and encryption type of each network.
 *
 * @param request The request object.
 */
void CPortal::handleScan(AsyncWebServerRequest* request) {
    // Serial.println("CaptivePortal::handleScan");
    WiFi.scanNetworksAsync([request](int networks) {
        String response;
        JsonDocument doc;
        JsonArray networksArray = doc.to<JsonArray>();

        for (int i = 0; i < networks; i++) {
            JsonObject net = networksArray.add<JsonObject>();
            net["ssid"] = WiFi.SSID(i);
            net["signal"] = map(WiFi.RSSI(i), -100, -50, 0, 100);
            net["channel"] = WiFi.channel(i);
            net["secured"] = (WiFi.encryptionType(i) != AUTH_OPEN);
        }
        serializeJson(doc, response);
        request->send(200, "application/json", response);
        WiFi.scanDelete();  // Speicher freigeben
    });
}

/**
 * @brief Handle WiFi connection request.
 *
 * This function processes an HTTP POST request to connect to a WiFi network.
 * It deserializes the incoming JSON data to extract the SSID and password.
 * If the JSON is invalid or the SSID is missing, it sends a 400 error response.
 * Otherwise, it saves the SSID and password, sends a 200 success response,
 * and restarts the ESP to apply the changes.
 *
 * @param request The HTTP request object.
 * @param data The incoming data buffer containing the JSON payload.
 * @param len The length of the data buffer.
 * @param index The current index of the data being processed.
 * @param total The total size of the data being processed.
 */
void CPortal::handleConnect(AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
    // Serial.println("CaptivePortal::handleConnect");

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, data);

    if (error) {
        request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
        return;
    }

    String ssid = doc["ssid"] | "";
    String password = doc["password"] | "";

    // Überprüfen, ob SSID vorhanden ist
    if (ssid.isEmpty()) {
        request->send(400, "application/json", "{\"error\":\"Missing SSID\"}");
        return;
    }

    request->send(200, "application/json", "{\"restart\":true}");

    store.save(ADDR_SSID, ssid);
    store.save(ADDR_PASSWORD, password);

    ESP.restart();
}

/**
 * @brief Try to connect to the given network.
 *
 * @param ssid The network's SSID.
 * @param password The network's password.
 *
 * @return true if the connection was successful, false otherwise.
 */
bool CPortal::tryConnect(const String& ssid, const String& password) {
    // Serial.println("CaptivePortal::tryConnect");

    // stopAccessPoint();
    WiFi.begin(ssid.c_str(), password.c_str());
    if (WiFi.waitForConnectResult() != WL_CONNECTED) {
        Serial.println("CaptivePortal::begin --> FAILED TO CONNECT");
        return false;
    } else {
        Serial.println("CaptivePortal::begin --> CONNECTED");
        return true;
    }
}

void CPortal::handleDisconnect(AsyncWebServerRequest* request) {
    // Serial.println("CaptivePortal::handleDisconnect");
    request->send(200, "application/json", "{\"connected\":false,\"restart\":true}");
    delay(2500);
    WiFi.disconnect(true);
    reset();
    setupAccessPoint();
}

/**
 * @brief Handle changed measurement interval.
 *
 * This function is called when the measurement interval is changed
 * via the captive portal. It deserializes the incoming JSON data to
 * extract the new interval, sends a 200 success response and invokes
 * the callback to change the interval.
 *
 * @param request The HTTP request object.
 * @param data The incoming data buffer containing the JSON payload.
 * @param len The length of the data buffer.
 * @param index The current index of the data being processed.
 * @param total The total size of the data being processed.
 */
void CPortal::handleInterval(AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, data);

    if (error) {
        request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
        return;
    }

    request->send(200, "application/json", "{\"success\":true}");

    String temp = doc["interval"];
    unsigned int interval = temp.toInt();

    if (onIntervalChangedCallback) {
        onIntervalChangedCallback(interval);
    }
}

/**
 * @brief Handle ADC value change request.
 *
 * This function processes an HTTP POST request to change the ADC values for
 * min or max. It deserializes the incoming JSON data to extract the new ADC
 * value. If the JSON is invalid, it sends a 400 error response. Otherwise, it
 * sends a 200 success response and invokes the callback to change the ADC
 * values.
 *
 * @param request The HTTP request object.
 * @param data The incoming data buffer containing the JSON payload.
 * @param len The length of the data buffer.
 * @param index The current index of the data being processed.
 * @param total The total size of the data being processed.
 */
void CPortal::handleAdc(AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, data);

    if (error) {
        request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
        return;
    }

    request->send(200, "application/json", "{\"success\":true}");

    if (onIntervalAdcChangedCallback && doc["change"] != "" && doc["value"] != "") {
        onIntervalAdcChangedCallback(doc["change"], doc["value"]);
    }
}

/**
 * @brief Handle LED direction change request.
 *
 * This function processes an HTTP POST request to change the LED direction.
 * It deserializes the incoming JSON data to extract the new LED direction state.
 * If the JSON is invalid, it sends a 400 error response. Otherwise, it sends a
 * 200 success response and invokes the callback to change the LED direction.
 *
 * @param request The HTTP request object.
 * @param data The incoming data buffer containing the JSON payload.
 * @param len The length of the data buffer.
 * @param index The current index of the data being processed.
 * @param total The total size of the data being processed.
 */
void CPortal::handleLedDirection(AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
	JsonDocument doc;
    DeserializationError error = deserializeJson(doc, data);
	
    if (error) {
		request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
        return;
    }
	
    request->send(200, "application/json", "{\"success\":true}");
	
	Serial.println("CPortal::handleLedDirection : " + String(doc["status"]));
    if (onLedDirectionChangedCallback && doc["status"] != "") {
        onLedDirectionChangedCallback(doc["status"]);
    }
}

/**
 * @brief Handle status request.
 *
 * This function generates and sends a status JSON response for the captive portal.
 * It includes the current connection status, timestamp, interval, menu orientation,
 * and network information (IP, SSID, signal strength, channel, and if secured).
 * The response is sent in JSON format with the necessary properties.
 */
void CPortal::handleStatus(AsyncWebServerRequest* request) {
    // Serial.println("CaptivePortal::handleStatus");
    JsonDocument doc;

    if (WiFi.status() == WL_CONNECTED) {
        doc["connected"] = true;
        doc["timestamp"] = measureTimestamp;
        doc["interval"] = measureInterval;
        doc["menuUpsideDown"] = menuUpsideDown;
        JsonObject wifi = doc["wifi"].to<JsonObject>();
        wifi["ip"] = WiFi.localIP().toString();
        wifi["ssid"] = WiFi.SSID();
        wifi["signal"] = map(WiFi.RSSI(), -100, -50, 0, 100);
        wifi["channel"] = WiFi.channel();
        wifi["secured"] = !WiFi.psk().isEmpty();
    } else {
        doc["connected"] = false;
    }

    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
}

/**
 * @brief Handle manifest request.
 *
 * This function generates and sends a manifest JSON response for the web application.
 * It sets the name, short name, display mode, background color, and icons for the 
 * application. The response is sent in JSON format with the necessary properties for 
 * a web manifest, allowing the web application to be installed on devices.
 *
 * @param request The request object.
 */
void CPortal::handleManifest(AsyncWebServerRequest* request) {
    JsonDocument doc;

    doc["name"] = "Sensor";
    doc["short_name"] = "Sensor";
    doc["display"] = "standalone";
    doc["background_color"] = "#212121";
    JsonArray icons = doc["icons"].to<JsonArray>();
    JsonDocument icon;
    icon["src"] = "icon.webp";
    icon["type"] = "image/webp";
    icon["sizes"] = "192x192";
    icons.add(icon);

    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
}

/**
 * @brief Handle sensor level request.
 *
 * This function is called when the sensor level is requested from the captive
 * portal. It returns the sensor level, the ADC value, the minimum and maximum
 * ADC values, the timestamp of the measurement and the interval of the
 * measurement.
 *
 * @param request The request object.
 */
void CPortal::handleSensorLevel(AsyncWebServerRequest* request) {
    // Serial.println("CaptivePortal::handleSensorLevel");
    JsonDocument doc;
    doc["value"] = String(sensorLevel);
    doc["adcValue"] = String(sensorAdc);
    doc["adcMin"] = String(sensorAdcMin);
    doc["adcMax"] = String(sensorAdcMax);
    doc["timestamp"] = String(measureTimestamp);
    doc["interval"] = String(measureInterval);

    String response;
    serializeJson(doc, response);

    request->send(200, "application/json", response);
}

/**
 * @brief Converts an IPAddress to a string in the format "X.X.X.X".
 *
 * @param ip The IPAddress to convert.
 *
 * @return A string representation of the IPAddress.
 */
String CPortal::toStringIp(IPAddress ip) {
    String res = "";
    for (int i = 0; i < 3; i++) {
        res += String((ip >> (8 * i)) & 0xFF) + ".";
    }
    res += String(((ip >> 8 * 3)) & 0xFF);
    return res;
}

/**
 * @brief Resets the Captive Portal to its default state.
 *
 * This function clears the stored SSID and password, and disconnects
 * the ESP from any currently connected network. The Captive Portal
 * is then restarted. This function is called when the user requests
 * a factory reset.
 */
void CPortal::reset() {
    store.clear(ADDR_SSID);
    store.clear(ADDR_PASSWORD);
}

/**
 * @brief Registers a callback for changed measurement intervals.
 *
 * This function sets a user-defined callback to be invoked when the
 * measurement interval is changed via the captive portal. The callback
 * is provided with a single parameter:
 * - An unsigned integer containing the new measurement interval in seconds.
 */
void CPortal::onIntervalChanged(std::function<void(unsigned int)> callback) {
    onIntervalChangedCallback = callback;
}

/**
 * @brief Registers a callback for changed ADC values.
 *
 * This function sets a user-defined callback to be invoked when the
 * ADC value is changed via the captive portal. The callback is
 * provided with two parameters:
 * - A string indicating if the minimum or maximum ADC value was changed.
 * - An unsigned integer containing the new ADC value.
 */
void CPortal::onAdcChanged(std::function<void(String, unsigned int)> callback) {
    onIntervalAdcChangedCallback = callback;
}

/**
 * @brief Registers a callback for LED direction changes.
 *
 * This function sets a user-defined callback to be invoked when the
 * LED direction is changed via the captive portal.
 *
 * @param callback A function that takes a boolean parameter indicating
 * the new LED direction state.
 */

void CPortal::onLedDirectionChanged(std::function<void(boolean)> callback) {
    onLedDirectionChangedCallback = callback;
}