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

  int mins = totalMsLeft / 60000;
  int secs = (totalMsLeft % 60000) / 1000;
  int ms = totalMsLeft % 1000;
  // -------------------------

  String html = "<!DOCTYPE html><html><head>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<style>";
  html += "body{font-family:-apple-system,sans-serif; background:#121212; color:#e0e0e0; text-align:center; padding:15px; margin:0;} ";
  html += "h2{color:#007bff; margin-bottom:10px;} ";
  html += ".container{max-width:400px; margin:auto; background:#1e1e1e; padding:20px; border-radius:15px; box-shadow:0 10px 30px rgba(0,0,0,0.5);} ";
  html += "textarea{width:100%; height:120px; padding:12px; box-sizing:border-box; border-radius:10px; border:1px solid #333; background:#2c2c2c; color:white; font-size:16px; margin-bottom:15px; resize:none;} ";
  html += ".stats{display:flex; justify-content:space-around; background:#2c2c2c; padding:10px; border-radius:10px; margin-bottom:15px; font-size:14px;} ";
  html += ".progress-container{background:#333; border-radius:10px; height:10px; margin-bottom:20px; overflow:hidden;} ";
  html += ".progress-bar{background:#007bff; width:" + String(progressPercent) + "%; height:100%; transition:width 0.3s;} ";
  html += ".grid{display:grid; grid-template-columns: 1fr 1fr; gap:10px;} ";
  html += ".btn{padding:12px; border:none; border-radius:8px; font-size:14px; font-weight:bold; cursor:pointer; color:white; transition:0.2s;} ";
  html += ".btn:active{transform:scale(0.96);} ";
  html += ".start{background:#007bff; grid-column: span 2;} .pause{background:#f39c12; color:white;} ";
  html += ".resume{background:#27ae60;} .reset{background:#e74c3c; grid-column: span 2;} .reboot{background:#444; grid-column: span 2; margin-top:10px; opacity:0.7;} ";
  html += "input[type=number]{background:#2c2c2c; border:1px solid #444; color:white; padding:5px; border-radius:5px; width:50px;} ";
  html += "</style>";

  if (!isPaused && isTypingActive) html += "<meta http-equiv='refresh' content='2'>";

  html += "</head><body>";
  html += "<h2>Pro Keyboard</h2>";
  html += "<div class='container'>";

  // Updated Status Bar with Timer
  html += "<div class='stats'>";
  html += "<div><b>Words</b><br>" + String(wordsTyped) + "/" + String(totalWords) + "</div>";
  html += "<div><b>Remaining</b><br>" + String(mins) + ":" + (secs < 10 ? "0" : "") + String(secs) + ":" + String(ms) + "</div>";
  html += "<div><b>Chars</b><br>" + String(currentIndex) + "/" + String(textToType.length()) + "</div>";
  html += "</div>";

  // Web Progress Bar
  html += "<div class='progress-container'><div class='progress-bar'></div></div>";

  // Input Form
  html += "<form action='/type' method='POST'>";
  html += "<textarea name='msg' placeholder='Paste text here...'>" + textToType + "</textarea>";
  html += "<p style='font-size:13px;'>Delay: <input type='number' name='speed' value='" + String(currentDelay) + "'> ms</p>";
  html += "<button type='submit' class='btn start' " + String(isTypingActive ? "disabled" : "") + ">BEGIN TYPING</button>";
  html += "</form>";

  // Control Grid
  html += "<div class='grid' style='margin-top:15px;'>";
  html += "<button onclick=\"location.href='/pause'\" class='btn pause'>PAUSE</button>";
  html += "<button onclick=\"location.href='/resume'\" class='btn resume'>RESUME</button>";
  html += "<button onclick=\"location.href='/end'\" class='btn reset'>CLEAR & RESET</button>";
  html += "<button onclick=\"if(confirm('Reboot ESP32?')) location.href='/reboot'\" class='btn reboot'>HARD REBOOT</button>";
  html += "</div>";

  html += "</div></body></html>";

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
  server.on("/reboot", HTTP_GET, handleReboot);  // Register reboot route
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