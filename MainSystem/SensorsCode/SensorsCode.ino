#include <array>
#include <string>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <BH1750.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include "FS.h"
#include "SD.h"
#include "SPI.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "src/MotionSensor.h"
#include "src/LightSensor.h"
#include "src/UltrasonicSensor.h"
#include "src/wifi.h"
#include "src/sdFuncs.h"
#include "src/ntpTime.h"
#include "src/fileServ.h"
#include <WiFi.h>
#include "time.h"

unsigned long previousMillis = 0;
const int interval = 1000; // Interval for logging data (1 second)

// These are the handles to the separate threads. Each individual thread will run one of the sensors
TaskHandle_t motionThread;
TaskHandle_t proximityThread;
TaskHandle_t lightThread;
TaskHandle_t DataloggingThread;


#define MOTION_BUFFER_SIZE 500
#define MOTION_BUFFER_CUTOFF 480

// Emergency alert time in minutes
// Every n minutes, if there has been no movement detected, an emergency alert will be sent
// Ex. If EMERGENCY_ALERT_TIME == 2, then every 2 minutes, if motion hasn't been detected, an alert notification will be sent to the Android device
#define EMERGENCY_ALERT_TIME 2

////////////////////////////////////////////////////////////////////////////////////////
// Pin setup for sensors
////////////////////////////////////////////////////////////////////////////////////////
const uint8_t motionSensorInputPin = 4;
const uint8_t trigPin = 2;  // ultrasonic sensor trigger pin
const uint8_t echoPin = 15; // ultrasonic sensor echo pin

//Time Server Info struct
timeData currentTime;

////////////////////////////////////////////////////////////////////////////////////////
// calibration time (in seconds) for the sensors to be calibrated
////////////////////////////////////////////////////////////////////////////////////////
const int calibrationTime = 5;

////////////////////////////////////////////////////////////////////////////////////////
// Dynamically allocated MotionSensor object which will be used for interacting with the motion sensor
////////////////////////////////////////////////////////////////////////////////////////
MotionSensor *motionSensor;

////////////////////////////////////////////////////////////////////////////////////////
// Ultrasonic sensor objects for interacting with the ultrasonic sensors
////////////////////////////////////////////////////////////////////////////////////////
UltrasonicSensor *ultrasonicSensor;
//UltrasonicSensor *secondUltrasonicSensor;

////////////////////////////////////////////////////////////////////////////////////////
// Light sensor object
////////////////////////////////////////////////////////////////////////////////////////
LightSensor *lightSensor;

////////////////////////////////////////////////////////////////////////////////////////
// String variable to keep track of previous detection state
////////////////////////////////////////////////////////////////////////////////////////
String previousMessage = "";

////////////////////////////////////////////////////////////////////////////////////////
// keep track of current time in ms when one of the sensors read HIGH
////////////////////////////////////////////////////////////////////////////////////////
unsigned int timeOfDetection = 0;
unsigned int lastDetected = 0; // time since last detection in seconds
unsigned int minutes = 0;      // minutes that have passed since last detection
unsigned int alertMinutes = 0; // this will determine when an alert will be sent (if it's equal to or greater than EMERGENCY_ALERT_TIME

////////////////////////////////////////////////////////////////////////////////////////
// JSON Object we will send to the Bluetooth app. It keeps track of the status as well as time since last detection
////////////////////////////////////////////////////////////////////////////////////////
StaticJsonDocument<300> jsonData;
String stringData;

////////////////////////////////////////////////////////////////////////////////////////
// Motion Buffer class to store last 500 sensor readings from the motion sensor (work in progress)
////////////////////////////////////////////////////////////////////////////////////////

enum class BufferResult
{
    NO_ALERT,
    SEND_ALERT
};

class MotionBuffer
{
private:
    int buffer[MOTION_BUFFER_SIZE];
    int current_index = 0;
    bool full = false;

public:
    void appendReading(int value);
    void clear();
    bool isFull() const;
    BufferResult processBuffer();
};

void MotionBuffer::appendReading(int value)
{
    buffer[current_index] = value;
    current_index++;
    if (current_index >= MOTION_BUFFER_SIZE)
    {
        full = true;
    }
}

void MotionBuffer::clear()
{
    for (int i = 0; i < MOTION_BUFFER_SIZE; i++)
    {
        buffer[i] = int();
    }
    current_index = 0;
}

bool MotionBuffer::isFull() const { return full; }

BufferResult MotionBuffer::processBuffer()
{
    int num_high_readings = 0;
    for (int i = 0; i < MOTION_BUFFER_SIZE; i++)
    {
        switch (buffer[i])
        {
        case 0:
        {
            break;
        }
        case 1:
        {
            num_high_readings++;
            break;
        }
        }
    }
    if (num_high_readings < MOTION_BUFFER_CUTOFF)
    {
        return BufferResult::SEND_ALERT;
    }
    else
    {
        return BufferResult::NO_ALERT;
    }
    clear();
}

MotionBuffer motion_buffer;

// Tasks/functions for multithreading sensors
void lightTask(void *parameter)
{
    while (true)
    {
        lightSensor->start();
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}


// Helper function to log the relevent sensor data to the SD card
void logData()
{
    updateLocalTime(currentTime);
    char fileName[25] = "/logs/log-"; //Log name should never exceed 20 characters
    strcat(fileName,currentTime.month);
    strcat(fileName,"-");
    strcat(fileName,currentTime.day);
    strcat(fileName,"-");
    strcat(fileName,currentTime.year);
    strcat(fileName,".txt");
    //Serial.println(fileName);
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= interval)
    {

        previousMillis = currentMillis;

        // Get the current time
        char time[10] = {""}; //Time should never exceed 10 characters, this way it generates an error if it does
        strcat(time, currentTime.hour);
        strcat(time, ":");
        strcat(time, currentTime.min);
        strcat(time, ":");
        strcat(time, currentTime.sec);

        // Retrieve sensor data
        bool motion = motionSensor->getRawOutput();
        float distance = ultrasonicSensor->distance();
        float lightIntensity = lightSensor->lightValue();

        // Format the data string
        String dataString = String(time) + " -> distance: " + distance + ", motion: " + motion + ", light intensity: " + lightIntensity + "\n";

        // Write the data string to the SD card
        appendFile(SD, fileName, dataString.c_str());
    }
}

// data logging task
void DataloggingTask(void *parameter)
{
    while (true)
    {
        logData();
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}


void setup()
{
    Serial.begin(115200);
    while (!Serial){
        continue;
    }


    //Initialize WiFi
    wifiInit(); 

    //NTP Server Sync
    ntpSetup();
    updateLocalTime(currentTime);
    
    //Initialize SD Card
    initSDCard();

    //Set up fileserver to access logs
    fileServInit();

    motionSensor = new MotionSensor(motionSensorInputPin);
    ultrasonicSensor = new UltrasonicSensor(trigPin, echoPin);
    lightSensor = new LightSensor;

    lightSensor->setup();
    light_module.begin();

    // Initializing I2C (the vibration and light sensor uses I2C)
    Wire.begin();

    Serial.print("calibrating sensor ");
    for (int i = 0; i < calibrationTime; i++)
    {
        Serial.print(".");
        delay(1000);
    }
    Serial.println(" done");
    Serial.println("SENSOR ACTIVE");

    // Start the data logging task on core 0
    xTaskCreatePinnedToCore(DataloggingTask, "DataloggingTask", 10000, NULL, 0, NULL, 0);

    // Start the light and vibration sensors on a separate thread on core 0
    xTaskCreatePinnedToCore(lightTask, "LightTask", 10000, NULL, 0, NULL, 0);
}

void loop()
{

    // Motion and ultrasonic sensor are running on the main thread (on core 1)
    motionSensor->start();
    ultrasonicSensor->start();
    
    // Obtain the number of currently "active" sensors
    // Active means that the sensor has individually concluded that there is human presence/motion within the room
    int activeSensors = motionSensor->isActive() + ultrasonicSensor->isActive() + lightSensor->isActive();

    // Setting the key value pairs for the JSON that will be sent to the main sensor data BLE characteristic
    // Boolean Data
    jsonData["motion"] = motionSensor->isActive();
    jsonData["proximity"] = ultrasonicSensor->isActive();
    jsonData["light"] = lightSensor->isActive();
    //jsonData["occupied"] = parser.getOccupied();

    // Numerical Data
    jsonData["lightIntensity"] = lightSensor->lightValue();
    jsonData["distance"] = ultrasonicSensor->distance();
    jsonData["proximityBaseline"] = ultrasonicSensor->baseline();
    jsonData["lightBaseline"] = lightSensor->baseline();
    jsonData["lightOffBaseline"] = lightSensor->baseline_off();


    if (lightSensor->lightValue() >= 120)
    {
        jsonData["lightingStatus"] = "Lights on";
    }
    else
    {
        jsonData["lightingStatus"] = "Lights off";
    }

    // log the sensor data to the SD card

    ///////////////////////////////////////////////////////////////////////////////////////////////////////
    // Two or more of the sensors are reading high (person is present)
    ///////////////////////////////////////////////////////////////////////////////////////////////////////
    if (activeSensors >= 2)
    {
        timeOfDetection = millis();
        minutes = 0;
        alertMinutes = 0;

        jsonData["movement_status"] = "Person is present";
        jsonData["lastDetected"] = 0;
        serializeJson(jsonData, stringData);
        previousMessage = "Person is present";

        /////////////////////////////////////////////////////////////////////////////////////////////////////
        // All sensors read low (person is absent or not being detected for an extended period of time)
        /////////////////////////////////////////////////////////////////////////////////////////////////////
    }
    else
    {
        lastDetected = ((millis() - timeOfDetection) / 1000); // keeps track of time since last detection in seconds
        jsonData["movement_status"] = "Not present";

        jsonData["lastDetected"] = minutes;
        serializeJson(jsonData, stringData);
        previousMessage = "Not present";

        // If person isn't detected, keep track of last detection in minutes
        if (lastDetected >= 60)
        {
            lastDetected = 0;
            timeOfDetection = millis();
            minutes++;
            alertMinutes++;
            jsonData["lastDetected"] = minutes;

            // If this statement evaluates to true, an emergency alert notification will be sent to the Android device
            if (minutes % EMERGENCY_ALERT_TIME == 0 /*&& parser.getOccupied()*/)
            {
                // Start advertising and send an alert to the client device
                alertMinutes = 0;
            }

            serializeJson(jsonData, stringData);
        }
    }
    stringData = "";
}
