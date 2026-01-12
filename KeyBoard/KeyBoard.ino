#include <WiFi.h>
#include <WebServer.h>
#include <BleKeyboard.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
const char MAIN_PAGE[] PROGMEM = R"raw(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Keyboard Controller</title>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body { font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', 'Roboto', sans-serif; background-color: #121316; color: #f9f9fa; }
        .container { max-width: 800px; margin: 0 auto; padding: 0 1rem; }
        
        /* Header */
        header { position: sticky; top: 0; z-index: 50; backdrop-filter: blur(12px); border-bottom: 1px solid #2d2e33; background-color: rgba(26, 27, 31, 0.9); }
        .header-content { display: flex; align-items: center; justify-content: space-between; height: 4rem; }
        .header-title h1 { font-size: 1.25rem; font-weight: 600; }
        .status-dot { width: 0.5rem; height: 0.5rem; border-radius: 50%; display: inline-block; margin-right: 0.5rem; }
        .status-dot.idle { background-color: #10b981; }
        .status-dot.typing { background-color: #3b82f6; animation: pulse 2s infinite; }
        @keyframes pulse { 0%, 100% { opacity: 1; } 50% { opacity: 0.5; } }

        /* Cards */
        .card { background-color: #1a1b1f; border-radius: 0.75rem; border: 1px solid #2d2e33; overflow: hidden; margin-bottom: 1.5rem; }
        .card-header { padding: 1rem 1.5rem; border-bottom: 1px solid #2d2e33; font-weight: 600; display: flex; justify-content: space-between; align-items: center; }
        .card-body { padding: 1.5rem; }

        /* Inputs */
        textarea, input[type="number"] { width: 100%; padding: 0.75rem; border-radius: 0.5rem; border: 2px solid #2d2e33; background-color: #121316; color: #f9f9fa; font-family: inherit; transition: border-color 0.2s; }
        textarea:focus, input:focus { outline: none; border-color: #3b82f6; }
        textarea { resize: vertical; min-height: 12rem; }

        /* Flex Layout */
        .flex-container { display: flex; flex-wrap: wrap; gap: 1rem; }
        .flex-full { flex: 1 1 100%; }
        .flex-half { flex: 1 1 300px; } /* Grow: 1, Shrink: 1, Basis: 300px */
        
        .flex-between { display: flex; justify-content: space-between; align-items: center; }
        .mb-2 { margin-bottom: 0.5rem; }

        /* Buttons */
        .btn { padding: 0.75rem 1.5rem; border-radius: 0.5rem; border: none; font-weight: 600; cursor: pointer; transition: opacity 0.2s; width: 100%; }
        .btn:disabled { opacity: 0.5; cursor: not-allowed; }
        .btn-primary { background-color: #3b82f6; color: white; }
        .btn-warning { background-color: #f59e0b; color: white; }
        .btn-danger { background-color: #ef4444; color: white; }

        /* Stats */
        .stat-group { display: flex; gap: 2rem; color: #a0a2ab; font-size: 0.875rem; }
        .stat-val { color: #f9f9fa; font-weight: 600; margin-left: 0.25rem; }

        /* Timer */
        /* Timer */
        .timer-display { font-family: monospace; font-size: 1.5rem; font-weight: 700; color: #3b82f6; text-align: center; margin-top: 0.5rem; }

        /* Progress Bar */
        .progress-wrapper { width: 200px; height: 1.25rem; background: #2d2e33; border-radius: 0.25rem; overflow: hidden; position: relative; display: none; }
        .progress-bar { height: 100%; background: linear-gradient(45deg, #10b981 25%, #059669 25%, #059669 50%, #10b981 50%, #10b981 75%, #059669 75%, #059669); background-size: 1rem 1rem; animation: stripemove 1s linear infinite; transition: width 0.3s ease; width: 0%; }
        .progress-text { position: absolute; top: 0; left: 0; width: 100%; height: 100%; display: flex; align-items: center; justify-content: center; font-size: 0.75rem; font-weight: 700; color: #fff; text-shadow: 0 1px 2px rgba(0,0,0,0.5); }
        @keyframes stripemove { from { background-position: 0 0; } to { background-position: 1rem 1rem; } }
    </style>
</head>
<body>
    <header>
        <div class="container">
            <div class="header-content">
                <div class="header-title">
                    <h1>Keyboard</h1>
                </div>
                <div style="display: flex; align-items: center;">
                    <div class="status-dot idle" id="statusDot"></div>
                    <span id="statusText" style="font-size: 0.875rem; font-weight: 600;">Idle</span>
                </div>
            </div>
        </div>
    </header>

    <main class="container" style="padding-top: 2rem;">
        
        <!-- System Stats Bar -->
        <div class="card">
            <div class="card-body" style="padding: 1rem;">
                <div class="flex-between">
                    <div class="stat-group">
                        <div>BT: <span class="stat-val" id="btVal" style="color: #ef4444;">Disconnected</span></div>
                        <div>WiFi: <span class="stat-val" id="wifiVal" style="color: #10b981;">Connected (-54dBm)</span></div>
                    </div>
                </div>
                <div style="height: 1px; background: #2d2e33; margin: 0.75rem 0;"></div>
                <div class="flex-between">
                    <div class="stat-group">
                        <div>CPU: <span class="stat-val" id="cpuVal">0%</span></div>
                        <div>RAM: <span class="stat-val" id="heapVal">0KB</span></div>
                        <div>Uptime: <span class="stat-val" id="uptimeVal">00:00:00</span></div>
                    </div>
                    <div style="font-size: 0.75rem; color: #52545c;">ESP32 Active</div>
                </div>
            </div>
        </div>

        <div class="flex-container">
            <!-- Text Input -->
            <div class="card flex-full">
                <div class="card-header">
                    <span>Text Input</span>
                    <div class="progress-wrapper" id="progWrapper">
                        <div class="progress-bar" id="progBar"></div>
                        <div class="progress-text" id="progText">0% Left</div>
                    </div>
                </div>
                <div class="card-body">
                    <textarea id="textInput" placeholder="Enter text here..." oninput="updateCalculations()"></textarea>
                    <div class="flex-between" style="margin-top: 1rem;">
                        <span style="font-size: 0.875rem; color: #a0a2ab;">Characters: <span id="charCount" style="color: #f9f9fa;">0</span></span>
                        <button class="btn btn-primary" id="startBtn" style="width: auto;" onclick="startTyping()">Start Typing</button>
                    </div>
                </div>
            </div>

            <!-- Settings -->
            <div class="card flex-half">
                <div class="card-header">Typing Speed</div>
                <div class="card-body">
                    <div class="mb-2" style="font-size: 0.875rem; color: #a0a2ab;">Delay per character (ms)</div>
                    <input type="number" id="speedInput" value="150" min="1" oninput="updateCalculations()">
                    
                    <div style="margin-top: 1.5rem; text-align: center;">
                        <div style="font-size: 0.75rem; color: #a0a2ab;">ESTIMATED DURATION</div>
                        <div class="timer-display" id="durationDisplay">00:00:00</div>
                    </div>
                </div>
            </div>

            <!-- Controls -->
            <div class="card flex-half">
                <div class="card-header">Controls</div>
                <div class="card-body" style="display: flex; flex-direction: column; gap: 1rem;">
                    <button class="btn btn-warning" id="pauseBtn" onclick="togglePause()">Pause / Resume</button>
                    <button class="btn btn-danger" onclick="if(confirm('Reboot device?')) rebootDevice()">Reboot Device</button>
                </div>
            </div>
        </div>
    </main>

    <script>
        let appState = {
            status: 'idle',
            speed: 150
        };

        document.addEventListener('DOMContentLoaded', function() {
            setInterval(fetchStatus, 2000);
            fetchStatus(); // Initial fetch
        });

        function formatTime(seconds) {
            const h = Math.floor(seconds / 3600).toString().padStart(2, '0');
            const m = Math.floor((seconds % 3600) / 60).toString().padStart(2, '0');
            const s = Math.floor(seconds % 60).toString().padStart(2, '0');
            return `${h}:${m}:${s}`;
        }

        async function fetchStatus() {
            try {
                const response = await fetch('/status');
                const data = await response.json();
                
                // Update Badge
                const statusDot = document.getElementById('statusDot');
                const statusText = document.getElementById('statusText');
                const btn = document.getElementById('startBtn');
                
                statusDot.className = 'status-dot ' + (data.status === 'typing' ? 'typing' : 'idle');
                statusText.textContent = data.status.toUpperCase();
                
                // State Restoration Logic
                const textInput = document.getElementById('textInput');
                const hasActiveJob = (data.total > 0 && data.current < data.total);
                
                if (hasActiveJob && textInput.value.length === 0) {
                     try {
                         const txtRes = await fetch('/get_text');
                         if(txtRes.ok) {
                             textInput.value = await txtRes.text();
                             document.getElementById('charCount').textContent = textInput.value.length;
                             if(data.speed) {
                                 document.getElementById('speedInput').value = data.speed;
                                 appState.speed = data.speed;
                             }
                         }
                     } catch(e) { console.error("Failed to restore text", e); }
                }

                // Update Button State
                if (data.status === 'typing') {
                    btn.disabled = true;
                    btn.textContent = 'Typing...';
                } else if (data.status === 'paused') {
                    btn.disabled = false; // Allow restart if needed, otherwise handled by Pause/Resume
                    btn.textContent = 'Start Typing'; 
                } else {
                    btn.disabled = false;
                    btn.textContent = 'Start Typing';
                }

                // Update Stats
                document.getElementById('heapVal').textContent = Math.round(data.heap / 1024) + 'KB';
                document.getElementById('wifiVal').textContent = 'Connected (' + data.rssi + 'dBm)';
                
                // BT Status
                const btEl = document.getElementById('btVal');
                btEl.textContent = data.bt ? 'Connected' : 'Disconnected';
                btEl.style.color = data.bt ? '#10b981' : '#ef4444';

                // Uptime
                document.getElementById('uptimeVal').textContent = formatTime(Math.floor(data.uptime / 1000));
                
                // Timer Logic
                const durationLabel = document.getElementById('durationDisplay').previousElementSibling;
                const progWrapper = document.getElementById('progWrapper');
                const progBar = document.getElementById('progBar');
                const progText = document.getElementById('progText');

                if (hasActiveJob) {
                    durationLabel.textContent = "REMAINING TIME";
                    const remainingChars = data.total - data.current;
                    const speed = data.speed || parseInt(document.getElementById('speedInput').value) || 150;
                    const ms = remainingChars * speed;
                    document.getElementById('durationDisplay').textContent = formatTime(Math.floor(ms / 1000));
                    
                    // Progress Bar Logic
                    progWrapper.style.display = 'block';
                    const pctDone = (data.current / data.total) * 100;
                    const pctLeft = 100 - pctDone;
                    progBar.style.width = pctDone + '%';
                    progText.textContent = Math.round(pctLeft) + '% Left';

                } else if (textInput.value.length > 0) {
                     // Fallback to calculation if job finished or idle but text exists
                     durationLabel.textContent = "ESTIMATED DURATION";
                     updateCalculations();
                     progWrapper.style.display = 'none';
                } else {
                     durationLabel.textContent = "ESTIMATED DURATION";
                     document.getElementById('durationDisplay').textContent = "00:00:00";
                     progWrapper.style.display = 'none';
                }
                
            } catch (error) {
                console.error('Connection Lost', error);
                document.getElementById('wifiVal').textContent = 'Offline'; 
            }
        }

        async function apiCall(endpoint, data = null) {
            try {
                let options = { method: data ? 'POST' : 'GET' };
                if (data) {
                    const formData = new FormData();
                    for (const key in data) formData.append(key, data[key]);
                    options.body = formData;
                }
                await fetch(endpoint, options);
                setTimeout(fetchStatus, 500); // Trigger immediate update
            } catch (e) {
                console.error(e);
            }
        }

        function startTyping() {
            const text = document.getElementById('textInput').value;
            const speed = parseInt(document.getElementById('speedInput').value) || 150;
            if (!text.trim()) return;
            
            apiCall('/type', { msg: text, speed: speed });
            
            // Immediate UI feedback
            document.getElementById('startBtn').disabled = true;
            document.getElementById('startBtn').textContent = 'Typing...';
            document.getElementById('statusDot').className = 'status-dot typing';
            document.getElementById('statusText').textContent = 'TYPING';
        }

        function togglePause() {
             const btn = document.getElementById('pauseBtn');
             // Simple toggle logic, actual state handled by poll
             if (document.getElementById('statusText').textContent === 'PAUSED') {
                 apiCall('/resume');
             } else {
                 apiCall('/pause');
             }
        }
        
        function rebootDevice() { apiCall('/reboot'); }

        function updateCalculations() {
            // Only update if we are not actively typing based on last known state (approx)
            // But fetchStatus will override anyway. This is for immediate feedback while editing.
            const text = document.getElementById('textInput').value;
            const speed = parseInt(document.getElementById('speedInput').value) || 150;
            
            document.getElementById('charCount').textContent = text.length;

            const totalMs = text.length * speed;
            document.getElementById('durationDisplay').textContent = formatTime(Math.floor(totalMs / 1000));
        }
    </script>
</body>
</html>
)raw";

// OLED Configuration
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Bluetooth Keyboard Setup
BleKeyboard bleKeyboard("Keyboard", "Espressif", 100);
WebServer server(80);

// WiFi Credentials
const char* ssid = "CMF Nothing One";
const char* password = "12345678901";

// Typing State Variables
String textToType = "";
int currentIndex = 0;
bool isPaused = true;
unsigned long lastTypeTime = 0;
int currentDelay = 150;
bool lastBTState = false;
unsigned long lastWifiCheck = 0;

// --- Helper Functions ---

bool isSpecialChar(char c) {
  return !isalnum(c) && c != ' ' && c != '\n' && c != '\r';
}

int countWords(String txt, int endIdx) {
  if (endIdx <= 0) return 0;
  int words = 1;
  for (int i = 0; i < endIdx; i++) {
    if (txt[i] == ' ' || txt[i] == '\n' || txt[i] == '\r') {
      if (i > 0 && txt[i - 1] != ' ' && txt[i - 1] != '\n') words++;
    }
  }
  return words;
}

void updateOLED() {
  display.clearDisplay();
  static byte frame = 0;
  char spinner[] = { '|', '/', '-', '\\' };
  bool blink = (millis() / 500) % 2;

  display.fillRect(0, 0, 128, 11, SSD1306_WHITE);
  display.setTextColor(SSD1306_BLACK);
  display.setCursor(1, 2);
  display.print("http://");
  display.print(WiFi.localIP());

  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 16);
  display.print("BT: ");
  if (bleKeyboard.isConnected()) {
    display.print("[CONNECTED] ");
    display.print(spinner[frame % 4]);
  } else {
    display.print("[OFFLINE]");
  }

  display.drawFastHLine(0, 28, 128, SSD1306_WHITE);

  int totalChars = textToType.length();
  display.setCursor(0, 34);

  if (totalChars > 0) {
    if (isPaused) {
      display.print("STATUS: PAUSED");
    } else if (currentIndex >= totalChars) {
      display.print("STATUS: DONE!");
    } else {
      display.print("STATUS: TYPING");
      if (blink) display.print("_");
      frame++;
    }

    display.setCursor(0, 46);
    display.print("Progress: ");
    display.print((currentIndex * 100) / totalChars);
    display.print("%");

    int barWidth = map(currentIndex, 0, totalChars, 0, 124);
    display.drawRect(0, 56, 128, 7, SSD1306_WHITE);
    display.fillRect(2, 58, barWidth, 3, SSD1306_WHITE);
  } else {
    display.setCursor(20, 40);
    display.println("<< STANDBY >>");
  }
  display.display();
}

// --- Web Server Handlers ---

void handleRoot() {
  server.send_P(200, "text/html", MAIN_PAGE);
}

void handleStatus() {
  String status = "idle";
  if (textToType.length() > 0 && currentIndex < textToType.length()) {
    status = isPaused ? "paused" : "typing";
  }
  
  int progress = (textToType.length() > 0) ? (currentIndex * 100) / textToType.length() : 0;
  
  String json = "{";
  json += "\"status\":\"" + status + "\",";
  json += "\"progress\":" + String(progress) + ",";
  json += "\"rssi\":" + String(WiFi.RSSI()) + ",";
  json += "\"heap\":" + String(ESP.getFreeHeap()) + ",";
  json += "\"uptime\":" + String(millis()) + ",";
  json += "\"bt\":" + String(bleKeyboard.isConnected() ? "true" : "false") + ",";
  json += "\"total\":" + String(textToType.length()) + ",";
  json += "\"current\":" + String(currentIndex) + ",";
  json += "\"speed\":" + String(currentDelay);
  json += "}";
  
  server.send(200, "application/json", json);
}

void handleGetText() {
  server.send(200, "text/plain", textToType);
}

void handleType() {
  if (server.hasArg("msg")) {
    textToType = server.arg("msg");
    currentIndex = 0;
    isPaused = false;
  }
  if (server.hasArg("speed")) currentDelay = server.arg("speed").toInt();
  server.sendHeader("Location", "/");
  server.send(303);
}

void handleEnd() {
  isPaused = true;
  currentIndex = 0;
  server.sendHeader("Location", "/");
  server.send(303);
}
void handlePause() {
  isPaused = true;
  server.sendHeader("Location", "/");
  server.send(303);
}
void handleResume() {
  isPaused = false;
  server.sendHeader("Location", "/");
  server.send(303);
}

// New Hard Reboot Handler
void handleReboot() {
  server.send(200, "text/html", "<html><body style='text-align:center;font-family:sans-serif;'><h1>Rebooting...</h1><p>Wait 10 seconds and refresh.</p></body></html>");
  delay(1000);
  ESP.restart();
}

// --- NEW FEATURE HANDLERS (Cyberpunk Update) ---

void handleMacro() {
  if (server.hasArg("cmd")) {
    String cmd = server.arg("cmd");
    if (cmd == "lock") {
      bleKeyboard.press(KEY_LEFT_GUI);
      bleKeyboard.press('l');
      delay(100);
      bleKeyboard.releaseAll();
    } else if (cmd == "desktop") {
      bleKeyboard.press(KEY_LEFT_GUI);
      bleKeyboard.press('d');
      delay(100);
      bleKeyboard.releaseAll();
    } else if (cmd == "tab") {
      bleKeyboard.press(KEY_LEFT_ALT);
      bleKeyboard.press(KEY_TAB);
      delay(100);
      bleKeyboard.releaseAll();
    } else if (cmd == "copy") {
      bleKeyboard.press(KEY_LEFT_CTRL);
      bleKeyboard.press('c');
      delay(100);
      bleKeyboard.releaseAll();
    } else if (cmd == "paste") {
      bleKeyboard.press(KEY_LEFT_CTRL);
      bleKeyboard.press('v');
      delay(100);
      bleKeyboard.releaseAll();
    }
  }
  server.send(204); // No Content (keeps user on same page)
}

void handleMedia() {
  if (server.hasArg("cmd")) {
    String cmd = server.arg("cmd");
    if (cmd == "play") bleKeyboard.write(KEY_MEDIA_PLAY_PAUSE);
    else if (cmd == "volup") bleKeyboard.write(KEY_MEDIA_VOLUME_UP);
    else if (cmd == "voldown") bleKeyboard.write(KEY_MEDIA_VOLUME_DOWN);
    else if (cmd == "mute") bleKeyboard.write(KEY_MEDIA_MUTE);
    else if (cmd == "next") bleKeyboard.write(KEY_MEDIA_NEXT_TRACK);
    else if (cmd == "prev") bleKeyboard.write(KEY_MEDIA_PREVIOUS_TRACK);
  }
  server.send(204);
}

// Quick Slots (M1-M5)
void handleQuick() {
  if (server.hasArg("msg")) {
    // We inject this directly into the typing queue logic or just print it immediately?
    // User asked NOT to change typing logic.
    // Safest way involves appending to textToType OR just blocking print if queue is empty.
    // Let's do direct print for "Instant" feel, assuming queue is empty.
    // If typing is active, we might want to wait. 
    // For now, let's just append to the current text buffer to adhere to "don't change typing code" 
    // and let the existing loop handle it!
    String msg = server.arg("msg");
    if(msg.length() > 0) {
        textToType += msg; 
        isPaused = false; // Auto-start typing
    }
  }
  server.send(204);
}

void setup() {
  Serial.begin(115200);

  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println("SSD1306 failed");
    for (;;)
      ;
  }

  WiFi.begin(ssid, password);

  int progress = 0;
  while (WiFi.status() != WL_CONNECTED) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 10);
    display.println("Looking for:");
    display.setCursor(0, 22);
    display.print("> ");
    display.println(ssid);
    display.drawRect(10, 45, 108, 10, SSD1306_WHITE);
    display.fillRect(12, 47, progress, 6, SSD1306_WHITE);
    display.display();
    progress += 4;
    if (progress > 104) progress = 0;
    delay(200);
  }

  bleKeyboard.begin();

  server.on("/", HTTP_GET, handleRoot);
  server.on("/status", HTTP_GET, handleStatus);
  server.on("/type", HTTP_POST, handleType);
  server.on("/pause", HTTP_GET, handlePause);
  server.on("/resume", HTTP_GET, handleResume);
  server.on("/end", HTTP_GET, handleEnd);
  server.on("/reboot", HTTP_GET, handleReboot);
  
  // New Routes
  server.on("/macro", HTTP_GET, handleMacro);
  server.on("/media", HTTP_GET, handleMedia);
  server.on("/quick", HTTP_GET, handleQuick);
  server.on("/get_text", HTTP_GET, handleGetText);
  server.begin();

  updateOLED();
}

void loop() {
  if (millis() - lastWifiCheck > 30000) {
    if (WiFi.status() != WL_CONNECTED) {
      WiFi.reconnect();
    }
    lastWifiCheck = millis();
  }
  server.handleClient();

  bool currentBTState = bleKeyboard.isConnected();
  if (currentBTState != lastBTState) {
    lastBTState = currentBTState;
    updateOLED();
  }

  if (!isPaused && currentBTState && currentIndex < textToType.length()) {
    char c = textToType[currentIndex];
    unsigned long effectiveDelay = isSpecialChar(c) ? 250 : currentDelay;

    if (millis() - lastTypeTime >= effectiveDelay) {
      if (c == '\n' || c == '\r') {
        bleKeyboard.press(KEY_LEFT_SHIFT);
        bleKeyboard.press(KEY_RETURN);
        delay(50);
        bleKeyboard.releaseAll();

        if (currentIndex + 1 < textToType.length() && ((textToType[currentIndex] == '\n' && textToType[currentIndex + 1] == '\r') || (textToType[currentIndex] == '\r' && textToType[currentIndex + 1] == '\n'))) {
          currentIndex++;
        }
      } else {
        // --- FIX PORTION START ---
        if (isSpecialChar(c)) {
          bleKeyboard.write(c);
        } else {
          bleKeyboard.print(c);
        }
        // --- FIX PORTION END ---
      }
      currentIndex++;
      lastTypeTime = millis();
      if (currentIndex % 5 == 0 || currentIndex == textToType.length()) {
        updateOLED();
      }
    }
  }
}