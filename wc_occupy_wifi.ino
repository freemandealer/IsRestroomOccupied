#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>

#ifndef STASSID
#define STASSID "CU_b659"
#define STAPSK  "txrtv47h"
#endif

ADC_MODE(ADC_VCC); // config A0 to read system voltage

const char* ssid = STASSID;
const char* password = STAPSK;

ESP8266WebServer server(80);

const int led = LED_BUILTIN;
const int OFF_THRESHOLD = 20;

int inPin  = 5;
int stat = LOW;
unsigned short off_count = 0;
bool wifi_on = 1;

void handleRoot() {
  if (stat == HIGH) {
    Serial.println("HIGH");
    server.send(200, "text/html", "<body style=\"background-color:#BA221F;\"/><h1 style=\"color:#FFFFFFFF\"/><center/>Occupied");
  } else {
    Serial.println("LOW");
    server.send(200, "text/html", "<body style=\"background-color:#77BA94;\"/><h1 style=\"color:#FFFFFFFF\"/><center/>Free");
  }      
}

void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain; charset=utf-8", message);
}

void setup(void) {
  pinMode(led, OUTPUT);
  pinMode(inPin, INPUT);
  digitalWrite(led, HIGH); // HIGN means off
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  if (MDNS.begin("esp8266")) {
    Serial.println("MDNS responder started");
  }

  server.on("/", handleRoot);
  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("HTTP server started");
}

void loop(void) {
  stat = digitalRead(inPin);
  if (stat == HIGH ) {
    off_count = 0;
    if (wifi_on == 0) {
      Serial.print("Wake up wifi");
      WiFi.mode( WIFI_STA);
      WiFi.begin(ssid, password);
      wifi_on = 1;
    }
  }
  else {
    ++off_count;
    if (off_count > OFF_THRESHOLD && wifi_on == 1) {
      Serial.print("Turn off wifi to save energy");
      WiFi.mode( WIFI_OFF);
      wifi_on = 0;
    }
  }
  server.handleClient();
  MDNS.update();
  delay(1000);
}
