#include <WiFi.h>

const char* ssid = "Hirsch"; //Network name
const char* password = "9512648172"; //Network password

void wifiInit()
{
  // Connect to Wi-Fi
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("WiFi connected.");
  Serial.println( WiFi.localIP());
}
