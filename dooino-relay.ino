#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <EEPROM.h>

ESP8266WebServer server(80);

const char* dooinoId = "93499fe06a80";
const char* ssid = "DOOINO-93499fe06a80";
const char* passphrase = "guest1234";

char hostString[16] = {0};

const char* jsonHeader = "application/json";
const int firstRelay  = 5;
String relayStatus = "0";
String dnsName;

String st;
String content;
int statusCode;

// Temperature
// #include "DHT.h"
// #define DHTPIN D4
// #define DHTTYPE DHT22

// DHT dht(DHTPIN, DHTTYPE);
// bool firstRun = true;

String getMainMessage() {
  String message = "{";
  message += "\"name\": \"Dooino One Relay v.1.0\",";
  message += "\"id\": \"93499fe06a80\",";
  message += "\"in\": [";
    message += "{";
      message += "\"name\": \"on\",";
      message += "\"action\": \"http://" + dnsName + ".local/in/on\"";
    message += "},";
    message += "{";
      message += "\"name\": \"off\",";
      message += "\"action\": \"http://" + dnsName + ".local/in/off\"";
    message += "}";
  message += "],";
  message += "\"out\": [";
    message += "{";
      message += "\"name\": \"status\",";
      message += "\"action\": \"http://" + dnsName + ".local/out/status\"";
    message += "}";
  message += "]";
  message += "}";

  return message;
}

String getStatusMessage() {
  String message = "{";
  message += "\"value\": \"" + relayStatus + "\",";
  message += "\"links\": {";
  message += "\"main\": \"http://" + dnsName + ".local/manifest.json\"";
  message += "}";
  message += "}";

  return message;
}

String getSuccessMessage() {
  String message = "{";
  message += "\"status\": \"success\",";
  message += "\"links\": {";
  message += "\"main\": \"http://" + dnsName + ".local/manifest.json\"";
  message += "}";
  message += "}";

  return message;
}

String getNotFoundMessage() {
  String message = "{";
  message += "\"message\": \"Url not found\",";
  message += "\"links\": {";
  message += "\"main\": \"http://" + dnsName + ".local/manifest.json\"";
  message += "}";
  message += "}";

  return message;
}

void switchRelayOn(const int relay) {
  digitalWrite(relay, HIGH);
  relayStatus = "1";
  server.send(200, jsonHeader, getSuccessMessage());
}

void switchRelayOff(const int relay) {
  digitalWrite(relay, LOW);
  relayStatus = "0";
  server.send(200, jsonHeader, getSuccessMessage());
}

String readStoredSsid(void) {
  return readStoreString(0, 32);
}

String readStoredPassword(void) {
  return readStoreString(32, 96);
}

String readStoreString(int startPoint, int endPoint) {
  String res = "";
  for (int i = startPoint; i < endPoint; ++i) {
    res += char(EEPROM.read(i));
  }

  return res;
}

void dooinoSetup(void) {
  pinMode(firstRelay, OUTPUT);

  // dht.begin();
}

void setup() {
  Serial.begin(115200);

  dooinoSetup();

  // Initialize EEPROM (for storing Wifi Credentials: SSID and Password)
  EEPROM.begin(512);
  delay(10);

  dnsName = ssid;

  Serial.println();
  Serial.println();
  Serial.println("Welcome to Dooino Systems");
  Serial.println("You're Dooino with " + String(dooinoId) + "is booting...");

  Serial.println();
  Serial.println();
  Serial.println("Reading EEPROM ssid");
  String esid = readStoredSsid();
  Serial.print("- Current SSID: ");
  Serial.println(esid);

  Serial.println("Reading EEPROM pass");
  String epass = readStoredPassword();
  Serial.print("- Current PASS: ");
  Serial.println(epass);
  Serial.println();
  
  // if (esid.length() > 1) {
  if (!esid.equals("")) {
    Serial.println("It looks there is a stored SSID and Password.");
    if (tryToConnectWifi(esid.c_str(), epass.c_str())) {
      createReadyWebServer();
      return;
    } 
  } else {
    Serial.println("It looks there is NOT a stored SSID and Password.");
  }
  
  setupAP();
}

bool tryToConnectWifi(const char* esid, const char* epass) {
  WiFi.begin(esid, epass);

  int c = 0;
  Serial.println("Waiting for Wifi to connect...");  
  while (c < 20) {
    if (WiFi.status() == WL_CONNECTED) { 
      Serial.println("Connected to WiFi!");  
      Serial.println("");

      return true;
    }
    
    delay(500);
    Serial.print(".");    
    c++;
  }

  Serial.println("");
  Serial.println("Connect timed out, opening AP for configuration...");
  
  return false;
} 

void createReadyWebServer(void)
{
  createWebServer(1);
}

void createConfigurationWebServer(void)
{
  createWebServer(0);
}

void createWebServer(int readyServer)
{
  Serial.println("WiFi connected");
  Serial.print("Local IP: ");
  Serial.println(WiFi.localIP());
  Serial.print("SoftAP IP: ");
  Serial.println(WiFi.softAPIP());
  
  if (readyServer) {
    Serial.println("Creating Server for interacting with your Arduino..."); 
  
    Serial.println("Starting DNS responder..."); 
    if (!MDNS.begin(ssid)) {
      Serial.println("Error setting up MDNS responder!");
    } else {
      Serial.println("mDNS responder started");
    }
    
    Serial.println("Registering DNS service..."); 
    MDNS.addService("dooino", "tcp", 80);
  } else {
    Serial.println("Creating Server for configuring your Arduino..."); 
  }
  
  server.on("/manifest.json", []() {
    server.send(200, "application/json", getMainMessage());
  });

  server.on("/out/status", [](){
    server.send(200, "application/json", getStatusMessage());
  });

  server.on("/in/on", [](){
    switchRelayOn(firstRelay);
  });

  server.on("/in/off", [](){
    switchRelayOff(firstRelay);
  });

  server.on("/wifis", []() {
      IPAddress ip = WiFi.softAPIP();
      String ipStr = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);
      content = "<!DOCTYPE HTML>\r\n<html>Hello from ESP8266 at ";
      content += ipStr;
      content += "<p>";
      content += st;
      content += "</p><form method='get' action='setting'><label>SSID: </label><input name='ssid' length=32><input name='pass' length=64><input type='submit'></form>";
      content += "</html>";
      server.send(200, "text/html", content);  
  });
  
  server.on("/setting", []() {
      String qsid = server.arg("ssid");
      String qpass = server.arg("pass");
      if (qsid.length() > 0 && qpass.length() > 0) {
        Serial.println("clearing eeprom");
        for (int i = 0; i < 96; ++i) { EEPROM.write(i, 0); }
        Serial.println(qsid);
        Serial.println("");
        Serial.println(qpass);
        Serial.println("");
          
        Serial.println("writing eeprom ssid:");
        for (int i = 0; i < qsid.length(); ++i)
          {
            EEPROM.write(i, qsid[i]);
            Serial.print("Wrote: ");
            Serial.println(qsid[i]); 
          }
        Serial.println("writing eeprom pass:"); 
        for (int i = 0; i < qpass.length(); ++i)
          {
            EEPROM.write(32+i, qpass[i]);
            Serial.print("Wrote: ");
            Serial.println(qpass[i]); 
          }    
        EEPROM.commit();
        content = "{\"Success\":\"saved to eeprom... reset to boot into new wifi\"}";
        statusCode = 200;
        server.send(statusCode, "application/json", content);
        rebootDooino();
      } else {
        content = "{\"Error\":\"404 not found\"}";
        statusCode = 404;
        Serial.println("Sending 404");
        server.send(statusCode, "application/json", content);
      }
  });

  server.on("/cleareeprom", []() {
    content = "<!DOCTYPE HTML>\r\n<html>";
    content += "<p>Clearing the EEPROM</p></html>";
    server.send(200, "text/html", content);
    Serial.println("clearing eeprom");
    for (int i = 0; i < 96; ++i) { EEPROM.write(i, 0); }
    EEPROM.commit();
  });

  server.onNotFound([]() {
    server.send(200, jsonHeader, getNotFoundMessage());
  });

  server.begin();
  
  Serial.println("Server started"); 
}

void setupAP(void) {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  Serial.println("Scanning available WiFi networks...");
  int n = WiFi.scanNetworks();
  Serial.println("Scan done!");
  if (n == 0)
    Serial.println("No networks found!");
  else
  {
    Serial.print(n);
    Serial.println(" networks found");
    for (int i = 0; i < n; ++i)
     {
      // Print SSID and RSSI for each network found
      Serial.print(i + 1);
      Serial.print(": ");
      Serial.print(WiFi.SSID(i));
      Serial.print(" (");
      Serial.print(WiFi.RSSI(i));
      Serial.print(")");
      Serial.println((WiFi.encryptionType(i) == ENC_TYPE_NONE)?" ":"*");
      delay(10);
     }
  }
  Serial.println(""); 
  st = "<ol>";
  for (int i = 0; i < n; ++i)
    {
      // Print SSID and RSSI for each network found
      st += "<li>";
      st += WiFi.SSID(i);
      st += " (";
      st += WiFi.RSSI(i);
      st += ")";
      st += (WiFi.encryptionType(i) == ENC_TYPE_NONE)?" ":"*";
      st += "</li>";
    }
  st += "</ol>";
  delay(100);
  WiFi.softAP(ssid, passphrase, 6);
  Serial.println("softap");

  createConfigurationWebServer();
  Serial.println("over");
}

void rebootDooino(void) {
  WiFi.forceSleepBegin(); wdt_reset(); ESP.restart(); while(1)wdt_reset();  
}

void loop() {
  server.handleClient();
}
