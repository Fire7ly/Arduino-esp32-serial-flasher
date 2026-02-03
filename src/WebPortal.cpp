#include "WebPortal.h"
#include "WebPortal.h"
// #include <SPIFFS.h> // Handled by SDStorage.h based on config
#include "ConfigFile.h"
#include <ArduinoJson.h>
#include "FlasherTask.h"
#include "OTAUpdate.h"

// OTA State
static bool shouldUpdateFirmware = false;
static String updateFirmwareUrl = "";

// Note: Ensure ArduinoJson is installed
// If using V6, DynamicJsonDocument doc(1024);

AsyncWebServer server(80);
WebPortal WebManager;

// Advanced UI with Multi-File Flashing Support
// Main Flasher Page
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <title>ESP32 Advanced Flasher</title>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body { font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; background: #f0f2f5; color: #333; margin: 0; padding: 20px; }
    h2 { color: #007bff; margin-bottom: 20px; text-align: center; } /* Also centered title for balance */
    .container { max-width: 800px; margin: auto; background: white; padding: 30px; border-radius: 12px; box-shadow: 0 4px 20px rgba(0,0,0,0.1); }
    .section { margin-bottom: 30px; border-bottom: 1px solid #eee; padding-bottom: 20px; }
    label { font-weight: 600; display: block; margin-bottom: 5px; }
    select, input[type=text] { width: 100%; padding: 10px; border: 1px solid #ddd; border-radius: 6px; margin-bottom: 10px; box-sizing: border-box; }
    button { padding: 10px 20px; font-size: 16px; cursor: pointer; background-color: #007bff; color: white; border: none; border-radius: 6px; transition: background 0.3s; }
    button:hover { background-color: #0056b3; }
    #status { margin-top: 20px; padding: 15px; background: #e9ecef; border-radius: 6px; font-weight: bold; border-left: 5px solid #007bff; }
    .row-inputs { display: flex; gap: 10px; align-items: center; }
    .row-inputs select { flex-grow: 1; }
  </style>
</head>
<body>
  <div class="container">
    <h2>ESP32 Advanced Web Flasher</h2>
    
    <div id="notificationArea" style="display:none; background:#ffc107; color:#333; padding:15px; border-radius:6px; margin-bottom:20px; text-align:center; border:1px solid #d39e00;">
       <!-- Notification Content -->
    </div>
    
    <div class="section" style="text-align:center;">
        <a href="/files"><button style="width:100%; max-width: 300px;">Open File Manager 📂</button></a>
    </div>

    <!-- Target Selection -->
    <div class="section">
      <label>Target Chip:</label>
      <select id="targetChip">
        <option value="esp32">ESP32</option>
        <option value="esp32s3">ESP32-S3</option>
        <option value="esp8266">ESP8266</option>
        <option value="esp32s2">ESP32-S2</option>
        <option value="esp32c3">ESP32-C3</option>
      </select>
    </div>

    <!-- Flash Composition -->
    <div class="section">
      <h3>Flash Composition</h3>
      <div id="flashContainer"></div>
    </div>

    <!-- Actions -->
    <div class="section">
      <button onclick="startFlash()">Start Flashing</button>
      <div id="status">Status: Ready</div>
    </div>
    
    <!-- System Logs -->
    <div class="section">
      <h3>Activity Log</h3>
      <textarea id="sysLoop" rows="10" style="width:100%; font-family:monospace; font-size:12px; resize:vertical;" readonly></textarea>
    </div>

    <!-- System Upgrade -->
    <div class="section">
      <h3>System Upgrade</h3>
      <button onclick="checkForUpdate()">Check for Updates 🔄</button>
      <div id="updateStatus" style="margin-top:10px;"></div>
    </div>
  </div>

<script>
  let availableFiles = [];
  let lastStatus = "";
  let lastLogIndex = 0;

  function log(msg) {
    const box = document.getElementById('sysLoop');
    const time = new Date().toLocaleTimeString();
    box.value += `[${time}] ${msg}\n`;
    box.scrollTop = box.scrollHeight;
  }

  function fetchLogs() {
    fetch('/logs?index=' + lastLogIndex)
        .then(res => res.json())
        .then(data => {
            if(data.logs && data.logs.length > 0) {
                data.logs.forEach(l => log(l));
                lastLogIndex = data.nextIndex;
            }
        })
        .catch(e => console.log("Log poll error", e));
  }
  
  // Poll logs frequently (500ms)
  setInterval(fetchLogs, 500);

  fetch('/list').then(res => res.json()).then(data => {
    availableFiles = data;
    updateForm();
  });

  document.getElementById('targetChip').addEventListener('change', () => updateForm());

  function updateForm() {
    const chip = document.getElementById('targetChip').value;
    const container = document.getElementById('flashContainer');
    container.innerHTML = ''; 

    let slots = [];
    if (chip === 'esp8266') {
        slots = [{ label: 'Firmware', addr: '0x0' }];
    } else {
        slots = [
            { label: 'Partition Table', addr: '0x8000' },
            { label: 'Firmware', addr: '0x10000' }
        ];
    }

    let fileOptions = '<option value="">-- Select File --</option>';
    if (availableFiles.length === 0) fileOptions += '<option value="" disabled>(No .bin files found)</option>';
    
    availableFiles.forEach(f => {
       fileOptions += `<option value="${f.name}">${f.name}</option>`;
    });

    slots.forEach((slot, index) => {
        const div = document.createElement('div');
        div.className = 'row-inputs';
        div.style.marginBottom = '10px';
        div.innerHTML = `
            <div style="width: 151px; font-weight:bold;">${slot.label}:</div>
            <input type="hidden" class="addrInput" value="${slot.addr}">
            <select class="fileInput">${fileOptions}</select>
        `;
        container.appendChild(div);
    });
  }

  function startFlash() {
    const chip = document.getElementById('targetChip').value;
    const inputs = document.querySelectorAll('#flashContainer .row-inputs');
    const files = [];

    inputs.forEach(div => {
      const addrStr = div.querySelector('.addrInput').value;
      const fileName = div.querySelector('.fileInput').value;
      if(fileName) files.push({ name: fileName, address: addrStr });
    });

    if(chip === 'esp8266') {
        if(files.length !== 1) { alert("Please select the firmware file."); return; }
    } else {
        if(files.length !== 2) { alert("Please select both Partition Table and Firmware files."); return; }
    }

    if(!confirm(`Flash ${files.length} files to ${chip}?`)) return;

    log("Sending Flash Request...");
    document.getElementById('status').innerText = 'Starting Flash...';

    fetch('/flash', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ target: chip, files: files })
    })
    .then(res => res.text())
    .then(msg => log("Server: " + msg))
    .catch(err => log("Error: " + err));
  }
  
  setInterval(() => {
    fetch('/status').then(res => res.text()).then(txt => {
       if(txt !== lastStatus) {
         log(txt);
         if(!txt.startsWith("Upload")) {
            document.getElementById('status').innerText = txt;
         }
         lastStatus = txt;
       }
    });
  }, 1000);

  // Check for notification on load
  function checkNotification() {
      fetch('/update_check?cached=true') // Check without forcing a new request immediately if possible
        .then(res => res.json())
        .then(data => {
          if(data.available) {
              const notif = document.getElementById('notificationArea');
              notif.style.display = 'block';
              notif.innerHTML = `
                  <strong>New Update Available! (${data.version})</strong>
                  <button onclick="performUpdate('${data.url}')" style="margin-left:15px; background:white; color:#333; border:none; padding:5px 10px; cursor:pointer;">Update Now</button>
              `;
          }
      });
  }
  
  // Call on load
  setTimeout(checkNotification, 2000);

  function checkForUpdate() {
      const statusDiv = document.getElementById('updateStatus');
      statusDiv.innerText = "Checking...";
      fetch('/update_check').then(res => res.json()).then(data => {
          if(data.error && data.error.length > 0) {
              statusDiv.innerText = "Update Failed: " + data.error + " \u274C";
          } else if(data.available) {
              statusDiv.innerHTML = 
                  `New version <strong>${data.version}</strong> available!<br>
                   <small>${data.notes}</small><br>
                   <button onclick="performUpdate('${data.url}')" style="background:#28a745; margin-top:5px;">Update Now \uD83D\uDE80</button>`;
          } else {
              statusDiv.innerText = "Firmware is up to date (" + data.version + ")";
          }
      }).catch(err => {
         statusDiv.innerText = "Error checking update: " + err;
      });
  }

  function performUpdate(url) {
      if(!confirm("Start Update? Device will reboot.")) return;
       document.getElementById('updateStatus').innerText = "Starting Update... Please Wait..."; // Also show here
       fetch('/update_perform', {
           method: 'POST',
           headers: {'Content-Type': 'application/x-www-form-urlencoded'},
           body: 'url=' + encodeURIComponent(url)
       }).then(res => res.text()).then(txt => {
           log("Update: " + txt);
       });
  }
</script>
</body>
</html>
)rawliteral";

// File Manager Page
const char files_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <title>ESP32 File Manager</title>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body { font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; margin: 0; padding: 0; background: #fff; height: 100vh; display: flex; flex-direction: column; }
    .container { flex: 1; display: flex; flex-direction: column; padding: 20px; width: 100%; box-sizing: border-box; }
    
    .header { display: flex; justify-content: space-between; align-items: center; margin-bottom: 20px; padding-bottom: 15px; border-bottom: 1px solid #eee; }
    h2 { color: #007bff; margin: 0; font-size: 24px; }
    
    /* Upload Section */
    .upload-section { 
        background: #f8f9fa; 
        padding: 20px; 
        border-radius: 12px; 
        border: 1px solid #e9ecef; 
        margin-bottom: 20px; 
        text-align: center;
        display: flex;
        flex-direction: column;
        align-items: center;
        justify-content: center;
        min-height: 100px;
    }
    
    .btn-choose {
        display: inline-block;
        padding: 12px 24px;
        background-color: #007bff;
        color: white;
        border-radius: 8px;
        font-weight: 600;
        cursor: pointer;
        transition: background 0.2s, transform 0.1s;
        font-size: 16px;
    }
    .btn-choose:active { transform: scale(0.98); }
    
    .upload-confirm {
        display: none; /* Hidden by default */
        flex-direction: column;
        align-items: center;
        width: 100%;
        gap: 15px;
    }
    
    .file-name-display {
        font-weight: bold;
        color: #333;
        font-size: 16px;
        word-break: break-all;
        padding: 5px;
    }
    
    .btn-upload {
        padding: 12px 30px;
        background-color: #28a745; /* Green for action */
        color: white;
        border: none;
        border-radius: 8px;
        font-weight: bold;
        font-size: 16px;
        cursor: pointer;
        width: 100%;
        max-width: 200px;
    }
    .btn-upload:hover { background-color: #218838; }
    .btn-upload:disabled { background-color: #94d3a2; cursor: not-allowed; }

    .btn-cancel {
        background: none;
        border: none;
        color: #dc3545;
        cursor: pointer;
        font-size: 14px;
        margin-top: 5px;
        text-decoration: underline;
    }

    /* Table */
    .table-container { flex: 1; overflow-y: auto; border: 1px solid #eee; border-radius: 8px; }
    table { width: 100%; border-collapse: collapse; }
    th { text-align: left; background: #f8f9fa; padding: 15px; border-bottom: 2px solid #ddd; position: sticky; top: 0; color: #495057; }
    td { padding: 15px; border-bottom: 1px solid #eee; color: #212529; }
    
    /* Action Buttons */
    .actions { display: flex; gap: 8px; }
    button.action-btn { 
        padding: 8px 12px; 
        border-radius: 6px; 
        border: none; 
        cursor: pointer; 
        color: white; 
        font-size: 14px;
    }
    .btn-dl { background-color: #6c757d; }
    .btn-ren { background-color: #ffc107; color: #212529 !important; }
    .btn-del { background-color: #dc3545; }

    input[type="file"] { display: none; }
  </style>
</head>
<body>
  <div class="container">
    <div class="header">
        <h2>File Manager</h2>
        <a href="/"><button style="padding: 8px 16px; border: 1px solid #ccc; background: white; color: #333; border-radius: 6px; cursor: pointer;">&larr; Back</button></a>
    </div>

    <!-- Dynamic Upload Section -->
    <div class="upload-section">
        <!-- State 1: Choose File -->
        <label id="chooseWrapper" class="btn-choose">
            + Choose .bin File
            <input type='file' id='uploadInput' name='upload' accept=".bin" onchange="handleFileSelect(this)">
        </label>

        <!-- State 2: Confirm Upload -->
        <div id="uploadWrapper" class="upload-confirm">
            <div id="fileNameDisplay" class="file-name-display"></div>
            <button type="button" onclick="uploadFileJS()" class="btn-upload">Upload Now</button>
            <button type="button" onclick="resetUpload()" class="btn-cancel">Cancel selection</button>
        </div>
    </div>

    <div class="table-container">
      <table id="fileManagerTable">
        <thead>
          <tr>
            <th>Name</th>
            <th>Size</th>
            <th>Actions</th>
          </tr>
        </thead>
        <tbody id="fileListBody"></tbody>
      </table>
    </div>
  </div>

<script>
  let availableFiles = [];

  function handleFileSelect(input) {
    if (input.files && input.files[0]) {
        let file = input.files[0];
        
        // Validate Extension
        if (!file.name.toLowerCase().endsWith(".bin")) {
            alert("Only .bin files are allowed!");
            input.value = ""; 
            return;
        }

        // Validate Length (safe limit 30)
        let name = file.name;
        if(name.length > 30) {
            alert("Filename is too long for device storage (max 30 chars).");
            let newName = prompt("Please enter a shorter name (must end in .bin):", name);
            
            if(newName && newName.length <= 30 && newName.toLowerCase().endsWith(".bin")) {
                // We can't rename the File object directly, but we can handle it in the upload step
                // Store the custom name in a data attribute
                input.dataset.customName = newName;
                name = newName;
            } else {
                alert("Invalid name or cancelled. Please try again.");
                input.value = "";
                return;
            }
        } else {
            input.dataset.customName = ""; // Clear
        }

        document.getElementById('fileNameDisplay').innerText = name;
        
        // Toggle UI
        document.getElementById('chooseWrapper').style.display = 'none';
        document.getElementById('uploadWrapper').style.display = 'flex';
    }
  }

  function resetUpload() {
    document.getElementById('uploadInput').value = "";
    document.getElementById('chooseWrapper').style.display = 'inline-block';
    document.getElementById('uploadWrapper').style.display = 'none';
  }

  function reloadFiles() {
    fetch('/list').then(res => res.json()).then(data => {
        availableFiles = data;
        renderFileManager();
    });
  }

  function renderFileManager() {
    const tbody = document.getElementById('fileListBody');
    tbody.innerHTML = '';
    if(availableFiles.length === 0) {
        tbody.innerHTML = '<tr><td colspan="3" style="text-align:center; padding: 20px; color: #777;">No files found on storage</td></tr>';
        return;
    }
    availableFiles.forEach(f => {
        const tr = document.createElement('tr');
        tr.innerHTML = `
            <td><strong>${f.name}</strong></td>
            <td>${f.size}</td>
            <td class="actions">
                <button class="action-btn btn-dl" onclick="downloadFile('${f.name}')" title="Download">⬇</button>
                <button class="action-btn btn-ren" onclick="renameFile('${f.name}')" title="Rename">✎</button>
                <button class="action-btn btn-del" onclick="deleteFile('${f.name}')" title="Delete">🗑</button>
            </td>
        `;
        tbody.appendChild(tr);
    });
  }

  function uploadFileJS() {
    const input = document.getElementById('uploadInput');
    const file = input.files[0];
    if(!file) return;
    
    const btn = document.querySelector('.btn-upload');
    const originalText = btn.innerText;
    btn.disabled = true;
    btn.innerText = "Uploading..."; 
    
    const formData = new FormData();
    // Check if we have a custom name
    if(input.dataset.customName) {
        formData.append("upload", file, input.dataset.customName);
    } else {
        formData.append("upload", file);
    }
    const xhr = new XMLHttpRequest();
    
    xhr.upload.addEventListener("progress", (e) => {
       if (e.lengthComputable) {
          const percent = Math.round((e.loaded / e.total) * 100);
          btn.innerText = `Uploading... ${percent}%`;
       }
    });

    xhr.onload = function() {
        if (xhr.status === 200 || xhr.status === 302) {
             reloadFiles();
             alert("Upload Successful! \u2705"); // Checkmark
             resetUpload(); // Back to start
        } else {
             alert("Upload Failed \u274C"); // X
        }
        btn.disabled = false;
        btn.innerText = originalText;
    };
    xhr.onerror = function() { 
        alert("Upload Error - Connection Lost"); 
        btn.disabled = false; 
        btn.innerText = originalText; 
    };
    xhr.open("POST", "/upload", true);
    xhr.send(formData);
  }

  function deleteFile(name) {
    if(!confirm("Delete " + name + "?")) return;
    fetch('/delete?name=' + encodeURIComponent(name)).then(res => {
        if(res.ok) { reloadFiles(); } else alert("Delete Failed");
    });
  }

  function renameFile(oldName) {
    const newName = prompt("New name (must end in .bin):", oldName);
    if(newName && newName !== oldName) {
        if(!newName.toLowerCase().endsWith(".bin")) { alert("Must end with .bin"); return; }
        fetch(`/rename?old=${encodeURIComponent(oldName)}&new=${encodeURIComponent(newName)}`).then(res => {
            if(res.ok) { reloadFiles(); } else alert("Rename Failed");
        });
    }
  }

  function downloadFile(name) {
    window.location.href = "/download?name=" + encodeURIComponent(name);
  }

  // Load Initial
  reloadFiles();

</script>
</body>
</html>
)rawliteral";

void WebPortal::begin() {
    // Serve UI
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send_P(200, "text/html", index_html);
    });

    // List Files (For API)
    server.on("/list", HTTP_GET, [](AsyncWebServerRequest *request){
        std::vector<String> files = SDStorage.listFiles("/");
        // Should use ArduinoJson to build this properly
        DynamicJsonDocument doc(4096);
        JsonArray array = doc.to<JsonArray>();
        
        for(const auto &f : files) {
            int sep = f.indexOf('|');
            String name = f.substring(0, sep);
            String size = f.substring(sep+1);
            JsonObject obj = array.createNestedObject();
            obj["name"] = name;
            obj["size"] = size;
        }
        String output;
        serializeJson(doc, output);
        request->send(200, "application/json", output);
    });

    // File Manager UI
    server.on("/files", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send_P(200, "text/html", files_html);
    });

    // Upload Handler
    server.on("/upload", HTTP_POST, [](AsyncWebServerRequest *request){
        String target = "/";
        // if (request->hasParam("upload", true, true)) {
        //     AsyncWebParameter* p = request->getParam("upload", true, true);
        //     String fn = p->value();
        //     target += "?uploaded=" + fn;
        // }
        request->redirect(target);
    }, WebPortal::handleUpload);

    // Flash Handler (JSON POST)
    server.on("/flash", HTTP_POST, [](AsyncWebServerRequest *request){
        // Request body processing happens in onBody or we assume it's small enough?
        // AsyncWebServer handles body in specific callback usually.
        // We will register a body handler below.
        request->send(200, "text/plain", "Flash Started");
    }, NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total){
        // Accumulate body
        static String jsonBody;
        if(index == 0) jsonBody = "";
        for(size_t i=0; i<len; i++) jsonBody += (char)data[i];
        
        if(index + len == total) {
            Serial.println("Flash Request: " + jsonBody);
            Flasher.setStatus("Parsing Flash Request...");
            
            DynamicJsonDocument doc(2048);
            DeserializationError error = deserializeJson(doc, jsonBody);
            if (error) {
                Serial.print("deserializeJson() failed: ");
                Serial.println(error.c_str());
                Flasher.setStatus("JSON Error");
                return;
            }

            String target = doc["target"].as<String>();
            JsonArray files = doc["files"].as<JsonArray>();
            
            std::vector<FlashFile> flashFiles;
            for(JsonObject f : files) {
                FlashFile ff;
                ff.name = f["name"].as<String>();
                // Parse address string (supports 0x prefix or int)
                String addrStr = f["address"].as<String>();
                ff.address = (uint32_t) strtol(addrStr.c_str(), NULL, 0); 
                flashFiles.push_back(ff);
            }
            
            if(!Flasher.flashFirmware(target, flashFiles)) {
                Serial.println("Flasher Busy!");
                Flasher.setStatus("System Busy");
            }
        }
    });

    // Status Handler
    server.on("/status", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(200, "text/plain", Flasher.getStatus());
    });

    // Logs Handler
    server.on("/logs", HTTP_GET, [](AsyncWebServerRequest *request){
        size_t index = 0;
        if(request->hasParam("index")) {
            index = request->getParam("index")->value().toInt();
        }
        
        std::vector<String> newLogs = Flasher.getLogs(index);
        DynamicJsonDocument doc(4096);
        JsonArray arr = doc.createNestedArray("logs");
        for(const auto &l : newLogs) {
            arr.add(l);
        }
        doc["nextIndex"] = Flasher.getLogCount();
        
        String output;
        serializeJson(doc, output);
        request->send(200, "application/json", output);
    });

    // Delete Handler
    server.on("/delete", HTTP_GET, [](AsyncWebServerRequest *request){
        if(request->hasParam("name")){
            String filename = request->getParam("name")->value();
            String path = "/" + filename;
            bool success = false;
            #ifdef USE_SD_CARD
                if(SD.exists(path)) success = SD.remove(path);
            #else
                if(SPIFFS.exists(path)) success = SPIFFS.remove(path);
            #endif
            
            if(success) request->send(200, "text/plain", "Deleted " + filename);
            else request->send(500, "text/plain", "Delete Failed");
        } else {
            request->send(400, "text/plain", "Missing name param");
        }
    });

    // Rename Handler
    server.on("/rename", HTTP_GET, [](AsyncWebServerRequest *request){
        if(request->hasParam("old") && request->hasParam("new")){
            String oldName = "/" + request->getParam("old")->value();
            String newName = "/" + request->getParam("new")->value();
            
            // Check extension
            if(!newName.endsWith(".bin") && !newName.endsWith(".BIN")) {
                 request->send(400, "text/plain", "Rename failed: Only .bin allowed");
                 return;
            }
            
            bool success = false;
            #ifdef USE_SD_CARD
                if(SD.exists(oldName)) success = SD.rename(oldName, newName);
            #else
                if(SPIFFS.exists(oldName)) success = SPIFFS.rename(oldName, newName);
            #endif
            
            if(success) request->send(200, "text/plain", "Renamed to " + newName);
            else request->send(500, "text/plain", "Rename Failed");
        } else {
            request->send(400, "text/plain", "Missing params");
        }
    });

    // Download Handler
    server.on("/download", HTTP_GET, [](AsyncWebServerRequest *request){
        if(request->hasParam("name")){
            String filename = request->getParam("name")->value();
            String path = "/" + filename;
            
            #ifdef USE_SD_CARD
                if(SD.exists(path)) {
                    request->send(SD, path, "application/octet-stream", true);
                }
            #else
                if(SPIFFS.exists(path)) {
                     request->send(SPIFFS, path, "application/octet-stream", true);
                }
            #endif
            else {
                request->send(404, "text/plain", "File not found");
            }
        } else {
            request->send(400, "text/plain", "Missing name param");
        }
    });

    // Check Update Handler
    server.on("/update_check", HTTP_GET, [](AsyncWebServerRequest *request){
        UpdateInfo info;
        if(request->hasParam("cached") && request->getParam("cached")->value() == "true") {
            info = OTAUpdate.getCachedUpdateInfo();
            // If cache is empty/invalid, maybe force check? 
            // For now, assume if cache is empty it just returns "available: false" which is fine.
            // If user wants to force check, they click the button which calls without cached=true.
        } else {
             info = OTAUpdate.checkForUpdate();
        }
        
        DynamicJsonDocument doc(1024);
        doc["available"] = info.available;
        doc["version"] = info.version;
        doc["url"] = info.url;
        doc["notes"] = info.releaseNotes;
        doc["error"] = info.error;
        String output;
        serializeJson(doc, output);
        request->send(200, "application/json", output);
    });

    // Perform Update Handler
    server.on("/update_perform", HTTP_POST, [](AsyncWebServerRequest *request){
        if(request->hasParam("url", true)) {
           updateFirmwareUrl = request->getParam("url", true)->value();
           shouldUpdateFirmware = true;
           request->send(200, "text/plain", "Update Initiated");
        } else {
           request->send(400, "text/plain", "Missing URL");
        }
    });

    server.begin();
    Serial.println("Web Server Started");
    
    // Auto-check for updates on startup (runs in Sync context or should be safe now with WDT fix)
    // We launch this after server start so it doesn't block server init? 
    // Actually blocking here is fine as long as WDT is happy.
    Serial.println("Performing initial OTA check...");
    OTAUpdate.checkForUpdate();
}

// Helper to keep track of upload file across packets
// Note: This simple static implementation assumes single-user, single-upload access.
static File uploadFile;
static String finalFilename; 

void WebPortal::handleUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
    if(!index){
        // Begin Upload: Ensure any previous handle is closed
        if(uploadFile) {
            uploadFile.close();
        }

        Serial.printf("Upload Start: %s\n", filename.c_str());
        
        // Validation: Only allow .bin files
        if(!filename.endsWith(".bin") && !filename.endsWith(".BIN")) {
             Serial.println("Error: Upload rejected. Only .bin files allowed.");
             request->send(400, "text/plain", "Only .bin files allowed");
             // We can't easily stop the upload stream from here, but we can refuse to open the file.
             return; 
        }

        finalFilename = filename;
        
        #ifdef USE_SD_CARD
            String path = "/" + filename;
            if(SD.exists(path)) SD.remove(path);
            uploadFile = SD.open(path, FILE_WRITE);
        #else
            // SPIFFS has a 32-char path limit (including /). Truncate if needed.
            // Max filename length ~30 chars to be safe.
            if(filename.length() > 30) {
                String ext = ".bin";
                if(filename.lastIndexOf('.') != -1) {
                    ext = filename.substring(filename.lastIndexOf('.'));
                }
                
                int extLen = ext.length();
                int baseLen = 30 - extLen; 
                int half = (baseLen - 2) / 2; 
                
                String start = filename.substring(0, half);
                String end = filename.substring(filename.length() - extLen - half, filename.length() - extLen);
                
                finalFilename = start + ".." + end + ext;
                
                Serial.printf("Filename too long (>30 chars). Smart Truncated: %s\n", finalFilename.c_str());
            } else {
                finalFilename = filename;
            }
            
            String path = "/" + finalFilename;
            if(SPIFFS.exists(path)) SPIFFS.remove(path);
            uploadFile = SPIFFS.open(path, FILE_WRITE);
        #endif
        
        if(!uploadFile) {
            Serial.println("Error: Failed to open file for writing at " + String(finalFilename));
            // e.g. SD card missing or full
        } else {
            Flasher.setStatus("Uploading " + finalFilename + " (0%)");
        }
    }
    
    // Write Data
    if(uploadFile){
        if(uploadFile.write(data, len) != len){
            Serial.println("Error: Write failed!");
        }
        
        // Progress Calcs
        size_t total = request->contentLength();
        if(total > 0 && index + len < total) {
            int progress = ((index + len) * 100) / total;
            if (progress % 10 == 0) {
                 Flasher.setStatus("Uploading " + finalFilename + " (" + String(progress) + "%)");
            }
        }
    }
    
    // Finalize
    if(final){
        if(uploadFile){
            uploadFile.close();
            Serial.printf("Upload End: %s, %u bytes\n", finalFilename.c_str(), index+len);
            Flasher.setStatus("Upload Complete: " + finalFilename);
        } else {
             // If file wasn't open (e.g. rejection), maybe log
             Serial.println("Upload finished but file was not open (Rejected?)");
        }
    }
}

void WebPortal::loop() {
    if(shouldUpdateFirmware) {
        shouldUpdateFirmware = false;
        Flasher.setStatus("Starting OTA Update...");
        String res = OTAUpdate.performUpdate(updateFirmwareUrl); 
        if(res == "Success") {
             Flasher.setStatus("OTA Success! Rebooting...");
             delay(2000);
             ESP.restart();
        } else {
             Flasher.setStatus("OTA Failed: " + res);
        }
    }
}
