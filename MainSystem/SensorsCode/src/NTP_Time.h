struct timeData {
	char* sec;
	char* min;
	char* hour;
	char* day;
	char* month;
	char* year;
};

class NTP_Time {
private:
  const char* ssid = "Hirsch-5G";
  const char* password = "9512648172";

  const char* ntpServer = "pool.ntp.org";
  const long gmtOffset_sec = -28800;
  const uint16_t daylightOffset_sec = 3600;

public:
  void ntpSetup() {
    // Connect to Wi-Fi
    Serial.print("Connecting to ");
    Serial.println(ssid);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }
    Serial.println("");
    Serial.println("WiFi connected.");

    // Init and get the time
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    //printLocalTime();

    //disconnect WiFi as it's no longer needed
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
  }
  /*void printLocalTime(timeData &localTime){
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return;
  }
  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
  Serial.print("Day of week: ");
  Serial.println(&timeinfo, "%A");
  Serial.print("Month: ");
  Serial.println(&timeinfo, "%B");
  Serial.print("Day of Month: ");
  Serial.println(&timeinfo, "%d");
  Serial.print("Year: ");
  Serial.println(&timeinfo, "%Y");
  Serial.print("Hour: ");
  Serial.println(&timeinfo, "%H");
  Serial.print("Hour (12 hour format): ");
  Serial.println(&timeinfo, "%I");
  Serial.print("Minute: ");
  Serial.println(&timeinfo, "%M");
  Serial.print("Second: ");
  Serial.println(&timeinfo, "%S");

	  strftime(localTime.sec  , 2, "%S",&timeinfo);
	  strftime(localTime.min  , 2, "%M",&timeinfo);
	  strftime(localTime.hour , 2, "%H",&timeinfo);
	  strftime(localTime.day  , 2, "%d",&timeinfo);
	  strftime(localTime.month, 2, "%m",&timeinfo);
	  strftime(localTime.year , 4, "%Y",&timeinfo);
}*/

  void updateLocalTime(timeData &localTime) {
      struct tm timeinfo;
    if(!getLocalTime(&timeinfo)){
      Serial.println("Failed to obtain time");
    return;
    }
	  strftime(localTime.sec  , 2, "%S",&timeinfo);
	  strftime(localTime.min  , 2, "%M",&timeinfo);
	  strftime(localTime.hour , 2, "%H",&timeinfo);
	  strftime(localTime.day  , 2, "%d",&timeinfo);
	  strftime(localTime.month, 2, "%m",&timeinfo);
	  strftime(localTime.year , 4, "%Y",&timeinfo);
  }
};