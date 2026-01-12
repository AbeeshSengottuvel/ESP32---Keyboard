#include <WiFi.h>
#include <WebServer.h>
#include <BleKeyboard.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

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
  int wordsTyped = countWords(textToType, currentIndex);
  int totalWords = countWords(textToType, textToType.length());
  int progressPercent = (textToType.length() > 0) ? (currentIndex * 100) / textToType.length() : 0;
  bool isTypingActive = (textToType.length() > 0 && currentIndex < textToType.length());

  // --- Timer Calculation ---
  int charsLeft = textToType.length() - currentIndex;
  unsigned long totalMsLeft = (unsigned long)charsLeft * currentDelay;

  String html = R"raw(
<!DOCTYPE html>
<html>
<head>
  <meta name='viewport' content='width=device-width, initial-scale=1.0'>
  <title>Neural Link v2.0</title>
  <style>
    :root { --bg: #121212; --panel: #1e1e1e; --primary: #00f3ff; --accent: #bd00ff; --text: #e0e0e0; }
    body { font-family: 'Segoe UI', sans-serif; background: var(--bg); color: var(--text); padding: 0; margin: 0; display: flex; flex-direction: column; align-items: center; min-height: 100vh; }
    h2 { color: var(--primary); text-transform: uppercase; letter-spacing: 2px; text-shadow: 0 0 10px rgba(0, 243, 255, 0.5); margin: 20px 0; }
    .container { width: 90%; max-width: 450px; padding-bottom: 50px; }
    
    /* INPUT ZONE */
    textarea { width: 100%; height: 100px; background: #0a0a0a; border: 1px solid #333; color: var(--primary); padding: 10px; border-radius: 8px; font-family: 'Consolas', monospace; resize: none; margin-bottom: 10px; box-shadow: inset 0 0 10px rgba(0,0,0,0.5); }
    textarea:focus { outline: none; border-color: var(--primary); box-shadow: 0 0 15px rgba(0, 243, 255, 0.2); }
    
    /* STATS */
    .status-bar { display: flex; justify-content: space-between; font-size: 12px; margin-bottom: 15px; color: #888; }
    .status-item span { color: var(--primary); font-weight: bold; }
    
    /* CONTROLS */
    .grid { display: grid; grid-template-columns: repeat(3, 1fr); gap: 10px; margin-bottom: 20px; }
    .btn { background: var(--panel); border: 1px solid #333; color: #fff; padding: 15px 5px; border-radius: 10px; font-size: 12px; cursor: pointer; transition: 0.2s; display: flex; flex-direction: column; align-items: center; justify-content: center; box-shadow: 0 4px 6px rgba(0,0,0,0.3); }
    .btn i { font-size: 20px; margin-bottom: 5px; } 
    .btn:active { transform: scale(0.95); box-shadow: 0 0 10px var(--primary); border-color: var(--primary); }
    .btn.media { border-color: rgba(189, 0, 255, 0.3); }
    .btn.macro { border-color: rgba(0, 243, 255, 0.3); }
    .btn.start { background: linear-gradient(135deg, #005c97, #363795); grid-column: span 3; font-size: 16px; font-weight: bold; padding: 15px; letter-spacing: 1px; }
    
    /* PROGRESS */
    .progress-box { background: #333; height: 6px; border-radius: 3px; margin: 10px 0; overflow: hidden; }
    .bar { background: var(--primary); height: 100%; width: 0%; transition: width 0.5s; box-shadow: 0 0 10px var(--primary); }
  </style>
  <script>
    function hit(url) { fetch(url); }
  </script>
</head>
<body>
  <h2>Neural Link_v2.0</h2>
  <div class='container'>
)raw";

  // Dynamic CSS injection for progress bar
  html += "<style>.bar { width: " + String(progressPercent) + "%; }</style>";
  
  if (!isPaused && isTypingActive) html += "<meta http-equiv='refresh' content='2'>";

  html += R"raw(
    <div class='status-bar'>
      <div class='status-item'>STATUS: <span>)raw";
  
  html += (isPaused ? "STANDBY" : "TRANSMITTING...");
  
  html += R"raw(</span></div>
      <div class='status-item'>WORDS: <span>)raw" + String(wordsTyped) + R"raw(</span></div>
    </div>
    
    <div class='progress-box'><div class='bar'></div></div>

    <form action='/type' method='POST'>
      <textarea name='msg' placeholder='// Enter Command Stream...'>)raw" + textToType + R"raw(</textarea>
      
      <div style='display:flex; justify-content:space-between; margin-bottom:15px; align-items:center;'>
        <span style='font-size:12px; color:#666;'>DELAY (ms):</span>
        <input type='number' name='speed' value=')raw" + String(currentDelay) + R"raw(' style='background:#222; border:none; color:white; padding:5px; width:50px; text-align:center;'>
      </div>

      <button type='submit' class='btn start'>INITIATE UPLOAD</button>
    </form>

    <div style='text-align:left; color:#666; font-size:10px; margin-top:20px; margin-bottom:5px;'>MEDIA DECK</div>
    <div class='grid'>
      <button class='btn media' onclick="hit('/media?cmd=prev')">|&lt;</button>
      <button class='btn media' onclick="hit('/media?cmd=play')">PLAY</button>
      <button class='btn media' onclick="hit('/media?cmd=next')">&gt;|</button>
      <button class='btn media' onclick="hit('/media?cmd=voldown')">VOL -</button>
      <button class='btn media' onclick="hit('/media?cmd=mute')">MUTE</button>
      <button class='btn media' onclick="hit('/media?cmd=volup')">VOL +</button>
    </div>

    <div style='text-align:left; color:#666; font-size:10px; margin-bottom:5px;'>MACRO MATRIX</div>
    <div class='grid'>
      <button class='btn macro' onclick="hit('/macro?cmd=copy')">COPY</button>
      <button class='btn macro' onclick="hit('/macro?cmd=paste')">PASTE</button>
      <button class='btn macro' onclick="hit('/macro?cmd=tab')">TAB</button>
      <button class='btn macro' onclick="hit('/macro?cmd=lock')">LOCK</button>
      <button class='btn macro' onclick="hit('/macro?cmd=desktop')">DESKTOP</button>
      <button class='btn macro' onclick="location.href='/end'" style='border-color: #ff0055; color:#ff0055;'>ABORT</button>
    </div>
    
    <div style='text-align:left; color:#666; font-size:10px; margin-bottom:5px;'>QUICK SLOTS</div>
    <div class='grid'>
       <button class='btn' onclick="hit('/quick?msg=Hello World')">M1</button>
       <button class='btn' onclick="hit('/quick?msg=git status')">M2</button>
       <button class='btn' onclick="hit('/quick?msg=npm run dev')">M3</button>
       <button class='btn' onclick="if(confirm('Reboot?')) location.href='/reboot'">RST</button>
       <button class='btn' onclick="location.href='/pause'" style='color:#f39c12'>PAUSE</button>
    </div>

    <div style='margin-top:20px; font-size:10px; color:#444;'>
      SYSTEM_ID: ESP32_C3 | BATTERY: N/A
    </div>
  </div>
</body>
</html>
)raw";

  server.send(200, "text/html", html);
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
  server.on("/type", HTTP_POST, handleType);
  server.on("/pause", HTTP_GET, handlePause);
  server.on("/resume", HTTP_GET, handleResume);
  server.on("/end", HTTP_GET, handleEnd);
  server.on("/reboot", HTTP_GET, handleReboot);
  
  // New Routes
  server.on("/macro", HTTP_GET, handleMacro);
  server.on("/media", HTTP_GET, handleMedia);
  server.on("/quick", HTTP_GET, handleQuick);
  server.begin();

  updateOLED();
}

void loop() {
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