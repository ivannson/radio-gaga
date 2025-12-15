#include "WebSetupServer.h"
#include "Logger.h"

static const char* kApSsid = "setup";   // open network as requested

// Single-page UI with vibrant Game Boy Color-inspired palette
static const char* kIndexHtml PROGMEM = R"HTML(
<!DOCTYPE html>
<html>
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1.0">
<title>Radio Gaga Setup</title>
<style>
:root {
  --bg1: #0b122c;
  --bg2: #193e7a;
  --card: rgba(8, 12, 28, 0.9);
  --stroke: rgba(255, 255, 255, 0.12);
  --text: #f6f8ff;
  --muted: #c2cffc;
  --primary: #1ee7ff;
  --primary-dark: #0aa0ff;
  --accent: #7aff59;
  --accent-2: #ff7af5;
}
* { box-sizing: border-box; }
body {
  margin: 0;
  font-family: "Inter", system-ui, -apple-system, sans-serif;
  background:
    radial-gradient(1200px at 12% 18%, rgba(255,122,245,0.12), transparent 55%),
    radial-gradient(900px at 88% 10%, rgba(122,255,89,0.10), transparent 52%),
    linear-gradient(135deg, var(--bg1), var(--bg2));
  color: var(--text);
  min-height: 100vh;
  display: flex;
  justify-content: center;
}
.wrap { width: 100%; max-width: 480px; padding: 18px 16px 28px; }
.header { display: flex; align-items: center; gap: 10px; margin-bottom: 14px; }
.logo {
  width: 42px; height: 42px; border-radius: 12px;
  background: linear-gradient(135deg, var(--accent), var(--accent-2));
  display: flex; align-items: center; justify-content: center;
  font-weight: 800; color: #0a1633;
  box-shadow: 0 6px 20px rgba(0,0,0,0.28), 0 0 0 2px rgba(10,16,35,0.6);
}
h1 { margin: 0; font-size: 19px; letter-spacing: .2px; }
.badge {
  background: linear-gradient(135deg, var(--primary), var(--accent));
  color: #081025;
  padding: 4px 10px; border-radius: 999px; font-size: 12px; font-weight: 800;
  box-shadow: 0 6px 12px rgba(0,0,0,0.18);
}
.card { background: var(--card); border: 1px solid var(--stroke); border-radius: 14px; padding: 14px; box-shadow: 0 12px 28px rgba(0,0,0,.25); backdrop-filter: blur(6px); margin-bottom: 14px; }
.label { font-size: 13px; color: var(--muted); margin: 0 0 6px; }
.folder { display: flex; align-items: center; justify-content: space-between; padding: 12px 12px; border-radius: 12px; border: 1px solid var(--stroke); background: rgba(255,255,255,.05); color: var(--text); margin-bottom: 10px; cursor: pointer; transition: transform .08s ease, box-shadow .12s ease, border-color .12s ease; border-left: 4px solid var(--accent); }
.folder:hover { transform: translateY(-2px); box-shadow: 0 10px 18px rgba(0,0,0,.25); border-color: rgba(122,255,89,0.5); }
.folder-title { font-weight: 800; font-size: 15px; line-height: 1.2; letter-spacing: .2px; }
.folder-arrow { color: var(--muted); font-size: 14px; }
.status { font-size: 14px; line-height: 1.4; color: var(--text); padding: 10px 12px; border-radius: 10px; background: rgba(255,255,255,.06); border: 1px solid var(--stroke); min-height: 42px; box-shadow: inset 0 0 0 1px rgba(255,255,255,.03); }
.actions { display: flex; gap: 10px; margin-top: 10px; }
button { border: none; border-radius: 10px; padding: 12px 14px; font-size: 15px; font-weight: 800; cursor: pointer; transition: transform .08s ease, box-shadow .12s ease; width: 100%; letter-spacing: .2px; }
button:active { transform: translateY(1px); }
.btn-primary { background: linear-gradient(135deg, var(--primary), var(--primary-dark)); color: #001429; box-shadow: 0 10px 20px rgba(10,160,255,.35); }
.btn-ghost { background: rgba(255,255,255,.1); color: var(--text); border: 1px solid var(--stroke); }
.hidden { display: none; }
#modal { position: fixed; inset: 0; background: rgba(5,10,24,.7); display: none; align-items: center; justify-content: center; padding: 16px; }
#modalContent { background: var(--card); border: 1px solid var(--stroke); border-radius: 14px; padding: 16px; max-width: 360px; width: 100%; box-shadow: 0 16px 30px rgba(0,0,0,.35); }
#modalText { font-size: 15px; color: var(--text); margin-bottom: 12px; }
#modalActions { display: flex; gap: 10px; }
@media (max-width: 480px) {
  .wrap { padding: 16px 12px 24px; }
  .folder { padding: 12px 10px; }
  button { font-size: 14px; }
}
</style>
</head>
<body>
<div class="wrap">
  <div class="header">
    <div class="logo">RG</div>
    <div>
      <div class="badge">Radio Gaga Setup</div>
      <h1>Choose folder & tag</h1>
    </div>
  </div>
  <div class="card">
    <div class="label">Folders</div>
    <div id="folders"></div>
  </div>
  <div class="card">
    <div class="label">Status</div>
    <div class="status" id="status">Pick a folder to begin.</div>
    <div class="actions hidden" id="actions">
      <button class="btn-primary" id="doneBtn">Done</button>
    </div>
  </div>
</div>
<div id="modal">
  <div id="modalContent">
    <div id="modalText"></div>
    <div id="modalActions">
      <button class="btn-primary" id="reassignBtn">Reassign</button>
      <button class="btn-ghost" id="cancelBtn">Cancel</button>
    </div>
  </div>
</div>
<script>
let t=null,f="",u="";
const S=e=>document.getElementById("status").innerText=e,
      A=e=>document.getElementById("actions").classList.toggle("hidden",!e),
      M=e=>{document.getElementById("modalText").innerText=e;document.getElementById("modal").style.display="flex"},
      C=()=>document.getElementById("modal").style.display="none";
async function L(){
  const e=await fetch("/folders"),n=await e.json(),d=document.getElementById("folders");
  if(d.innerHTML="",!n.folders||!n.folders.length){d.innerHTML='<div class="status">No unassigned folders found.</div>';A(!0);return;}
  n.folders.forEach(e=>{
    const n=document.createElement("div");
    n.className="folder";
    n.innerHTML='<span class="folder-title">'+e+'</span><span class="folder-arrow">›</span>';
    n.onclick=()=>E(e);
    d.appendChild(n);
  });
}
async function E(e){
  await fetch("/select",{method:"POST",headers:{"Content-Type":"application/x-www-form-urlencoded"},body:"folder="+encodeURIComponent(e)});
  f=e;S("Waiting for tag for "+e+"...");A(!1);g();
}
function g(){t&&clearInterval(t);t=setInterval(T,600);}
async function T(){
  const e=await fetch("/tag"),n=await e.json();
  if("tag_detected"===n.status){u=n.uid;clearInterval(t);S("Tag "+u+" detected, assigning...");y(!1);}
}
async function y(e){
  const n=`uid=${encodeURIComponent(u)}&folder=${encodeURIComponent(f)}&force=${e?"1":"0"}`,
        d=await fetch("/assign",{method:"POST",headers:{"Content-Type":"application/x-www-form-urlencoded"},body:n}),
        o=await d.json();
  if("assigned"===o.status||"already_assigned_same"===o.status){S("Assigned to "+f+".");A(!0);}
  else if("conflict"===o.status){M("This cassette is already assigned to "+o.folder+". Reassign?");}
  else {S("Error: "+(o.message||"unknown"));A(!0);}
}
document.getElementById("reassignBtn").onclick=async()=>{
  C();
  const e=`uid=${encodeURIComponent(u)}&folder=${encodeURIComponent(f)}`,
        n=await fetch("/reassign",{method:"POST",headers:{"Content-Type":"application/x-www-form-urlencoded"},body:e}),
        d=await n.json();
  "reassigned"===d.status?S("Reassigned to "+f+"."):S("Reassign failed: "+(d.message||"unknown"));
  A(!0);
};
document.getElementById("cancelBtn").onclick=async()=>{
  C();S("Choose another folder.");u="";await L();
};
document.getElementById("doneBtn").onclick=async()=>{
  await fetch("/done",{method:"POST"});S("Done. You can close this page.");
};
L();
</script>
</body>
</html>
)HTML";

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

