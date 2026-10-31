#pragma once

#include <Arduino.h>
#include <HardwareSerial.h>
#include <TinyGPSPlus.h>

struct GpsReading
{
    bool locationValid = false;
    double latitude = 0;
    double longitude = 0;
    bool timeValid = false;
    int localHour = 0;
    int minute = 0;
    int second = 0;
    bool dateValid = false;
    int localDay = 0;
    int localMonth = 0;
    int localYear = 0;
    int utcOffsetHours = 0;
    bool satellitesValid = false;
    uint32_t satellites = 0;
    bool altitudeValid = false;
    double altitudeMeters = 0;
    bool speedValid = false;
    double speedKmph = 0;
    bool courseValid = false;
    double courseDegrees = 0;
    bool hdopValid = false;
    double hdop = 0;
    bool hasReliableFix = false;
    bool noDataTimedOut = false;
};

class GpsManager
{
  public:
    GpsManager(uint8_t rxPin, uint8_t txPin, bool daylightSavingTime = false);

    void begin();
    void update();
    bool hasReliableFix();
    bool hasNoDataTimedOut() const;
    GpsReading getReading();
    void printReportIfDue();

  private:
    void syncClock();
    int utcOffsetHours() const;
    int localHour();
    void localDate(int& day, int& month, int& year);
    void printReport();

    TinyGPSPlus gps_;
    HardwareSerial serial_;
    uint8_t rxPin_;
    uint8_t txPin_;
    bool daylightSavingTime_;
    bool echoingNmea_ = true;
    unsigned long lastReportAt_ = 0;
    unsigned long lastClockSyncAt_ = 0;
    bool clockSynced_ = false;
};
