#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include "user_interface.h"
#include "gpio.h"

#ifndef STASSID
#define STASSID "CU_b659"
#define STAPSK  "txrtv47h"
#endif

#define ENABLE_DEBUG 0
void log(const char* str)
{
#if ENABLE_DEBUG
  log(str);
#else
  return;
#endif
}

ADC_MODE(ADC_VCC); // config A0 to read system voltage

const char* ssid = STASSID;
const char* password = STAPSK;

ESP8266WebServer server(80);

const int led = LED_BUILTIN;
#if ENABLE_DEBUG
const int OFF_THRESHOLD = 10;
#else
const int OFF_THRESHOLD = 60;
#endif

int inPin  = 5;
int stat = LOW;
unsigned short off_count = 0;

void handleRoot() {
  if (stat == HIGH) {
    log("HIGH");
    server.send(200, "text/html", "<body style=\"background-color:#BA221F;\"/><h1 style=\"color:#FFFFFFFF\"/><center/>Occupied");
  } else {
    log("LOW");
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

void wifi_start()
{
  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  if (WiFi.waitForConnectResult(20000) == WL_CONNECTED) {   // timeout 30s
    log("");
    log("Connected to ");
    log(ssid);
#if ENABLE_DEBUG
    log("IP address: ");
    Serial.println(WiFi.localIP());
    uint8_t macAddr[6];
    WiFi.macAddress(macAddr);
    log("MAC address: ");
    Serial.printf("%02x:%02x:%02x:%02x:%02x:%02x\n", macAddr[0], macAddr[1], macAddr[2], macAddr[3], macAddr[4], macAddr[5]);
#endif
  }
  else {
    log("connect timeout");
    log("ignore this connection, pass to later sleep and wait for wakeup & retry connect");
  }
}

void wifi_stop()
{
  WiFi.mode(WIFI_OFF);
}

void powersaving()
{
  log("powersaving");
  wifi_fpm_set_sleep_type(LIGHT_SLEEP_T);
  gpio_pin_wakeup_enable(GPIO_ID_PIN(inPin), GPIO_PIN_INTR_HILEVEL);
  wifi_fpm_open();
  wifi_fpm_do_sleep(0xFFFFFFF);
  delay(10); // must delay, otherwise WDT will reset
  /* ============================== */
  log("wake up now");
  /* ============================== */
  wifi_fpm_do_wakeup();
  wifi_fpm_close();
  gpio_pin_wakeup_disable();
}

void setup(void) {
  gpio_init();
  pinMode(led, OUTPUT);
  pinMode(inPin, INPUT);
  digitalWrite(led, HIGH); // HIGN means off
  Serial.begin(115200);

  wifi_start();

  wifi_set_sleep_type(LIGHT_SLEEP_T);
  log("light sleep mode enabled");

  server.on("/", handleRoot);
  server.onNotFound(handleNotFound);

  server.begin();
  log("HTTP server started");
}

void loop(void) {
  stat = digitalRead(inPin);
  if (stat == HIGH ) {
    off_count = 0;
  }
  else {
    ++off_count;
    if (off_count > OFF_THRESHOLD) {
      log("go to sleep");
      wifi_stop();
      delay(10); // must delay, otherwise WDT will reset
      powersaving();
      wifi_start();
    }
  }
  server.handleClient();
  digitalWrite(led, HIGH); // HIGN means off
  delay(1000);
}
