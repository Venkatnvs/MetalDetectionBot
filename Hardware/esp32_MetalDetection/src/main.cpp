#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"
#include <WebServer.h>
#include <Preferences.h>
#include <TinyGPS++.h>
#include <HardwareSerial.h>

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

// GPS update interval
unsigned long lastGPSUpdateTime = 0;
const unsigned long gpsUpdateInterval = 10000; // Update GPS every 10 seconds

// GPS status tracking
bool gpsHasFix = false;
unsigned long lastValidGPSTime = 0;
const unsigned long gpsTimeout = 30000; // Consider GPS lost after 30 seconds without valid data
int gpsSatelliteCount = 0;
float gpsHDOP = 99.9; // Horizontal Dilution of Precision (lower is better)
unsigned long gpsStartTime = 0;
bool gpsConfigured = false;
unsigned long lastGPSReconfigTime = 0;
const unsigned long gpsReconfigInterval = 300000; // Reconfigure GPS every 5 minutes if no fix

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

// GPS Configuration
#define GPS_RX_PIN 16
#define GPS_TX_PIN 17
#define GPS_BAUD 9600
HardwareSerial gpsSerial(2); // Use Serial2 for GPS
TinyGPSPlus gps;

// Firebase configuration
#define DATABASE_URL "https://bot-projects-193c9-default-rtdb.asia-southeast1.firebasedatabase.app"
#define API_KEY "AIzaSyCyYA0c19EqGGKdUObqryuBBXUL9e1c4_o"

// Define Firebase objects
FirebaseData fbdoStream, fbdo;
FirebaseAuth auth;
FirebaseConfig config;
bool firebaseConnected = false;

// Firebase real-time database paths
#define RTDB_PATH "/metal-detection-bot-2"
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
void updateGPS();
void sendGPSLocation(float latitude, float longitude);
void sendGPSStatus(bool hasFix, int satellites, float hdop);
bool isGPSValid();
void configureGPS();
void sendNMEACommand(String command);

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

  // Initialize GPS
  gpsSerial.begin(GPS_BAUD, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
  Serial.println("GPS initialized on Serial2");
  
  // Wait a bit for GPS to initialize
  delay(1000);
  
  // Configure GPS for faster fix acquisition
  configureGPS();
  gpsConfigured = true;
  gpsStartTime = millis();
  lastGPSReconfigTime = millis();

  Serial.println("Starting main loop...");
}

void loop() {
  server.handleClient();
  if (configMode) return;

  // Update GPS data continuously (optimized processing)
  updateGPS();

  // Periodically update GPS location to Firebase (saves only one latest location)
  if (millis() - lastGPSUpdateTime > gpsUpdateInterval) {
    if (isGPSValid()) {
      sendGPSLocation(gps.location.lat(), gps.location.lng());
    } else {
      // Send GPS status even when no fix (so frontend knows GPS is not available)
      sendGPSStatus(false, gpsSatelliteCount, gpsHDOP);
      Serial.println("GPS: No valid fix available");
      
      // If GPS has been running for more than 5 minutes without fix, try reconfiguring
      if (millis() - lastGPSReconfigTime > gpsReconfigInterval) {
        Serial.println("GPS: Attempting to reconfigure for better signal...");
        configureGPS();
        lastGPSReconfigTime = millis();
      }
    }
    lastGPSUpdateTime = millis();
  }

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
          // Also update GPS location immediately when metal is detected (if available)
          if (isGPSValid()) {
            sendGPSLocation(gps.location.lat(), gps.location.lng());
          } else {
            Serial.println("GPS location not available - metal detection still recorded");
            // Send GPS status to indicate no fix
            sendGPSStatus(false, gpsSatelliteCount, gpsHDOP);
          }
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

void moveForward() {
  digitalWrite(IN1, LOW); digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
}

void moveBackward() {
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

void updateGPS() {
  // Process all available GPS data
  unsigned long startTime = millis();
  const unsigned long maxProcessingTime = 100; // Don't spend more than 100ms processing GPS
  
  while (gpsSerial.available() > 0 && (millis() - startTime < maxProcessingTime)) {
    char c = gpsSerial.read();
    if (gps.encode(c)) {
      // Check if we have a valid fix
      if (gps.location.isValid()) {
        // Check fix quality and satellite count
        gpsSatelliteCount = gps.satellites.value();
        gpsHDOP = gps.hdop.hdop();
        
        // Consider GPS valid if we have at least 3 satellites and reasonable HDOP
        bool wasValid = gpsHasFix;
        gpsHasFix = (gpsSatelliteCount >= 3 && gpsHDOP < 10.0);
        
        if (gpsHasFix) {
          lastValidGPSTime = millis();
          if (!wasValid) {
            Serial.printf("GPS fix acquired! Satellites: %d, HDOP: %.2f\n", gpsSatelliteCount, gpsHDOP);
          }
        }
      } else {
        // No valid location yet
        gpsHasFix = false;
      }
    }
  }
  
  // Check if GPS signal is lost (timeout)
  if (gpsHasFix && (millis() - lastValidGPSTime > gpsTimeout)) {
    gpsHasFix = false;
    Serial.println("GPS signal lost (timeout)");
  }
}

bool isGPSValid() {
  return gpsHasFix && gps.location.isValid() && (millis() - lastValidGPSTime < gpsTimeout);
}

void sendNMEACommand(String command) {
  // Calculate checksum for NMEA command
  uint8_t checksum = 0;
  for (int i = 1; i < command.length(); i++) {
    if (command[i] == '*') break;
    checksum ^= command[i];
  }
  
  // Format: $COMMAND*CHECKSUM\r\n
  String fullCommand = command + "*";
  if (checksum < 16) fullCommand += "0";
  fullCommand += String(checksum, HEX);
  fullCommand.toUpperCase();
  fullCommand += "\r\n";
  
  gpsSerial.print(fullCommand);
  Serial.print("GPS Config: ");
  Serial.print(fullCommand);
  delay(100); // Small delay between commands
}

void configureGPS() {
  Serial.println("Configuring GPS for faster fix acquisition...");
  
  // Wait a bit more for GPS module to be ready
  delay(500);
  
  // Clear any pending data
  while (gpsSerial.available() > 0) {
    gpsSerial.read();
  }
  
  // 1. Set update rate to 10Hz (10 updates per second) - faster acquisition
  // PMTK_SET_NMEA_UPDATE_RATE_10HZ (100ms = 10Hz)
  sendNMEACommand("$PMTK220,100");
  
  // 2. Set NMEA sentence output frequency
  // Enable GGA (Global Positioning System Fix Data) - most important for location
  // Enable RMC (Recommended Minimum Specific GNSS Data)
  // PMTK_SET_NMEA_OUTPUT_RMCGGA - RMC and GGA only (reduces data load, faster processing)
  sendNMEACommand("$PMTK314,0,1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0");
  
  // 3. Enable hot start mode for faster acquisition (uses last known position)
  // PMTK_SET_HOT_START
  sendNMEACommand("$PMTK101");
  
  // 4. Set dynamic platform model to "Portable" (better for moving objects)
  // PMTK_SET_NAV_SPEED_THRESHOLD - allows faster position updates
  sendNMEACommand("$PMTK386,0.2");
  
  // 5. Enable SBAS (Satellite-Based Augmentation System) for better accuracy and faster fix
  // PMTK_ENABLE_SBAS
  sendNMEACommand("$PMTK313,1");
  
  // 6. Enable WAAS (Wide Area Augmentation System) if available
  // PMTK_ENABLE_WAAS - improves accuracy and can help with faster fix
  sendNMEACommand("$PMTK301,2");
  
  // 7. Set SBAS search mode - allows using weaker signals for faster acquisition
  // PMTK_SET_SBAS_SEARCH_MODE
  sendNMEACommand("$PMTK319,0");
  
  // 8. Disable power save mode for faster response and continuous tracking
  // PMTK_SET_POWER_SAVE_MODE - 0 = normal mode (no power saving)
  sendNMEACommand("$PMTK225,0");
  
  // 9. Set fix interval (continuous tracking, no sleep)
  // PMTK_API_SET_FIX_CTL - 100ms fix interval, continuous
  sendNMEACommand("$PMTK300,100,0,0,0,0");
  
  // 10. Enable DGPS (Differential GPS) mode for better accuracy
  // PMTK_API_SET_DGPS_MODE - 2 = WAAS enabled
  // (Already set above with PMTK301,2)
  
  // 11. Request hot start again to ensure fast acquisition
  // PMTK_CMD_HOT_START - uses last known position for faster fix
  sendNMEACommand("$PMTK101");
  
  Serial.println("GPS configuration complete!");
  Serial.println("Waiting for GPS fix...");
  Serial.println("Note: First fix may take 30-60 seconds (cold start)");
  Serial.println("Subsequent fixes will be much faster (hot start)");
  
  // Give GPS time to process commands
  delay(500);
  
  // Clear buffer again after configuration
  while (gpsSerial.available() > 0) {
    gpsSerial.read();
  }
}

void sendGPSLocation(float latitude, float longitude) {
  if (!Firebase.ready()) {
    Serial.println("Firebase not ready, reconnecting...");
    connectToFirebase();
  }
  String locationPath = mainPath;
  locationPath += "/location";
  
  FirebaseJson json;
  json.set("latitude", latitude);
  json.set("longitude", longitude);
  json.set("timestamp", millis());
  json.set("satellites", gpsSatelliteCount);
  json.set("hdop", gpsHDOP);
  json.set("hasFix", gpsHasFix);
  
  if (Firebase.RTDB.setJSON(&fbdo, locationPath, &json)) {
    Serial.printf("GPS location sent to Firebase: %.6f, %.6f (Sats: %d, HDOP: %.2f)\n", 
                  latitude, longitude, gpsSatelliteCount, gpsHDOP);
  } else {
    Serial.printf("Failed to send GPS location to Firebase: %s\n", fbdo.errorReason().c_str());
  }
  
  // Also send GPS status separately
  sendGPSStatus(gpsHasFix, gpsSatelliteCount, gpsHDOP);
}

void sendGPSStatus(bool hasFix, int satellites, float hdop) {
  if (!Firebase.ready()) {
    return;
  }
  String statusPath = mainPath;
  statusPath += "/gpsStatus";
  
  FirebaseJson json;
  json.set("hasFix", hasFix);
  json.set("satellites", satellites);
  json.set("hdop", hdop);
  json.set("timestamp", millis());
  
  Firebase.RTDB.setJSON(&fbdo, statusPath, &json);
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