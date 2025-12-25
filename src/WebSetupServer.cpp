#include "WebSetupServer.h"
#include "Logger.h"
#include "Settings_Manager.h"
#include "Battery_Manager.h"
#include <ArduinoJson.h>

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
.topline { display: flex; align-items: center; justify-content: space-between; gap: 10px; }
.battery { padding: 8px 10px; border-radius: 10px; border: 1px solid var(--stroke); background: rgba(255,255,255,.06); font-weight: 800; font-size: 13px; color: var(--text); min-width: 86px; text-align: center; box-shadow: inset 0 0 0 1px rgba(255,255,255,.03); }
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
.btn-link { background: rgba(255,255,255,.04); color: var(--text); border: 1px solid var(--stroke); box-shadow: 0 8px 16px rgba(0,0,0,.12); }
.btn-danger { background: linear-gradient(135deg, #ff5f6d, #c70039); color: #fff; box-shadow: 0 10px 20px rgba(255,95,109,.35); border: 1px solid rgba(255,255,255,.1); }
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
    <div style="flex:1">
      <div class="badge">Radio Gaga Setup</div>
      <div class="topline">
        <h1>Choose folder & tag</h1>
        <div class="battery" id="battery">--%</div>
      </div>
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
  <button class="btn-link" id="settingsBtn">Settings</button>
  <button class="btn-danger" id="exitBtn">Exit Web Setup</button>
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
async function B(){
  try{
    const e=await fetch("/api/battery"),n=await e.json(),d=document.getElementById("battery");
    if(n.status==="ok"&&typeof n.percentage==="number"){d.innerText=`${n.percentage.toFixed(0)}%`;d.title=n.voltage?`Voltage: ${n.voltage.toFixed(2)}V`:"";}
    else{d.innerText="N/A";}
  }catch(e){document.getElementById("battery").innerText="N/A";}
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
document.getElementById("settingsBtn").onclick=()=>{window.location.href="/settings";};
document.getElementById("exitBtn").onclick=async()=>{
  S("Exiting setup...");
  try{
    await fetch("/done",{method:"POST"});
    S("Stopping web setup and WiFi...");
  }catch(e){
    S("Stopping web setup...");
  }
};
L();
B();setInterval(B,10000);
</script>
</body>
</html>
)HTML";

// Settings UI page
static const char* kSettingsHtml PROGMEM = R"HTML(
<!DOCTYPE html>
<html>
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1.0">
<title>Radio Gaga Settings</title>
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
.wrap { width: 100%; max-width: 520px; padding: 20px 16px 32px; }
.header { display: flex; align-items: center; gap: 12px; margin-bottom: 14px; }
.logo {
  width: 44px; height: 44px; border-radius: 12px;
  background: linear-gradient(135deg, var(--accent), var(--accent-2));
  display: flex; align-items: center; justify-content: center;
  font-weight: 800; color: #0a1633;
  box-shadow: 0 6px 20px rgba(0,0,0,0.28), 0 0 0 2px rgba(10,16,35,0.6);
}
h1 { margin: 0; font-size: 20px; letter-spacing: .2px; }
.sub { color: var(--muted); font-size: 13px; margin-top: 2px; }
.card { background: var(--card); border: 1px solid var(--stroke); border-radius: 14px; padding: 14px; box-shadow: 0 12px 28px rgba(0,0,0,.25); backdrop-filter: blur(6px); margin-bottom: 14px; }
.label { font-size: 13px; color: var(--muted); margin: 0 0 6px; font-weight: 700; letter-spacing: .2px; }
.field { display: flex; flex-direction: column; gap: 8px; margin-bottom: 12px; }
.field input[type="text"],
.field input[type="password"],
.field input[type="number"] {
  padding: 10px 12px;
  border-radius: 10px;
  border: 1px solid var(--stroke);
  background: rgba(255,255,255,.06);
  color: var(--text);
  font-size: 14px;
}
.slider-row { display: flex; align-items: center; gap: 10px; }
input[type="range"] { flex: 1; accent-color: var(--primary); }
.value-pill { min-width: 54px; text-align: center; padding: 8px 10px; border-radius: 10px; border: 1px solid var(--stroke); background: rgba(255,255,255,.06); font-weight: 800; }
.actions { display: flex; gap: 10px; margin-top: 12px; }
button { border: none; border-radius: 10px; padding: 12px 14px; font-size: 15px; font-weight: 800; cursor: pointer; transition: transform .08s ease, box-shadow .12s ease; width: 100%; letter-spacing: .2px; }
button:active { transform: translateY(1px); }
.btn-primary { background: linear-gradient(135deg, var(--primary), var(--primary-dark)); color: #001429; box-shadow: 0 10px 20px rgba(10,160,255,.35); }
.btn-ghost { background: rgba(255,255,255,.1); color: var(--text); border: 1px solid var(--stroke); }
.status { font-size: 14px; line-height: 1.4; color: var(--text); padding: 10px 12px; border-radius: 10px; background: rgba(255,255,255,.06); border: 1px solid var(--stroke); min-height: 42px; box-shadow: inset 0 0 0 1px rgba(255,255,255,.03); }
.row { display: grid; grid-template-columns: repeat(auto-fit,minmax(220px,1fr)); gap: 10px; }
@media (max-width: 480px) { .wrap { padding: 16px 12px 26px; } }
</style>
</head>
<body>
<div class="wrap">
  <div class="header">
    <div class="logo">RG</div>
    <div>
      <h1>Settings</h1>
      <div class="sub">Adjust device defaults and limits</div>
    </div>
  </div>
  <div class="card">
    <div class="field">
      <div class="label">Default volume</div>
      <div class="slider-row">
        <input type="range" id="defaultVolume" min="0" max="1" step="0.01">
        <div class="value-pill" id="defaultVolumeValue">--</div>
      </div>
    </div>
    <div class="field">
      <div class="label">Max volume</div>
      <div class="slider-row">
        <input type="range" id="maxVolume" min="0" max="1" step="0.01">
        <div class="value-pill" id="maxVolumeValue">--</div>
      </div>
    </div>
  </div>
  <div class="card">
    <div class="row">
      <div class="field">
        <div class="label">WiFi SSID</div>
        <input type="text" id="wifiSSID" placeholder="Network name">
      </div>
      <div class="field">
        <div class="label">WiFi Password</div>
        <input type="password" id="wifiPassword" placeholder="Password">
      </div>
    </div>
    <div class="row">
      <div class="field">
        <div class="label">Sleep timeout (minutes)</div>
        <input type="number" id="sleepTimeout" min="1" max="1440">
      </div>
      <div class="field">
        <div class="label">Battery check interval (minutes)</div>
        <input type="number" id="batteryCheckInterval" min="1" max="60">
      </div>
    </div>
  </div>
  <div class="card">
    <div class="label">Status</div>
    <div class="status" id="status">Loading...</div>
    <div class="actions">
      <button class="btn-ghost" id="backBtn">Back</button>
      <button class="btn-primary" id="saveBtn">Save settings</button>
    </div>
  </div>
</div>
<script>
const statusEl=document.getElementById("status");
const inputs={
  defaultVolume:document.getElementById("defaultVolume"),
  maxVolume:document.getElementById("maxVolume"),
  wifiSSID:document.getElementById("wifiSSID"),
  wifiPassword:document.getElementById("wifiPassword"),
  sleepTimeout:document.getElementById("sleepTimeout"),
  batteryCheckInterval:document.getElementById("batteryCheckInterval")
};
const pills={
  defaultVolume:document.getElementById("defaultVolumeValue"),
  maxVolume:document.getElementById("maxVolumeValue")
};
function setStatus(msg){statusEl.innerText=msg;}
function clamp(v,min,max){return Math.max(min,Math.min(max,v));}
function bindSliders(){
  inputs.defaultVolume.oninput=()=>{
    const max=parseFloat(inputs.maxVolume.value||1);
    inputs.defaultVolume.value=clamp(parseFloat(inputs.defaultVolume.value||0),0,max);
    pills.defaultVolume.innerText=(parseFloat(inputs.defaultVolume.value)*100).toFixed(0)+"%";
  };
  inputs.maxVolume.oninput=()=>{
    inputs.maxVolume.value=clamp(parseFloat(inputs.maxVolume.value||1),0,1);
    if(parseFloat(inputs.defaultVolume.value)>parseFloat(inputs.maxVolume.value)){
      inputs.defaultVolume.value=inputs.maxVolume.value;
    }
    pills.maxVolume.innerText=(parseFloat(inputs.maxVolume.value)*100).toFixed(0)+"%";
    pills.defaultVolume.innerText=(parseFloat(inputs.defaultVolume.value)*100).toFixed(0)+"%";
  };
}
async function loadSettings(){
  setStatus("Loading...");
  try{
    const res=await fetch("/api/settings");
    const data=await res.json();
    if(data.error){setStatus(data.error);return;}
    inputs.defaultVolume.value=data.defaultVolume ?? 0.2;
    inputs.maxVolume.value=data.maxVolume ?? 1.0;
    inputs.wifiSSID.value=data.wifiSSID || "";
    inputs.wifiPassword.value=data.wifiPassword || "";
    inputs.sleepTimeout.value=data.sleepTimeout ?? 15;
    inputs.batteryCheckInterval.value=data.batteryCheckInterval ?? 1;
    pills.defaultVolume.innerText=(parseFloat(inputs.defaultVolume.value)*100).toFixed(0)+"%";
    pills.maxVolume.innerText=(parseFloat(inputs.maxVolume.value)*100).toFixed(0)+"%";
    setStatus("Ready.");
  }catch(err){
    setStatus("Failed to load settings.");
  }
}
async function saveSettings(){
  setStatus("Saving...");
  const payload={
    defaultVolume:parseFloat(inputs.defaultVolume.value||0),
    maxVolume:parseFloat(inputs.maxVolume.value||1),
    wifiSSID:inputs.wifiSSID.value||"",
    wifiPassword:inputs.wifiPassword.value||"",
    sleepTimeout:parseInt(inputs.sleepTimeout.value||0,10),
    batteryCheckInterval:parseInt(inputs.batteryCheckInterval.value||0,10)
  };
  try{
    const res=await fetch("/api/settings",{method:"POST",headers:{"Content-Type":"application/json"},body:JSON.stringify(payload)});
    const data=await res.json();
    if(data.status==="ok"){setStatus("Settings saved.");}
    else{setStatus(data.error||"Save failed.");}
  }catch(err){
    setStatus("Save failed.");
  }
}
document.getElementById("saveBtn").onclick=saveSettings;
document.getElementById("backBtn").onclick=()=>{window.location.href="/";};
bindSliders();
loadSettings();
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
      rfidManager(nullptr),
      settingsManager(nullptr),
      batteryManager(nullptr) {}

bool WebSetupServer::begin(MappingStore* store, SdScanner* scanner, RFID_Manager* rfid, const String& root, Settings_Manager* settings, Battery_Manager* battery) {
    mappingStore = store;
    sdScanner = scanner;
    rfidManager = rfid;
    contentRoot = root;
    settingsManager = settings;
    batteryManager = battery;

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
    server.on("/settings", HTTP_GET, [this]() { handleSettingsPage(); });
    server.on("/api/settings", HTTP_GET, [this]() { handleSettingsJson(); });
    server.on("/api/settings", HTTP_POST, [this]() { handleSettingsSave(); });
    server.on("/api/battery", HTTP_GET, [this]() { handleBattery(); });
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

void WebSetupServer::handleSettingsPage() {
    server.send_P(200, "text/html", kSettingsHtml);
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
    // Give the response a brief moment to flush before stopping AP
    delay(150);
    stop();
}

void WebSetupServer::handleBattery() {
    StaticJsonDocument<128> doc;
    if (!batteryManager || !batteryManager->isInitialized()) {
        doc["status"] = "unavailable";
    } else {
        doc["status"] = "ok";
        doc["percentage"] = batteryManager->getBatteryPercentage();
        doc["voltage"] = batteryManager->getBatteryVoltage();
    }
    String body;
    serializeJson(doc, body);
    sendJson(200, body);
}

void WebSetupServer::handleSettingsJson() {
    StaticJsonDocument<256> doc;
    if (!settingsManager) {
        doc["error"] = "Settings unavailable";
        String body;
        serializeJson(doc, body);
        sendJson(500, body);
        return;
    }

    const Settings& s = settingsManager->getSettings();
    doc["defaultVolume"] = s.defaultVolume;
    doc["maxVolume"] = s.maxVolume;
    doc["wifiSSID"] = s.wifiSSID;
    doc["wifiPassword"] = s.wifiPassword;
    doc["sleepTimeout"] = s.sleepTimeout;
    doc["batteryCheckInterval"] = s.batteryCheckInterval;

    String body;
    serializeJson(doc, body);
    sendJson(200, body);
}

void WebSetupServer::handleSettingsSave() {
    StaticJsonDocument<256> errorDoc;
    if (!settingsManager) {
        errorDoc["error"] = "Settings unavailable";
        String body; serializeJson(errorDoc, body);
        sendJson(500, body);
        return;
    }

    StaticJsonDocument<512> doc;
    DeserializationError err = deserializeJson(doc, server.arg("plain"));
    if (err) {
        errorDoc["error"] = "Invalid JSON payload";
        String body; serializeJson(errorDoc, body);
        sendJson(400, body);
        return;
    }

    Settings next = settingsManager->getSettings();
    if (doc.containsKey("defaultVolume")) {
        next.defaultVolume = doc["defaultVolume"];
    }
    if (doc.containsKey("maxVolume")) {
        next.maxVolume = doc["maxVolume"];
    }
    if (doc.containsKey("wifiSSID")) {
        const char* ssid = doc["wifiSSID"] | "";
        strncpy(next.wifiSSID, ssid, sizeof(next.wifiSSID) - 1);
        next.wifiSSID[sizeof(next.wifiSSID) - 1] = '\0';
    }
    if (doc.containsKey("wifiPassword")) {
        const char* pwd = doc["wifiPassword"] | "";
        strncpy(next.wifiPassword, pwd, sizeof(next.wifiPassword) - 1);
        next.wifiPassword[sizeof(next.wifiPassword) - 1] = '\0';
    }
    if (doc.containsKey("sleepTimeout")) {
        next.sleepTimeout = doc["sleepTimeout"];
    }
    if (doc.containsKey("batteryCheckInterval")) {
        next.batteryCheckInterval = doc["batteryCheckInterval"];
    }

    settingsManager->updateSettings(next);
    if (!settingsManager->validateSettings()) {
        errorDoc["error"] = "Validation failed";
        String body; serializeJson(errorDoc, body);
        sendJson(400, body);
        return;
    }

    if (!settingsManager->saveSettings()) {
        errorDoc["error"] = "Save failed";
        String body; serializeJson(errorDoc, body);
        sendJson(500, body);
        return;
    }

    StaticJsonDocument<64> okDoc;
    okDoc["status"] = "ok";
    String body; serializeJson(okDoc, body);
    sendJson(200, body);
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

