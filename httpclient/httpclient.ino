#include <WiFiS3.h>
#include <RTC.h>
#include <DHT.h>
#include <ArduinoHttpClient.h>

// ================= CONFIGURATION =================
const char* SSID_NAME     = "NAME";
const char* SSID_PASS     = "PASSPASS";

// Worker Config
const char* SERVER_ADDR   = "EXAMPLE.COM"; 
const char* API_KEY       = "MY-API-KEY";
const int ROOM_ID         = 1;

// Sensor Config
#define DHTPIN 2     
#define DHTTYPE DHT11
// =================================================

DHT dht(DHTPIN, DHTTYPE);
WiFiSSLClient wifi;
HttpClient client = HttpClient(wifi, SERVER_ADDR, 443);

// Sleep duration: 1 hour (3600 seconds)
const int SLEEP_SECONDS = 3600; 

void setup() {
  Serial.begin(9600);
  while (!Serial) delay(10);

  dht.begin();
  RTC.begin();

  Serial.println("Birdcage Monitor Starting...");
  
  // 1. Connect to Wi-Fi immediately to get the time
  connectToWiFi();

  // 2. Sync the internal RTC with the Internet (NTP)
  // syncRTC();
}

void loop() {
  Serial.println("--------------------------------");

  // Ensure Wi-Fi is connected (it might be off from sleeping)
  connectToWiFi();

  // 1. Get Current Time for logging
  RTCTime currentTime;
  RTC.getTime(currentTime);
  Serial.print("Current Time (UTC): ");
  Serial.println(String(currentTime));

  // 2. Read Sensor
  float t = dht.readTemperature();
  float h = dht.readHumidity();
  
  if (isnan(t) || isnan(h)) {
    Serial.println("Failed to read from DHT sensor!");
  } else {
    Serial.print("Temperature: ");
    Serial.print(t);
    Serial.println(" *C");
    
    // 3. Send to Cloudflare
    sendDataToWorker(t, h);
  }

  // 4. Disconnect Wi-Fi to save power
  WiFi.disconnect();
  WiFi.end(); 
  Serial.println("Wi-Fi disconnected.");

  // 5. Enter Low Power Sleep
  //enterDeepSleep(SLEEP_SECONDS);
  delay(SLEEP_SECONDS * 1000);
}

void connectToWiFi() {
  if (WiFi.status() == WL_CONNECTED) return;

  Serial.print("Connecting to Wi-Fi: ");
  Serial.println(SSID_NAME);

  while (WiFi.status() != WL_CONNECTED) {
    WiFi.begin(SSID_NAME, SSID_PASS);
    delay(2000);
    Serial.print(".");
  }
  Serial.println("\nConnected!");
}

void syncRTC() {
  Serial.println("Syncing time with NTP Server...");

  // WiFi.getTime() fetches Unix timestamp from pool.ntp.org
  // It returns 0 if it fails. We try a few times.
  unsigned long epochTime = 0;
  int retries = 0;

  while (epochTime == 0 && retries < 10) {
    epochTime = WiFi.getTime();
    if (epochTime == 0) {
      Serial.println("NTP fetch failed, retrying...");
      delay(1000);
      retries++;
    }
  }

  if (epochTime > 0) {
    // Update the internal RTC with the fetched time
    RTCTime timeToSet(epochTime);
    RTC.setTime(timeToSet);
    Serial.print("Time synced successfully! Unix: ");
    Serial.println(epochTime);
  } else {
    Serial.println("Failed to sync time. RTC may be inaccurate.");
  }
}

void sendDataToWorker(float temperature, float humidity) {
  String payload = "{\"room_id\": " + String(ROOM_ID) + 
                   ", \"temperature\": " + String(temperature, 1) +
                   ", \"humidity\": " + String(humidity, 1) + "}";

  Serial.println("Sending POST...");

  client.beginRequest();
  client.post("/op/add");
  client.sendHeader("Content-Type", "application/json");
  client.sendHeader("x-api-key", API_KEY);
  client.sendHeader("Content-Length", payload.length());
  client.beginBody();
  client.print(payload);
  client.endRequest();

  int statusCode = client.responseStatusCode();
  String response = client.responseBody();

  Serial.print("Status: ");
  Serial.println(statusCode);
}

