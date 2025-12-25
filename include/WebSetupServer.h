#ifndef WEB_SETUP_SERVER_H
#define WEB_SETUP_SERVER_H

#include <Arduino.h>
#include <WebServer.h>
#include <WiFi.h>
#include <vector>
#include "MappingStore.h"
#include "SdScanner.h"
#include "RFID_Manager.h"

class Settings_Manager;
class Battery_Manager;

class WebSetupServer {
public:
    WebSetupServer();

    bool begin(MappingStore* store, SdScanner* scanner, RFID_Manager* rfid, const String& contentRoot = "/", Settings_Manager* settings = nullptr, Battery_Manager* battery = nullptr);
    bool start();
    void stop();
    void loop();

    bool isActive() const { return active; }

private:
    WebServer server;
    bool active;
    bool waitingForTag;
    String selectedFolder;
    String lastUid;
    String contentRoot;

    MappingStore* mappingStore;
    SdScanner* sdScanner;
    RFID_Manager* rfidManager;
    Settings_Manager* settingsManager;
    Battery_Manager* batteryManager;

    std::vector<String> unassignedFolders;

    // Internal helpers
    void registerRoutes();
    void sendJson(int statusCode, const String& body);
    void handleRoot();
    void handleSettingsPage();
    void handleFolders();
    void handleSelect();
    void handleTag();
    void handleAssign();
    void handleReassign();
    void handleDone();
    void handleBattery();
    void handleSettingsJson();
    void handleSettingsSave();

    void refreshFolders();
    bool normalizeUid(String& uid) const;
    bool appendMapping(const String& uid, const String& folder);
    bool reassignMapping(const String& uid, const String& folder, String& previous);
};

#endif // WEB_SETUP_SERVER_H

