#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"
#include <WebServer.h>
#include <Preferences.h>

String storedSSID;
String storedPassword;

String WIFI_SSID_AP = "Metal_Detection_Bot_Config";

// For configuration portal
bool configMode = false;
WebServer server(80);
Preferences preferences;

void handleRoot() {
  // This is the configuration page if no credentials are set or you want to reconfigure.
  String html = "<!DOCTYPE html><html><head><meta charset='utf-8'><meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<title>" + WIFI_SSID_AP + "</title><style>";
  html += "body { font-family: Arial, sans-serif; margin: 0; padding: 20px; background:#f5f5f5; color:#333; }";
  html += ".container { max-width:500px; margin:0 auto; background:white; padding:30px; border-radius:10px; box-shadow:0 2px 10px rgba(0,0,0,0.1); }";
  html += "label { display:block; margin-bottom:5px; font-weight:bold; }";
  html += "input[type='text'], input[type='password'] { width:100%; padding:10px; margin-bottom:20px; border:1px solid #ddd; border-radius:4px; }";
  html += "input[type='submit'] { background:#0066cc; color:white; border:none; padding:12px 20px; border-radius:4px; cursor:pointer; width:100%; font-size:16px; }";
  html += "input[type='submit']:hover { background:#0055aa; }";
  html += ".reset-link { display:block; text-align:center; margin-top:20px; color:#cc0000; text-decoration:none; }";
  html += ".status { text-align:center; margin-top:20px; padding:10px; border-radius:4px; }";
  html += ".connected { background:#d4edda; color:#155724; }";
  html += ".disconnected { background:#f8d7da; color:#721c24; }";
  html += "</style></head><body><div class='container'>";
  html += "<h1>" + WIFI_SSID_AP + "</h1>";
  if(WiFi.status() == WL_CONNECTED)
    html += "<div class='status connected'>Connected to WiFi: " + WiFi.SSID() + "<br>IP: " + WiFi.localIP().toString() + "</div>";
  else
    html += "<div class='status disconnected'>Not connected to WiFi</div>";
  html += "<form action='/save' method='POST'>";
  html += "<label for='ssid'>WiFi SSID:</label>";
  html += "<input type='text' id='ssid' name='ssid' value='" + storedSSID + "' required>";
  html += "<label for='password'>WiFi Password:</label>";
  html += "<input type='password' id='password' name='password' value='" + storedPassword + "'>";
  html += "<input type='submit' value='Save Configuration'>";
  html += "</form>";
  html += "<a href='/reset' class='reset-link'>Reset All Configuration</a><br><br>";
  html += "</div></body></html>";
  server.send(200, "text/html", html);
}

void handleSave() {
  if(server.hasArg("ssid") && server.hasArg("password")) {
    storedSSID = server.arg("ssid");
    storedPassword = server.arg("password");

    preferences.begin("config", false);
    preferences.putString("ssid", storedSSID);
    preferences.putString("password", storedPassword);
    preferences.end();

    String html = "<!DOCTYPE html><html><head><meta charset='utf-8'><meta name='viewport' content='width=device-width, initial-scale=1'>";
    html += "<title>Configuration Saved</title><style>body { font-family:Arial, sans-serif; text-align:center; padding:20px; background:#f5f5f5; }";
    html += ".container { max-width:500px; margin:0 auto; background:white; padding:30px; border-radius:10px; box-shadow:0 2px 10px rgba(0,0,0,0.1); }";
    html += "h1 { color:#28a745; }</style>";
    html += "<script>setTimeout(function(){ window.location.href = '/'; },5000);</script>";
    html += "</head><body><div class='container'><h1>Configuration Saved!</h1><p>Your settings have been saved. The device will restart shortly.</p></div></body></html>";
    server.send(200, "text/html", html);
    delay(2000);
    ESP.restart();
  } else {
    String html = "<!DOCTYPE html><html><head><meta charset='utf-8'><meta name='viewport' content='width=device-width, initial-scale=1'>";
    html += "<title>Error</title><style>body { font-family:Arial, sans-serif; text-align:center; padding:20px; background:#f5f5f5; }";
    html += ".container { max-width:500px; margin:0 auto; background:white; padding:30px; border-radius:10px; box-shadow:0 2px 10px rgba(0,0,0,0.1); }";
    html += "h1 { color:#dc3545; }</style></head><body><div class='container'><h1>Error</h1><p>Missing required parameters.</p>";
    html += "<a href='/'>Go Back</a></div></body></html>";
    server.send(400, "text/html", html);
  }
}

void handleReset() {
  preferences.begin("config", false);
  preferences.clear();
  preferences.end();
  String html = "<!DOCTYPE html><html><head><meta charset='utf-8'><meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<title>Configuration Reset</title><style>body { font-family:Arial, sans-serif; text-align:center; padding:20px; background:#f5f5f5; }";
  html += ".container { max-width:500px; margin:0 auto; background:white; padding:30px; border-radius:10px; box-shadow:0 2px 10px rgba(0,0,0,0.1); }";
  html += "h1 { color:#dc3545; }</style></head><body><div class='container'><h1>Configuration Reset!</h1>";
  html += "<p>All settings have been cleared. The device will restart shortly.</p></div></body></html>";
  server.send(200, "text/html", html);
  delay(2000);
  ESP.restart();
}

void mainConfigServer() {
  server.on("/", HTTP_GET, handleRoot);
  server.on("/save", HTTP_POST, handleSave);
  server.on("/reset", HTTP_GET, handleReset);
  server.begin();
  Serial.println("Web server started");
}

unsigned long lastWifiCheckTime = 0;
const unsigned long wifiCheckInterval = 30000;

// Debounce-related variables
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 50; // in milliseconds
bool lastMetalState = false;
bool metalDetected = false;

// Motor direction pins
#define IN1 13
#define IN2 12
#define IN3 14
#define IN4 27

// Metal detection Pins
#define METAL_DETECT_PIN 5
// Buzzer Pin
#define BUZZER_PIN 18

// Firebase configuration
#define DATABASE_URL "https://bot-projects-193c9-default-rtdb.asia-southeast1.firebasedatabase.app"
#define API_KEY "AIzaSyCyYA0c19EqGGKdUObqryuBBXUL9e1c4_o"

// Define Firebase objects
FirebaseData fbdoStream, fbdo;
FirebaseAuth auth;
FirebaseConfig config;
bool firebaseConnected = false;

// Firebase real-time database paths
#define RTDB_PATH "/metal-detection-bot"
String mainPath = RTDB_PATH;
String commandPath;

// Function declarations
void streamCallback(FirebaseStream data);
void streamTimeoutCallback(bool timeout);
void checkWiFiConnection();
void loadConfig();
void moveForward();
void moveBackward();
void turnLeft();
void turnRight();
void stopMotors();
void processCommand(String command);
void mainConfigServer();
void connectToFirebase();
void sendDetectionTrue();
void sendDetectionFalse();

void connectToFirebase() {
  // Configure Firebase
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;
  config.token_status_callback = tokenStatusCallback;
  config.max_token_generation_retry = 3;

  Firebase.reconnectNetwork(true);
  fbdo.setBSSLBufferSize(4096, 1024);
  fbdo.setResponseSize(2048);

  auth.user.email = "venkatnvs2005@gmail.com";
  auth.user.password = "venkat123";
  Firebase.begin(&config, &auth);

  // Wait for Firebase to be ready
  int retryCount = 0;
  while (!Firebase.ready() && retryCount < 5)
  {
    delay(500);
    retryCount++;
    Serial.print(".");
  }
  Serial.println("\nFirebase ready!");

  // Set up Firebase paths
  commandPath = mainPath;
  commandPath+= "/triggers/command";

  // Start Firebase streaming for manual commands
  if (!Firebase.RTDB.beginStream(&fbdoStream, commandPath))
  {
    Serial.printf("Stream failed: %s\n", fbdoStream.errorReason().c_str());
  }
  else
  {
    Firebase.RTDB.setStreamCallback(&fbdoStream, streamCallback, streamTimeoutCallback);
  }
}

void checkWiFiConnection() {
  if (WiFi.status() != WL_CONNECTED) {
      Serial.println("WiFi disconnected. Reconnecting...");
      stopMotors();
      delay(1000);
      WiFi.disconnect();
      WiFi.begin(storedSSID.c_str(), storedPassword.c_str());
  } else {
      Serial.println("WiFi is connected. No action needed.");
  }
}

void loadConfig() {
  preferences.begin("config", true);
  storedSSID = preferences.getString("ssid", "");
  storedPassword = preferences.getString("password", "");
  preferences.end();
}

void setup() {
  Serial.begin(115200);
  loadConfig();
  if (storedSSID == "" || storedPassword == "") {
    Serial.println("No WiFi credentials found, starting configuration portal.");
    configMode = true;
    WiFi.mode(WIFI_AP);
    WiFi.softAP(WIFI_SSID_AP.c_str());
    mainConfigServer();
    return;
  } else {
    // Attempt to connect to WiFi in STA mode.
    WiFi.mode(WIFI_STA);
    WiFi.begin(storedSSID.c_str(), storedPassword.c_str());
    Serial.print("Connecting to WiFi");
    unsigned long startAttemptTime = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 15000) {
      Serial.print(".");
      delay(500);
    }
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("\nFailed to connect. Starting configuration portal.");
      configMode = true;
      WiFi.mode(WIFI_AP);
      WiFi.softAP(WIFI_SSID_AP.c_str());
      mainConfigServer();
      return;
    } else {
      Serial.println();
      Serial.print("Connected! IP address: ");
      Serial.println(WiFi.localIP());
      mainConfigServer();
      connectToFirebase();
    }
  }

  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(METAL_DETECT_PIN, INPUT);

  Serial.println("Starting main loop...");
}

void loop() {
  server.handleClient();
  if (configMode) return;

  // Check and maintain WiFi connection
  if (millis() - lastWifiCheckTime > wifiCheckInterval) {
    checkWiFiConnection();
    lastWifiCheckTime = millis();
  }

  // Debounced metal detection
  if (METAL_DETECT_PIN >= 0) {
    bool reading = digitalRead(METAL_DETECT_PIN) == LOW;
    if (reading != lastMetalState) {
      lastDebounceTime = millis();
    }
    if ((millis() - lastDebounceTime) > debounceDelay) {
      if (reading != metalDetected) {
        metalDetected = reading;

        if (metalDetected) {
          Serial.println("Metal detected!");
          digitalWrite(BUZZER_PIN, HIGH);
          sendDetectionTrue();
        } else {
          Serial.println("No metal detected.");
          digitalWrite(BUZZER_PIN, LOW);
          sendDetectionFalse();
        }
      }
    }
    lastMetalState = reading;
  }
}

// Movement functions
void turnRight() {
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
}

void turnLeft() {
  digitalWrite(IN1, LOW); digitalWrite(IN2, HIGH);
  digitalWrite(IN3, LOW); digitalWrite(IN4, HIGH);
}

void moveBackward() {
  digitalWrite(IN1, LOW); digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
}

void moveForward() {
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW); digitalWrite(IN4, HIGH);
}

void stopMotors() {
  digitalWrite(IN1, LOW); digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW); digitalWrite(IN4, LOW);
}

void sendDetectionTrue() {
  if (!Firebase.ready()) {
    Serial.println("Firebase not ready, reconnecting...");
    connectToFirebase();
  }
  String newCommandPath = mainPath;
  newCommandPath += "/detection";
  if (Firebase.RTDB.setBool(&fbdo, newCommandPath, true)) {
    Serial.printf("Command sent to Firebase: %s\n", newCommandPath.c_str());
  } else {
    Serial.printf("Failed to send command to Firebase: %s\n", fbdo.errorReason().c_str());
  }
}

void sendDetectionFalse() {
  if (!Firebase.ready()) {
    Serial.println("Firebase not ready, reconnecting...");
    connectToFirebase();
  }
  String newCommandPath = mainPath;
  newCommandPath += "/detection";
  if (Firebase.RTDB.setBool(&fbdo, newCommandPath, false)) {
    Serial.printf("Command sent to Firebase: %s\n", newCommandPath.c_str());
  } else {
    Serial.printf("Failed to send command to Firebase: %s\n", fbdo.errorReason().c_str());
  }
}

void processCommand(String command) {
  if (command == "F"){
    moveForward();
    Serial.println("Moving forward");
  }
  else if (command == "B"){
    moveBackward();
    Serial.println("Moving backward");
  }
  else if (command == "L"){
    turnLeft();
    Serial.println("Turning left");
  }
  else if (command == "R"){
    turnRight();
    Serial.println("Turning right");
  }
  else if (command == "S"){
    stopMotors();
    Serial.println("Stopped");
  }
  else {
    Serial.println("Unknown command: ");
    Serial.println(command);
  }
}

void streamCallback(FirebaseStream data)
{
  Serial.println("Stream event received!");
  if (data.dataType() == "string")
  {
    String action = data.stringData();
    Serial.printf("Action received: %s\n", action.c_str());
    processCommand(action);
  }
}

void streamTimeoutCallback(bool timeout)
{
  if (timeout)
  {
    Serial.println("Stream timeout occurred, reconnecting...");
  }
  else
  {
    Serial.println("Stream disconnected, trying to reconnect...");
  }
  Firebase.RTDB.beginStream(&fbdoStream, commandPath);
}