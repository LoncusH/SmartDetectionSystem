struct timeData {
	char sec[3];
	char min[3];
	char hour[3];
	char day[3];
	char month[3];
	char year[5];
};

const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = -28800;
const int daylightOffset_sec = 3600;

void ntpSetup()
{
    // Init and get the time
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
}

void updateLocalTime(timeData &currentTime) {
    struct tm timeinfo;
    if(!getLocalTime(&timeinfo)){
        Serial.println("Failed to obtain time");
        return;
    }
	strftime(currentTime.sec  , 3, "%S",&timeinfo);
	strftime(currentTime.min  , 3, "%M",&timeinfo);
	strftime(currentTime.hour , 3, "%H",&timeinfo);
	strftime(currentTime.day  , 3, "%d",&timeinfo);
	strftime(currentTime.month, 3, "%m",&timeinfo);
	strftime(currentTime.year , 5, "%Y",&timeinfo);
}
