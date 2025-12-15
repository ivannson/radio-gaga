#include "WebSetupServer.h"
#include "Logger.h"

static const char* kApSsid = "setup";   // open network as requested

// Minified single-page UI to reduce flash size
static const char* kIndexHtml PROGMEM = R"HTML(<!DOCTYPE html><html><head><meta charset="UTF-8"><title>Setup</title><style>body{font-family:Arial;margin:12px;background:#f6f6f6}h1{font-size:18px}button{padding:8px 12px;margin:4px}.folder{padding:8px;background:#fff;margin:6px 0;border:1px solid #ddd;cursor:pointer}.status{margin:10px 0}.hidden{display:none}#modal{position:fixed;top:0;left:0;right:0;bottom:0;background:rgba(0,0,0,.5);display:none;align-items:center;justify-content:center}#modalContent{background:#fff;padding:12px;border-radius:4px;max-width:320px}</style></head><body><h1>Setup</h1><div id=folders></div><div class=status id=status></div><div id=actions class=hidden><button id=doneBtn>Done</button></div><div id=modal><div id=modalContent><div id=modalText></div><div style=margin-top:10px><button id=reassignBtn>Reassign</button><button id=cancelBtn>Cancel</button></div></div></div><script>let t=null,f="",u="";const S=e=>document.getElementById("status").innerText=e,A=e=>document.getElementById("actions").classList.toggle("hidden",!e),M=e=>{document.getElementById("modalText").innerText=e,document.getElementById("modal").style.display="flex"},C=()=>document.getElementById("modal").style.display="none";async function L(){const e=await fetch("/folders"),n=await e.json(),d=document.getElementById("folders");if(d.innerHTML="",!n.folders||!n.folders.length)return d.innerHTML="<div>No unassigned folders found.</div>",A(!0);n.folders.forEach(e=>{const n=document.createElement("div");n.className="folder",n.innerText=e,n.onclick=()=>E(e),d.appendChild(n)})}async function E(e){await fetch("/select",{method:"POST",headers:{"Content-Type":"application/x-www-form-urlencoded"},body:"folder="+encodeURIComponent(e)}),f=e,S("Waiting for tag for "+e+"..."),A(!1),g()}function g(){t&&clearInterval(t),t=setInterval(T,600)}async function T(){const e=await fetch("/tag"),n=await e.json();"tag_detected"===n.status&&(u=n.uid,clearInterval(t),S("Tag "+u+" detected, assigning..."),y(!1))}async function y(e){const n=`uid=${encodeURIComponent(u)}&folder=${encodeURIComponent(f)}&force=${e?"1":"0"}`,d=await fetch("/assign",{method:"POST",headers:{"Content-Type":"application/x-www-form-urlencoded"},body:n}),o=await d.json();"assigned"===o.status||"already_assigned_same"===o.status?(S("Assigned to "+f+"."),A(!0)):"conflict"===o.status?M("This cassette is already assigned to "+o.folder+". Reassign?"):(S("Error: "+(o.message||"unknown")),A(!0))}document.getElementById("reassignBtn").onclick=async()=>{C();const e=`uid=${encodeURIComponent(u)}&folder=${encodeURIComponent(f)}`,n=await fetch("/reassign",{method:"POST",headers:{"Content-Type":"application/x-www-form-urlencoded"},body:e}),d=await n.json();"reassigned"===d.status?S("Reassigned to "+f+"."):S("Reassign failed: "+(d.message||"unknown")),A(!0)},document.getElementById("cancelBtn").onclick=async()=>{C(),S("Choose another folder."),u="",await L()},document.getElementById("doneBtn").onclick=async()=>{await fetch("/done",{method:"POST"}),S("Done. You can close this page.")},L();</script></body></html>)HTML";

WebSetupServer::WebSetupServer()
    : server(80),
      active(false),
      waitingForTag(false),
      selectedFolder(""),
      lastUid(""),
      contentRoot("/"),
      mappingStore(nullptr),
      sdScanner(nullptr),
      rfidManager(nullptr) {}

bool WebSetupServer::begin(MappingStore* store, SdScanner* scanner, RFID_Manager* rfid, const String& root) {
    mappingStore = store;
    sdScanner = scanner;
    rfidManager = rfid;
    contentRoot = root;

    if (!mappingStore || !sdScanner || !rfidManager) {
        LOG_ERROR("[WEB-SETUP] Invalid components");
        return false;
    }

    return true;
}

bool WebSetupServer::start() {
    if (!mappingStore || !sdScanner || !rfidManager) {
        LOG_ERROR("[WEB-SETUP] Not initialized");
        return false;
    }

    if (active) {
        return true;
    }

    // Load mappings and folders
    if (!mappingStore->loadAll()) {
        LOG_ERROR("[WEB-SETUP] Failed to load mapping store");
        return false;
    }
    refreshFolders();

    // Bring up AP
    WiFi.mode(WIFI_AP);
    if (!WiFi.softAP(kApSsid)) {
        LOG_ERROR("[WEB-SETUP] Failed to start softAP");
        return false;
    }

    // Disable audio control during setup
    rfidManager->enableAudioControl(false);

    registerRoutes();
    server.begin();
    active = true;
    waitingForTag = false;
    selectedFolder = "";
    lastUid = "";

    LOG_INFO("[WEB-SETUP] SoftAP '%s' started, web server running", kApSsid);
    return true;
}

void WebSetupServer::stop() {
    if (!active) return;

    server.stop();
    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_OFF);

    waitingForTag = false;
    selectedFolder = "";
    lastUid = "";
    unassignedFolders.clear();
    active = false;

    // Re-enable audio control
    rfidManager->enableAudioControl(true);

    LOG_INFO("[WEB-SETUP] Stopped web setup and AP");
}

void WebSetupServer::loop() {
    if (!active) return;
    server.handleClient();
}

void WebSetupServer::registerRoutes() {
    server.on("/", HTTP_GET, [this]() { handleRoot(); });
    server.on("/folders", HTTP_GET, [this]() { handleFolders(); });
    server.on("/select", HTTP_POST, [this]() { handleSelect(); });
    server.on("/tag", HTTP_GET, [this]() { handleTag(); });
    server.on("/assign", HTTP_POST, [this]() { handleAssign(); });
    server.on("/reassign", HTTP_POST, [this]() { handleReassign(); });
    server.on("/done", HTTP_POST, [this]() { handleDone(); });
}

void WebSetupServer::sendJson(int statusCode, const String& body) {
    server.send(statusCode, "application/json", body);
}

void WebSetupServer::handleRoot() {
    server.send_P(200, "text/html", kIndexHtml);
}

void WebSetupServer::handleFolders() {
    refreshFolders();
    String json = "{\"folders\":[";
    for (size_t i = 0; i < unassignedFolders.size(); ++i) {
        json += "\"" + unassignedFolders[i] + "\"";
        if (i + 1 < unassignedFolders.size()) json += ",";
    }
    json += "]}";
    sendJson(200, json);
}

void WebSetupServer::handleSelect() {
    String folder = server.arg("folder");
    if (folder.isEmpty()) {
        sendJson(400, "{\"error\":\"folder required\"}");
        return;
    }
    selectedFolder = folder;
    waitingForTag = true;
    lastUid = "";
    sendJson(200, "{\"status\":\"waiting_tag\"}");
}

void WebSetupServer::handleTag() {
    // Ensure latest RFID state even if main loop is busy serving clients
    if (rfidManager) {
        rfidManager->update();
    }

    if (!waitingForTag) {
        sendJson(200, "{\"status\":\"no_selection\"}");
        return;
    }

    if (rfidManager->isTagPresent()) {
        String uid = rfidManager->getLastDetectedUIDString();
        if (uid.length() > 0) {
            lastUid = uid;
            String resp = "{\"status\":\"tag_detected\",\"uid\":\"" + uid + "\"}";
            sendJson(200, resp);
            return;
        }
    }
    sendJson(200, "{\"status\":\"waiting\"}");
}

bool WebSetupServer::normalizeUid(String& uid) const {
    if (uid.isEmpty()) return false;
    uid.toUpperCase();
    uid.replace(":", "");
    uid.replace(" ", "");
    return !uid.isEmpty();
}

bool WebSetupServer::appendMapping(const String& uid, const String& folder) {
    Mapping m(uid, folder);
    return mappingStore->append(m);
}

bool WebSetupServer::reassignMapping(const String& uid, const String& folder, String& previous) {
    if (!mappingStore->getPathFor(uid, previous)) {
        return false;
    }
    return mappingStore->rebind(uid, folder);
}

void WebSetupServer::handleAssign() {
    String uid = server.arg("uid");
    String folder = server.arg("folder");
    bool force = server.arg("force") == "1";

    if (uid.isEmpty() || folder.isEmpty()) {
        sendJson(400, "{\"error\":\"uid and folder required\"}");
        return;
    }

    if (!normalizeUid(uid)) {
        sendJson(400, "{\"error\":\"invalid uid\"}");
        return;
    }

    String existingPath;
    if (mappingStore->getPathFor(uid, existingPath)) {
        if (existingPath == folder) {
            waitingForTag = false;
            sendJson(200, "{\"status\":\"already_assigned_same\"}");
            return;
        }
        if (!force) {
            String resp = "{\"status\":\"conflict\",\"folder\":\"" + existingPath + "\"}";
            sendJson(200, resp);
            return;
        }
        String previous;
        if (reassignMapping(uid, folder, previous)) {
            waitingForTag = false;
            sendJson(200, "{\"status\":\"reassigned\",\"previous\":\"" + previous + "\"}");
        } else {
            sendJson(500, "{\"error\":\"reassign failed\"}");
        }
        return;
    }

    if (appendMapping(uid, folder)) {
        waitingForTag = false;
        sendJson(200, "{\"status\":\"assigned\"}");
    } else {
        sendJson(500, "{\"error\":\"append failed\"}");
    }
}

void WebSetupServer::handleReassign() {
    String uid = server.arg("uid");
    String folder = server.arg("folder");
    if (uid.isEmpty() || folder.isEmpty()) {
        sendJson(400, "{\"error\":\"uid and folder required\"}");
        return;
    }
    if (!normalizeUid(uid)) {
        sendJson(400, "{\"error\":\"invalid uid\"}");
        return;
    }

    String previous;
    if (reassignMapping(uid, folder, previous)) {
        waitingForTag = false;
        sendJson(200, "{\"status\":\"reassigned\",\"previous\":\"" + previous + "\"}");
    } else {
        sendJson(500, "{\"error\":\"reassign failed\"}");
    }
}

void WebSetupServer::handleDone() {
    sendJson(200, "{\"status\":\"ok\"}");
    stop();
}

void WebSetupServer::refreshFolders() {
    std::vector<String> allPaths;
    if (!sdScanner->listAudioDirs(SD_MMC, contentRoot, allPaths)) {
        LOG_ERROR("[WEB-SETUP] Failed to list audio dirs");
        unassignedFolders.clear();
        return;
    }

    unassignedFolders.clear();
    for (const String& path : allPaths) {
        String dummy;
        if (!mappingStore->getUidFor(path, dummy)) {
            unassignedFolders.push_back(path);
        }
    }
}

