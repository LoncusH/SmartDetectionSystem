#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "FS.h"
#include "SD.h"
#include "SPI.h"

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

void generateHTML() 
{
  String html = "<!DOCTYPE HTML><html>\
<head>\
  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\
  <link rel=\"stylesheet\" type=\"text/css\" href=\"style.css\">\
  <link rel=\"icon\"  type=\"image/png\" href=\"favicon.png\">\
  <title>ESP Web File Server</title>\
</head>\
<body>\
  <h1>Smart Wellness Device</h1>\
  <p>Data log files below.</p>";

  File root = SD.open("/logs/");
  File entry = root.openNextFile();
  while (entry) {
    if (!entry.isDirectory()) {
      html += "<li>" + String(entry.name()) + " (" + String(entry.size()) + " bytes)";
      html += " <a href=\"/" + String(entry.name()) + "\">View</a>";      
      html += " <a href=\"/" + String(entry.name()) + "\" download>Download</a>";
      //html += " <a href=\"/delete?name=" + String(entry.name()) + "\">Delete</a></li>";
    }
    entry.close();
    entry = root.openNextFile();
  }
  html += "</body>\
</html>";

  if (SD.open("/index.html")) 
  {
    deleteFile(SD, "/index.html");
  }
  appendFile(SD, "/index.html", html.c_str());

}

void fileServInit() 
{
  generateHTML();
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SD, "/index.html", "text/html");
  });

  server.serveStatic("/", SD, "/");

  File root = SD.open("/logs/");
  File entry = root.openNextFile();
  while (entry) {
    if (!entry.isDirectory()) {
      String temp = "/logs/" + String(entry.name());
      Serial.println(temp);
      server.on(temp, HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(SD, request->url(), "text/html");
      });
      server.serveStatic(entry.name(), SD, temp.c_str());
    }
    entry.close();
    entry = root.openNextFile();
}

  server.begin();
}