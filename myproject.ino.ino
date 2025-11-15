//Data Logging of Temperature and Humidity Values Using ESP32 with RS485 and RTC into SD Card
//Code:-
#include <Wire.h>
#include <ModbusMaster.h>
#include <SPI.h>
#include <SD.h>
#include <RTClib.h>

// RS485 (XY-MOD02) Connections
#define RS485_TX_PIN 33
#define RS485_RX_PIN 32
#define RS485_ENABLE_PIN 4

// SD Card Module (SPI) Connections
#define SD_CS_PIN 5  // Chip Select for SD Card

ModbusMaster node;
RTC_DS3231 rtc;
File logFile;

// Pre-transmission function
void preTransmission() {
  digitalWrite(RS485_ENABLE_PIN, HIGH);
}

// Post-transmission function
void postTransmission() {
  digitalWrite(RS485_ENABLE_PIN, LOW);
}

void setup() {
  Serial.begin(115200);

  // Initialize RS-485 communication
  pinMode(RS485_ENABLE_PIN, OUTPUT);
  digitalWrite(RS485_ENABLE_PIN, LOW);
  Serial1.begin(9600, SERIAL_8N1, RS485_RX_PIN, RS485_TX_PIN);
  node.begin(1, Serial1);
  node.preTransmission(preTransmission);
  node.postTransmission(postTransmission);

  // Initialize RTC
  Wire.begin();
  if (!rtc.begin()) {
    Serial.println("RTC not found!");
    while (1);
  }

  if (rtc.lostPower()) {
    Serial.println("RTC lost power, setting time...");
    rtc.adjust(DateTime(__DATE__, __TIME__)); // Set RTC to compile time
  }

  // OPTIONAL: Manually set RTC time (use once, then remove or comment out)
  // rtc.adjust(DateTime(2025, 1, 29, 14, 30, 0)); // YYYY, MM, DD, HH, MM, SS

  // Initialize SD Card
  if (!SD.begin(SD_CS_PIN)) {
    Serial.println("SD Card Initialization Failed!");
  } else {
    Serial.println("SD Card Ready.");
  }

  // Create the log file if it doesn't exist
  if (!SD.exists("/data_log.txt")) {
    logFile = SD.open("/data_log.txt", FILE_WRITE);
    if (logFile) {
      logFile.println("Timestamp, Temperature (°C), Humidity (%)");
      logFile.close();
    }
  }
}

void loop() {
  // 1️⃣ Read Current Time from RTC
  DateTime now = rtc.now();
  String timestamp = String(now.day()) + "-" + String(now.month()) + "-" + String(now.year()) + " " +
                     String(now.hour()) + ":" + String(now.minute()) + ":" + String(now.second());

  // 2️⃣ Read Temperature & Humidity from XY-MOD02
  uint8_t result;
  uint16_t tempData, humidityData;
  float temperature = -999, humidity = -999;

  result = node.readInputRegisters(0x0001, 1);
  if (result == node.ku8MBSuccess) {
    tempData = node.getResponseBuffer(0);
    temperature = tempData / 10.0;
  } else {
    Serial.println("Error reading temperature.");
  }

  result = node.readInputRegisters(0x0002, 1);
  if (result == node.ku8MBSuccess) {
    humidityData = node.getResponseBuffer(0);
    humidity = humidityData / 10.0;
  } else {
    Serial.println("Error reading humidity.");
  }

  // 3️⃣ Format Data String
  String dataString = timestamp + ", " + String(temperature, 1) + "°C, " + String(humidity, 1) + "%";
  
  // 4️⃣ Send Data Over RS485
  Serial.println("Sending via RS485: " + dataString);
  Serial1.println(dataString); // Send as ASCII over RS485

  // 5️⃣ Store Data in SD Card
  logFile = SD.open("/data_log.txt", FILE_APPEND);
  if (logFile) {
    logFile.println(dataString);
    logFile.close();
  } else {
    Serial.println("Failed to write to SD card!");
  }

  delay(2000);
}
